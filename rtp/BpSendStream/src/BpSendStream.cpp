#include "BpSendStream.h"
#include "Logger.h"
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpSendStream::BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, uint64_t numCircularBufferVectors, size_t maxOutgoingBundleSizeBytes, uint64_t numFifoBuffers) : BpSourcePattern(),
    m_running(true),
    m_maxIncomingUdpPacketSizeBytes(maxIncomingUdpPacketSizeBytes),
    m_incomingRtpStreamPort(incomingRtpStreamPort),
    m_maxOutgoingBundleSizeBytes(maxOutgoingBundleSizeBytes)
{
    /**
     * DtnRtp objects keep track of the RTP related paremeters such as sequence number and stream identifiers. 
     * The information in the header can be used to enhance audio/video (AV) playback.
     * Here, we have a queue for the incoming and outgoing RTP streams. 
     * BpSendStream has the ability to reduce RTP related overhead by concatenating RTP 
     * packets. The concatenation of these packets follows the guidelines presented in the CCSDS 
     * red book "SPECIFICATION FOR RTP AS TRANSPORT FOR AUDIO AND VIDEO OVER DTN CCSDS 766.3-R-1"
    */
    m_incomingDtnRtpPtr = std::make_shared<DtnRtp>(m_maxIncomingUdpPacketSizeBytes);
    m_outgoingDtnRtpPtr = std::make_shared<DtnRtp>(m_maxOutgoingBundleSizeBytes);

    m_processingThread = boost::make_unique<boost::thread>(boost::bind(boost::bind(&BpSendStream::ProcessIncomingBundlesThread, this))); 
    
    // processing will begin almost immediately, so call this when our callback and rtp session objects are already initialized
    m_bundleSinkPtr = std::make_shared<UdpBundleSink>(m_ioService, m_incomingRtpStreamPort, 
        boost::bind(&BpSendStream::WholeBundleReadyCallback, this, boost::placeholders::_1),
        numCircularBufferVectors, 
        maxIncomingUdpPacketSizeBytes, 
        boost::bind(&BpSendStream::DeleteCallback, this));
    m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioService));
    ThreadNamer::SetIoServiceThreadName(m_ioService, "ioServiceBpUdpSink");


    m_incomingCircularPacketQueue.set_capacity(numCircularBufferVectors);
    m_outgoingCircularFrameQueue.set_capacity(numCircularBufferVectors);


    // for (unsigned int i = 0; i < numCircularBufferVectors; ++i) {
    //     m_incomingBuffersCbVec[i].resize(maxIncomingUdpPacketSizeBytes);
    // }
    
}

BpSendStream::~BpSendStream()
{
    m_running = false;

    m_bundleSinkPtr.reset();
    
    if (m_ioServiceThreadPtr) {
        m_ioServiceThreadPtr->join();   
        m_ioServiceThreadPtr.reset(); //delete it
    }

    LOG_INFO(subprocess) << "Number of elements left in incoming queue: " << m_incomingCircularPacketQueue.size();
    LOG_INFO(subprocess) << "Number of elements left in outgoing queue: " << m_outgoingCircularFrameQueue.size();

    Stop();
}

void BpSendStream::WholeBundleReadyCallback(padded_vector_uint8_t &wholeBundleVec)
{
    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex);// lock mutex 
        // copy out bundle to local queue for processing
        m_incomingCircularPacketQueue.push_back(std::move(wholeBundleVec));
        
        // unlock 
    }
        
    m_outgoingQueueCv.notify_one();
}


/**
 * Some assumptions BpSendStream makes
 * - Processing thread can export bundles faster than they are imported by UdpBundleSink so there is no backlog
 * - RTP packets are delivered in order. If an old sequence number is delivered the packet is ignored. If a future sequence number
 *      is delivered (greated than +1 of the current), then the current concatentated frame (if there is one) is dropped. The next frame 
 *      starts with the aformentioned future sequence number.
 *      - Note: this is not particularly robust. In future implementations users can consider handling skipped frames and future frames
 *          in a more robust manner. The initial testing of this concept uses a local generated RTP stream from FFmpeg, so we can somewhat safely 
 *          assume that the incoming RTP packets are in order, and well formatted. 
 * - RTP UDP packets are delivered, not bundles. The UDP bundle sink name scheme is inherited, but we are really receiving UDP packets, not bundles.
 * 
*/
void BpSendStream::ProcessIncomingBundlesThread()
{
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static std::vector<uint8_t> currentFrame;  // make a member var
    size_t offset = 0;// make a member var

    currentFrame.reserve(m_maxOutgoingBundleSizeBytes);

    
    while (m_running) {
        bool inWaitForNewBundlesState = !TryWaitForIncomingDataAvailable(timeout);

        if (!inWaitForNewBundlesState) {
            // boost::this_thread::sleep_for(boost::chrono::milliseconds(250));
            rtp_packet_status_t packetStatus = m_incomingDtnRtpPtr->PacketHandler(m_incomingCircularPacketQueue.front(), (rtp_header *) currentFrame.data());
            
            switch(packetStatus) {
                /**
                 * For the first valid frame we receive assign the CSRC, sequence number, and generic status bits by copying in the first header
                 * Note - after this point, it is likely and intended that the sequence of the incoming and outgoing DtnRtp objects diverge.
                */
                case RTP_FIRST_FRAME:
                    m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data(), USE_INCOMING_SEQ);
                    CreateFrame(currentFrame, offset);
                    break;

                case RTP_CONCATENATE:
                    Concatenate(m_incomingCircularPacketQueue.front(), currentFrame, offset); // concatenation may fail if the bundle size is less than requested rtp frame, if so RTP_PUSH_PREVIOUS_FRAME is performed 
                    break;
                    
                case RTP_PUSH_PREVIOUS_FRAME: // push current frame and make incoming frame the current frame
                    PushFrame(currentFrame, offset);
                    CreateFrame(currentFrame, offset);
                    break;

                case RTP_OUT_OF_SEQ: 
                    PushFrame(currentFrame, offset); 
                    CreateFrame(currentFrame, offset);
                    break;
                case RTP_INVALID_HEADER: // discard incoming data
                case RTP_MISMATCH_SSRC: // discard incoming data
                case RTP_INVALID_VERSION: // discard incoming data
                default:
                    LOG_ERROR(subprocess) << "Unknown return type " << packetStatus;
            }
            m_incomingCircularPacketQueue.pop_front();
        }
    }
}
// Copy in our outgoing Rtp header and the next rtp frame payload
void BpSendStream::CreateFrame(std::vector<uint8_t> & currentFrame, size_t &offset)
{
    currentFrame.resize(m_incomingCircularPacketQueue.front().size()); 

    // fill header with outgoing Dtn Rtp header information
    // this is tranlating from incoming DtnRtp session to outgoing DtnRtp session
    memcpy(&currentFrame.front(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
    offset = sizeof(rtp_header);
    
    memcpy(&currentFrame.front(), 
            &m_incomingCircularPacketQueue.front() + offset,  // skip the incoming header for our outgoing header instead
            m_incomingCircularPacketQueue.front().size() - offset);
    offset = m_incomingCircularPacketQueue.front().size(); // assumes that this had aheader that we skipped

    m_outgoingDtnRtpPtr->IncNumConcatenated();
}

// if the current frame + incoming packet < bundle size, then concatenate
void BpSendStream::Concatenate(padded_vector_uint8_t &incomingRtpFrame, std::vector<uint8_t>  &currentFrame, size_t &offset)
{

    if ((offset + incomingRtpFrame.size() - sizeof(rtp_header)) < m_maxOutgoingBundleSizeBytes) { // concatenate if we have enough space
        currentFrame.resize(currentFrame.size() + incomingRtpFrame.size()); // enlarge buffer
        
        memcpy(&currentFrame.at(offset), // skip to our previous end of buffer
                &incomingRtpFrame + sizeof(rtp_header), // skip incoming header, use outgoing header
                incomingRtpFrame.size() - sizeof(rtp_header)); 
        offset += incomingRtpFrame.size() - sizeof(rtp_header); // did not copy header

        m_outgoingDtnRtpPtr->IncNumConcatenated();

        LOG_DEBUG(subprocess) << "Number of RTP packets in current frame: " << m_outgoingDtnRtpPtr->GetNumConcatenated();
 
    } else { // not enough space to concatenate, send separately
        LOG_DEBUG(subprocess) << "Not enough space to concatenate, sending as separate bundle.";
        PushFrame(currentFrame, offset);
        CreateFrame(currentFrame, offset);
    }
}

// Add current frame to the outgoing queue to be bundled and sent
void BpSendStream::PushFrame(std::vector<uint8_t> & currentFrame, size_t &offset)
{
    LOG_DEBUG(subprocess) << "Push frame with \n"
        << m_outgoingDtnRtpPtr->GetNumConcatenated() << " concatenated RTP packets\n"
        << ntohs(m_outgoingDtnRtpPtr->GetSequence()) << " =seq\n" 
        << ntohl(m_outgoingDtnRtpPtr->GetTimestamp()) << "=ts\n"
        << currentFrame.size() << "=size into outgoing queue";

    {
        boost::mutex::scoped_lock lock (m_ougoingQueueCv)    // lock mutex 

        m_outgoingCircularFrameQueue.push_back(std::move(currentFrame));
        
        currentFrame.resize(0);
        offset = 0;

    }

    m_outgoingQueueCv.notify_one();


    // Outgoing DtnRtp session needs to update seq status since we just sent an RTP frame
    m_outgoingDtnRtpPtr->IncSequence();
    m_outgoingDtnRtpPtr->ResetNumConcatenated();
    m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data(), USE_OUTGOING_SEQ); // updates our next header to have the correct ts, fmt, ext, m ect.


    // LOG_DEBUG(subprocess) << "out going circ buffer size : " << m_outgoingCircularFrameQueue.size(); 
}

void BpSendStream::DeleteCallback()
{
}


uint64_t BpSendStream::GetNextPayloadLength_Step1() 
{
    if (m_outgoingCircularFrameQueue.size() == 0) {
        return UINT64_MAX; // wait for data
    } 

    return (uint64_t) m_outgoingCircularFrameQueue.front().size();
}

bool BpSendStream::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    // copy out rtp packet
    memcpy(destinationBuffer, m_outgoingCircularFrameQueue.front().data(), m_outgoingCircularFrameQueue.front().size());

    rtp_frame * frame = (rtp_frame * ) m_outgoingCircularFrameQueue.front().data();

    LOG_DEBUG(subprocess) << "Copied out frame with\n " << \
            "seq=" << ntohs(frame->header.seq)     << "\n" << \
            "size=" << m_outgoingCircularFrameQueue.front().size();
                
    // pop outgoing queue
    m_outgoingCircularFrameQueue.pop_front();

    LOG_DEBUG(subprocess) << "Size after copy and popping out " << m_outgoingCircularFrameQueue.size();

    return true;
}

/**
 * If TryWaitForDataAvailable returns true, BpSourcePattern will move to export data (Step1 and Step2). 
 * If TryWaitForDataAvailable returns false, BpSourcePattern will recall this function after a timeout
*/
bool BpSendStream::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_outgoingCircularFrameQueue.size()==0) {
        return GetNextOutgoingPacketTimeout(timeout);
    }
    return true; //
}

/**
 * If return true, we have data
 * If return false, we do not have data
*/
bool BpSendStream::TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_incomingCircularPacketQueue.size() == 0) { // if empty, we wait
        return GetNextIncomingPacketTimeout(timeout);
    }
    return true; 
}

bool BpSendStream::GetNextOutgoingPacketTimeout(const boost::posix_time::time_duration& timeout)
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    bool inWaitForPacketState = (m_outgoingCircularFrameQueue.size() == 0);

    if (inWaitForPacketState) {
        m_outgoingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
    }

    return !inWaitForPacketState;
}

bool BpSendStream::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration& timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingCircularPacketQueue.size() == 0)) {
        m_incomingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}

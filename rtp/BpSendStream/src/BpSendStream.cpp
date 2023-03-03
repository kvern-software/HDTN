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

    // allocate buffers for incoming and outgoing packets
    // for (unsigned int i = 0; i < numFifoBuffers; ++i) {
    //     m_incomingPacketQueue[i].resize(maxIncomingUdpPacketSizeBytes);
    // }

    // for (unsigned int i = 0; i < numFifoBuffers; ++i) {
    //     m_OutgoingPacketQueue[i].resize(maxOutgoingBundleSizeBytes);
    // }
    m_incomingCircularPacketQueue.set_capacity(100);
    m_outgoingCircularFrameQueue.set_capacity(100);
    
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
    // copy out bundle to local queue for processing
    m_incomingCircularPacketQueue.push_back(std::move(wholeBundleVec));
    LOG_DEBUG(subprocess) << "callback " << m_incomingCircularPacketQueue.size();
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
    // LOG_INFO(subprocess) << "Entered processing thread";
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(25));
    static std::vector<uint8_t> currentFrame;
    currentFrame.resize(m_maxOutgoingBundleSizeBytes); // todo account for bundle overhead

    size_t offset = 0;
    
    while (m_running) {
        bool inWaitForNewBundlesState = !TryWaitForIncomingDataAvailable(timeout);

        if (!inWaitForNewBundlesState) {
            LOG_DEBUG(subprocess) << "Data available";
            rtp_packet_status_t packetStatus = m_incomingDtnRtpPtr->PacketHandler(m_incomingCircularPacketQueue.front(), (rtp_header *) currentFrame.data());
            
            switch(packetStatus) {
                /**
                 * For the first valid frame we receive assign the CSRC, sequence number, and generic status bits by copying in the first header
                 * Note - after this point, it is likely and intended that the sequence of the incoming and outgoing DtnRtp objects diverge.
                */
                case RTP_FIRST_FRAME:
                    m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data());
                    CreateFrame(currentFrame, offset);
                    break;

                case RTP_CONCATENATE:
                    // concatenation may fail if the bundle size is less than requested rtp frame, if so, goto RTP_PUSH_PREVIOUS_FRAME
                    packetStatus = Concatenate(m_incomingCircularPacketQueue.front(), currentFrame, offset);
                    if (packetStatus == RTP_CONCATENATE) 
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
    // fill header with outgoing Dtn Rtp header information
    // this is tranlating from incoming DtnRtp session to outgoing DtnRtp session
    memcpy(&currentFrame.front(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
    offset = sizeof(rtp_header);
    
    memcpy(&currentFrame.front(), 
            &m_incomingCircularPacketQueue.front() + offset,  // skip the incoming header for our outgoing header instead
            m_incomingCircularPacketQueue.front().size() - offset);
    offset = m_incomingCircularPacketQueue.front().size(); // assumes that this had a header that we skipped
}

// if the current frame + incoming packet < bundle size, then concatenate
rtp_packet_status_t BpSendStream::Concatenate(padded_vector_uint8_t &incomingRtpFrame, std::vector<uint8_t>  &currentFrame, size_t &offset)
{
    if ((offset + incomingRtpFrame.size() - sizeof(rtp_header)) < m_maxOutgoingBundleSizeBytes) { // concatenate if we have enough space
        memcpy(&currentFrame.at(offset), // skip to our previous end of buffer
                &incomingRtpFrame + sizeof(rtp_header), // skip incoming header, use outgoing header
                incomingRtpFrame.size() - sizeof(rtp_header)); 
        offset += incomingRtpFrame.size() - sizeof(rtp_header); // did not copy header
        return RTP_CONCATENATE;
    } else { // not enough space to concatenate, send separately
        return RTP_PUSH_PREVIOUS_FRAME;
    }
}

// Add current frame to the outgoing queue to be bundled and sent
void BpSendStream::PushFrame(std::vector<uint8_t> & currentFrame, size_t &offset)
{
    m_outgoingCircularFrameQueue.push_back(std::move(currentFrame));
    currentFrame.resize(m_maxOutgoingBundleSizeBytes);
    offset = 0;

    std::cout << "out goiung circ buffer size : " << m_outgoingCircularFrameQueue.size() << std::endl; 
    // increment sequence number
    // m_OutgoingPacketQueue.`
}

void BpSendStream::DeleteCallback()
{
    
}


uint64_t BpSendStream::GetNextPayloadLength_Step1() 
{

    if (m_outgoingCircularFrameQueue.size() == 0) {
        LOG_INFO(subprocess) << "waiting for data";
        return UINT64_MAX; // wait for data
    } 
    

    return (uint64_t) m_outgoingCircularFrameQueue.front().size();
}

bool BpSendStream::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    // copy out rtp packet
    // memcpy(destinationBuffer, &m_OutgoingPacketQueue.front(), m_OutgoingPacketQueue.front().size());

    // pop outgoing queue
    m_outgoingCircularFrameQueue.pop_front();

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

    return inWaitForPacketState;
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

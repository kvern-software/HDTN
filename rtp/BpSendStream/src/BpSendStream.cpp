#include "BpSendStream.h"
#include "Logger.h"
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpSendStream::BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, size_t numCircularBufferVectors, 
        size_t maxOutgoingBundleSizeBytes, bool enableRtpConcatentation, std::string sdpFile, uint64_t sdpInterval_ms, uint16_t numRtpPacketsPerBundle) : BpSourcePattern(),
    m_running(true),
    m_maxIncomingUdpPacketSizeBytes(maxIncomingUdpPacketSizeBytes),
    m_incomingRtpStreamPort(incomingRtpStreamPort),
    m_maxOutgoingBundleSizeBytes(maxOutgoingBundleSizeBytes),
    m_enableRtpConcatentation(enableRtpConcatentation),
    m_sdpFileStr(sdpFile),
    m_sdpInterval_ms(sdpInterval_ms),
    m_numRtpPacketsPerBundle(numRtpPacketsPerBundle)
{
    m_currentFrame.reserve(m_maxOutgoingBundleSizeBytes);

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

    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&BpSendStream::ProcessIncomingBundlesThread, this)); 
    m_sdpThread = boost::make_unique<boost::thread>(boost::bind(&BpSendStream::SdpTimerThread, this)); 

    // processing will begin almost immediately, so call this when our callback and rtp session objects are already initialized
    m_bundleSinkPtr = std::make_shared<UdpBundleSink>(m_ioService, m_incomingRtpStreamPort, 
        boost::bind(&BpSendStream::WholeBundleReadyCallback, this, boost::placeholders::_1),
        numCircularBufferVectors, 
        maxIncomingUdpPacketSizeBytes, 
        boost::bind(&BpSendStream::DeleteCallback, this));
    m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioService));
    ThreadNamer::SetIoServiceThreadName(m_ioService, "ioServiceBpUdpSink");

    m_incomingCircularPacketQueue.set_capacity(numCircularBufferVectors);
    m_outgoingCircularBundleQueue.set_capacity(numCircularBufferVectors);    
}

BpSendStream::~BpSendStream()
{
    m_running = false;

    m_bundleSinkPtr.reset();
    
    if (m_ioServiceThreadPtr) {
        m_ioServiceThreadPtr->join();   
        m_ioServiceThreadPtr.reset(); //delete it
    }

    Stop();

    LOG_INFO(subprocess) << "m_incomingCircularPacketQueue.size(): " << m_incomingCircularPacketQueue.size();
    LOG_INFO(subprocess) << "m_outgoingCircularBundleQueue.size(): " << m_outgoingCircularBundleQueue.size();
    LOG_INFO(subprocess) << "m_totalRtpPacketsReceived: " << m_totalRtpPacketsReceived;
    LOG_INFO(subprocess) << "m_totalRtpPacketsSent: " << m_totalRtpPacketsSent ;
    LOG_INFO(subprocess) << "m_totalRtpPacketsQueued: " << m_totalRtpPacketsQueued;
    LOG_INFO(subprocess) << "m_totalConcatenationsPerformed" << m_totalConcatenationsPerformed;
    LOG_INFO(subprocess) << "m_totalIncomingCbOverruns: " << m_totalIncomingCbOverruns;
    LOG_INFO(subprocess) << "m_totalOutgoingCbOverruns: " << m_totalOutgoingCbOverruns;

    // LOG_INFO(subprocess) << "m_totalMarkerBits" << m_totalMarkerBits;
    // LOG_INFO(subprocess) << "m_totalTimestampChanged" << m_totalTimestampChanged;

}

void BpSendStream::WholeBundleReadyCallback(padded_vector_uint8_t &wholeBundleVec)
{
    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex);// lock mutex 
        // LOG_DEBUG(subprocess) << "Pushing frame into incoming queue";
        if (m_incomingCircularPacketQueue.full())
            m_totalIncomingCbOverruns++;

        m_incomingCircularPacketQueue.push_back(std::move(wholeBundleVec));  // copy out bundle to local queue for processing
        // rtp_header * header = (rtp_header *) wholeBundleVec.data();
    }
    m_incomingQueueCv.notify_one();
    m_totalRtpPacketsReceived++;
}


/**
 * - RTP UDP packets are delivered, not bundles. The UDP bundle sink name scheme is inherited, but we are really receiving UDP packets, not bundles.
*/
void BpSendStream::ProcessIncomingBundlesThread()
{
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));

    while (m_running) {
        bool notInWaitForNewBundlesState = TryWaitForIncomingDataAvailable(timeout);

        if (notInWaitForNewBundlesState) {
            // LOG_DEBUG(subprocess) << "Processing front of incoming queue"; 
            // boost::this_thread::sleep_for(boost::chrono::milliseconds(250));
            {
                boost::mutex::scoped_lock lock(m_incomingQueueMutex);

                padded_vector_uint8_t &incomingRtpFrame = m_incomingCircularPacketQueue.front(); 
                rtp_packet_status_t packetStatus = m_incomingDtnRtpPtr->PacketHandler(incomingRtpFrame, (rtp_header *) m_currentFrame.data());
                
                switch(packetStatus) {
            //         /**
            //          * For the first valid frame we receive assign the CSRC, sequence number, and generic status bits by copying in the first header
            //          * Note - after this point, it is likely and intended that the sequence of the incoming and outgoing DtnRtp objects diverge.
            //         */
                    case RTP_FIRST_FRAME:
                        m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) incomingRtpFrame.data(), USE_INCOMING_SEQ);
                        CreateFrame();
                        break;

                    case RTP_CONCATENATE:
                        if (m_enableRtpConcatentation) {
                            // concatenation may fail if the bundle size is less than requested rtp frame, if so RTP_PUSH_PREVIOUS_FRAME is performed 
                            Concatenate(incomingRtpFrame); 
                            break;
                        } else {
                            PushFrame();
                            CreateFrame();
                            break;
                        }
                        
                    case RTP_PUSH_PREVIOUS_FRAME: // push current frame and make incoming frame the current frame
                        PushFrame();
                        CreateFrame();
                        break;

                    case RTP_OUT_OF_SEQ: 
                        PushFrame(); 
                        CreateFrame();
                        break;

                    case RTP_INVALID_HEADER: // discard incoming data
                    case RTP_MISMATCH_SSRC: // discard incoming data
                    case RTP_INVALID_VERSION: // discard incoming data
                    default:
                        LOG_ERROR(subprocess) << "Unknown return type " << packetStatus;
                }

                // LOG_DEBUG(subprocess) << "Popping front of incoming queue";
                m_incomingCircularPacketQueue.pop_front(); // already locked, safe to pop
                // LOG_DEBUG(subprocess) << "Successful pop of incoming queue";

            }
        }
    }
}
// Copy in our outgoing Rtp header and the next rtp frame payload
void BpSendStream::CreateFrame()
{
    m_currentFrame.resize(m_incomingCircularPacketQueue.front().size()); 

    memcpy(&m_currentFrame.front(), m_incomingCircularPacketQueue.front().data(), m_incomingCircularPacketQueue.front().size());
    
    // fill header with outgoing Dtn Rtp header information
    // this is tranlating from incoming DtnRtp session to outgoing DtnRtp session
    // memcpy(&m_currentFrame.front(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
    // memcpy(&m_currentFrame.front() + sizeof(rtp_header), 
            // &m_incomingCircularPacketQueue.front() + sizeof(rtp_header),  // skip the incoming header for our outgoing header instead
            // m_incomingCircularPacketQueue.front().size() - sizeof(rtp_header));

    m_offset = m_incomingCircularPacketQueue.front().size(); // assumes that this had a header that we skipped

    m_outgoingDtnRtpPtr->IncNumConcatenated();
}

// if the current frame + incoming packet payload < bundle size, then concatenate
void BpSendStream::Concatenate(padded_vector_uint8_t &incomingRtpFrame)
{
    if ((m_offset + incomingRtpFrame.size() - sizeof(rtp_header)) < m_maxOutgoingBundleSizeBytes) { // concatenate if we have enough space
        m_currentFrame.resize(m_currentFrame.size() + incomingRtpFrame.size() - sizeof(rtp_header)); // enlarge buffer
        
        memcpy(&m_currentFrame.at(m_offset), // skip to our previous end of buffer
                &incomingRtpFrame + sizeof(rtp_header), // skip incoming header, use outgoing header
                incomingRtpFrame.size() - sizeof(rtp_header)); 
        m_offset += incomingRtpFrame.size() - sizeof(rtp_header); // did not copy header

        m_outgoingDtnRtpPtr->IncNumConcatenated();
        m_totalConcatenationsPerformed++;

        // LOG_DEBUG(subprocess) << "Number of RTP packets in current frame: " << m_outgoingDtnRtpPtr->GetNumConcatenated();
    } else { // not enough space to concatenate, send separately
        // LOG_DEBUG(subprocess) << "Not enough space to concatenate, sending as separate bundle.";
        PushFrame();
        CreateFrame();
    }
}

// Add current frame to the outgoing queue to be bundled and sent
void BpSendStream::PushFrame()
{
    // prepend the payload size to the payload here
    size_t rtpFrameSize = m_currentFrame.size();
    padded_vector_uint8_t rtpPacketSizeAndPacket(m_currentFrame.size() + sizeof(size_t));
    memcpy(rtpPacketSizeAndPacket.data(), &rtpFrameSize, sizeof(size_t));
    memcpy(rtpPacketSizeAndPacket.data() + sizeof(size_t), m_currentFrame.data(), m_currentFrame.size());
    m_currentFrame.resize(0);


    // move into rtp packet group. note that the payload size is 4 bytes in front of the data() pointer
    m_outgoingRtpPacketQueue.push(std::move(rtpPacketSizeAndPacket)); // new element at end 
    m_rtpBytesInQueue += rtpFrameSize + sizeof(size_t);

    // LOG_DEBUG(subprocess) << "Inserted " << rtpFrameSize << " byte rtp packet into queue";
    // LOG_DEBUG(subprocess) << "m_rtpBytesInQueue = " <<  m_rtpBytesInQueue;

    // Outgoing DtnRtp session needs to update seq status since we just sent an RTP frame. This is the seq number used for the next packet
    m_outgoingDtnRtpPtr->IncSequence();
    m_outgoingDtnRtpPtr->ResetNumConcatenated();
    m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data(), USE_OUTGOING_SEQ); // updates our next header to have the correct ts, fmt, ext, m ect.

    m_offset = 0;
    
    m_totalRtpPacketsQueued++;

    if (m_outgoingRtpPacketQueue.size() == m_numRtpPacketsPerBundle) // we should move this packet group into the bundle queue
    {
        PushBundle();
    }
}

void BpSendStream::PushBundle()
{
    // LOG_DEBUG(subprocess) << "pushing bundle";

    // determine how much memory we need to allocate for this final bundle (rtp packets + payload_size * num payloads)
    size_t bundleSize = m_rtpBytesInQueue;
    
    std::vector<uint8_t> outgoingBundle(bundleSize);
    // LOG_DEBUG(subprocess) << "allocated  bundle size " << bundleSize;

    size_t offset = 0;
    
    uint64_t count = 0;
    while (m_outgoingRtpPacketQueue.size() != 0)
    {
        // copy the packet size and rtp packet into our bundle
        memcpy(outgoingBundle.data() + offset, m_outgoingRtpPacketQueue.front().data(), m_outgoingRtpPacketQueue.front().size());
        offset +=  m_outgoingRtpPacketQueue.front().size();

        // size_t * length = (size_t * ) m_outgoingRtpPacketQueue.front().data();
        // std::cout << *length;
        // rtp_frame * frame = (rtp_frame *)  m_outgoingRtpPacketQueue.front().data() + sizeof(size_t);
        // frame->print_header();

        m_outgoingRtpPacketQueue.pop();
        count ++;
        // LOG_DEBUG(subprocess) << "copied rtp packet " << count << " into bundle";
    }
    
    m_rtpBytesInQueue = 0;

    {
        boost::mutex::scoped_lock lock(m_outgoingQueueMutex);    // lock mutex 
        if (m_outgoingCircularBundleQueue.full())
            m_totalOutgoingCbOverruns++;
        m_outgoingCircularBundleQueue.push_back(std::move(outgoingBundle)); 
    }
    m_outgoingQueueCv.notify_one();
}

void BpSendStream::DeleteCallback()
{
}

uint64_t BpSendStream::GetNextPayloadLength_Step1() 
{
    m_outgoingQueueMutex.lock();

    if (m_outgoingCircularBundleQueue.size() == 0) {
        m_outgoingQueueMutex.unlock();

        // LOG_DEBUG(subprocess) << "Circ Queue Size: " << m_outgoingCircularBundleQueue.size() << " waiting...";
        return UINT64_MAX; // wait for data
    } 

    return (uint64_t) m_outgoingCircularBundleQueue.front().size();
}

bool BpSendStream::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    memcpy(destinationBuffer, m_outgoingCircularBundleQueue.front().data(), m_outgoingCircularBundleQueue.front().size());
    m_outgoingCircularBundleQueue.pop_front(); 
    m_outgoingQueueMutex.unlock();
    m_outgoingQueueCv.notify_one();
    m_totalRtpPacketsSent++; 

    return true;
}

/**
 * If TryWaitForDataAvailable returns true, BpSourcePattern will move to export data (Step1 and Step2). 
 * If TryWaitForDataAvailable returns false, BpSourcePattern will recall this function after a timeout
*/
bool BpSendStream::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_outgoingCircularBundleQueue.size()==0) {
        return GetNextOutgoingPacketTimeout(timeout);
    }

    return true; 
}
bool BpSendStream::GetNextOutgoingPacketTimeout(const boost::posix_time::time_duration& timeout)
{
    boost::mutex::scoped_lock lock(m_outgoingQueueMutex);
    bool inWaitForPacketState = (m_outgoingCircularBundleQueue.size() == 0);

    if (inWaitForPacketState) {
        m_outgoingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }

    return true;
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


bool BpSendStream::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingCircularPacketQueue.size() == 0)) {
        m_incomingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}

bool BpSendStream::SdpTimerThread()
{
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    
    std::vector<uint8_t> sdpBuffer(m_sdpFileStr.size() + sizeof(uint8_t));
    uint8_t header = SDP_FILE_STR_HEADER;
    memcpy(sdpBuffer.data(), &header, sizeof(uint8_t));
    memcpy(sdpBuffer.data() + sizeof(uint8_t), m_sdpFileStr.data(), m_sdpFileStr.size());

    while (1)
    {     
        {
            boost::mutex::scoped_lock lock(m_outgoingQueueMutex);
            m_outgoingCircularBundleQueue.push_back(sdpBuffer);
        }

        m_outgoingQueueCv.notify_one();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(m_sdpInterval_ms));
    }
    return false;
}
#include "BpSendStream.h"
#include "Logger.h"
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpSendStream::BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, size_t numCircularBufferVectors, 
        size_t maxOutgoingBundleSizeBytes, bool enableRtpConcatentation, std::string sdpFile, uint64_t sdpInterval_ms) : BpSourcePattern(),
    m_running(true),
    m_maxIncomingUdpPacketSizeBytes(maxIncomingUdpPacketSizeBytes),
    m_incomingRtpStreamPort(incomingRtpStreamPort),
    m_maxOutgoingBundleSizeBytes(maxOutgoingBundleSizeBytes),
    m_enableRtpConcatentation(enableRtpConcatentation),
    m_sdpFileStr(sdpFile),
    m_sdpInterval_ms(sdpInterval_ms)
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
    m_outgoingCircularFrameQueue.set_capacity(numCircularBufferVectors);    
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
    LOG_INFO(subprocess) << "m_outgoingCircularFrameQueue.size(): " << m_outgoingCircularFrameQueue.size();
    LOG_INFO(subprocess) << "m_totalRtpPacketsReceived: " << m_totalRtpPacketsReceived - 1;
    LOG_INFO(subprocess) << "m_totalRtpPacketsSent: " << m_totalRtpPacketsSent - 1;
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
    // LOG_DEBUG(subprocess) 
    //     // << m_outgoingDtnRtpPtr->GetNumConcatenated() << " concatenated RTP packets\n"
    //     << ntohs(m_outgoingDtnRtpPtr->GetSequence()) << "=seq & " 
    //     // << ntohl(m_outgoingDtnRtpPtr->GetTimestamp()) << "=ts\n"
    //     << m_currentFrame.size() << "=size into outgoing queue";
    
    {
        boost::mutex::scoped_lock lock(m_outgoingQueueMutex);    // lock mutex 
        if (m_outgoingCircularFrameQueue.full())
            m_totalOutgoingCbOverruns++;

        m_outgoingCircularFrameQueue.push_back(std::move(m_currentFrame));
    } 
    m_outgoingQueueCv.notify_one();
    
    // Outgoing DtnRtp session needs to update seq status since we just sent an RTP frame. This is the seq number used for the next packet
    m_outgoingDtnRtpPtr->IncSequence();
    m_outgoingDtnRtpPtr->ResetNumConcatenated();
    m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data(), USE_OUTGOING_SEQ); // updates our next header to have the correct ts, fmt, ext, m ect.

    m_currentFrame.resize(0);
    m_offset = 0;
    

    m_totalRtpPacketsQueued++;
}

void BpSendStream::DeleteCallback()
{
}


uint64_t BpSendStream::GetNextPayloadLength_Step1() 
{
    // m_sdpMutex.lock();
    if (m_sendSdp) {
        LOG_INFO(subprocess) << "Sending SDP file size " << m_sdpFileStr.size();
        return m_sdpFileStr.size() + sizeof(SDP_FILE_STR_HEADER);
    }

    m_outgoingQueueMutex.lock();

    if (m_outgoingCircularFrameQueue.size() == 0) {
        m_outgoingQueueMutex.unlock();

        // LOG_DEBUG(subprocess) << "Circ Queue Size: " << m_outgoingCircularFrameQueue.size() << " waiting...";
        return UINT64_MAX; // wait for data
    } 

    return (uint64_t) m_outgoingCircularFrameQueue.front().size();
}

bool BpSendStream::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    if (m_sendSdp) 
    {
        m_sendSdp = false;
        uint8_t header = SDP_FILE_STR_HEADER;
        memcpy(destinationBuffer, &header, sizeof(uint8_t));
        memcpy(destinationBuffer + sizeof(uint8_t), m_sdpFileStr.data(), m_sdpFileStr.size());
        m_sdpMutex.unlock();
        m_sdpCv.notify_one();
        LOG_INFO(subprocess) << "Sent SDP information";
        return true;
    }
    
    
    // LOG_DEBUG(subprocess) << "Popping outgoing queue seq";
    memcpy(destinationBuffer, m_outgoingCircularFrameQueue.front().data(), m_outgoingCircularFrameQueue.front().size());
    m_outgoingCircularFrameQueue.pop_front(); 
    m_outgoingQueueMutex.unlock();
    m_outgoingQueueCv.notify_one();
    // LOG_DEBUG(subprocess) << "Succesesful pop of outgoing queue - new size:" << m_outgoingCircularFrameQueue.size();;
    
    m_totalRtpPacketsSent++;
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

    return true; 
}
bool BpSendStream::GetNextOutgoingPacketTimeout(const boost::posix_time::time_duration& timeout)
{
    boost::mutex::scoped_lock lock(m_outgoingQueueMutex);
    bool inWaitForPacketState = (m_outgoingCircularFrameQueue.size() == 0);

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


    while (1)
    {
        {
            // boost::mutex::scoped_lock lock(m_sdpMutex);
            
            if (m_sendSdp == false)
            {
                // LOG_DEBUG(subprocess) << "sleeping SDP";
                boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));
            // } else {
                // m_sdpCv.timed_wait(lock, timeout);
                m_sendSdp = true;
            }
            // LOG_DEBUG(subprocess) << "waiting for sdp to send";
            boost::this_thread::sleep_for(boost::chrono::milliseconds(250));

        }

    }
    return false;
}
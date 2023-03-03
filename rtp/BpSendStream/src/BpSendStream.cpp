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
    
}

BpSendStream::~BpSendStream()
{
    m_running = false;

    m_bundleSinkPtr.reset();
    
    if (m_ioServiceThreadPtr) {
        m_ioServiceThreadPtr->join();   
        m_ioServiceThreadPtr.reset(); //delete it
    }

    LOG_INFO(subprocess) << "Number of elements left in incoming queue: " << m_incomingPacketQueue.size();

    Stop();
}





void BpSendStream::WholeBundleReadyCallback(padded_vector_uint8_t &wholeBundleVec)
{
    // LOG_INFO(subprocess) << "Got bundle size " << wholeBundleVec.size();
    // copy out bundle to local queue for processing
    m_incomingPacketQueue.emplace(std::move(wholeBundleVec));
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
    static std::vector<uint8_t> currentFrame;
    currentFrame.resize(m_maxOutgoingBundleSizeBytes); // todo account for bundle overhead

    size_t offset;

    m_OutgoingPacketQueue.emplace(std::vector<uint8_t>());
    
    while (m_running) {
        if (m_incomingPacketQueue.size() > 0) {          
            rtp_packet_status_t packetStatus = m_incomingDtnRtpPtr->PacketHandler(m_incomingPacketQueue.front(), (rtp_header *) currentFrame.data());
            // process incoming data in our packet handler
            switch(packetStatus) {
                case RTP_FIRST_FRAME:
                    /**
                     * For the first valid frame we receive assign the CSRC, sequence number, and generic status bits by copying in the first header
                     * Note - after this point, it is likely and intended that the sequence of the incoming and outgoing DtnRtp objects diverge.
                    */
                    m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingPacketQueue.front().data());
                    CreateFrame(currentFrame, offset);
                    
                    break;
                case RTP_CONCATENATE:
                    Concatenate(m_incomingPacketQueue.front(), currentFrame, offset);
                    break;

                case RTP_PUSH_PREVIOUS_FRAME: // push current frame and make incoming frame the current frame
                    PushFrame(currentFrame, offset);
                    
                    CreateFrame(currentFrame, offset);
                    break;

                case RTP_OUT_OF_SEQ: 
                    // push current frame, make new frame on new sequence
                    m_OutgoingPacketQueue.emplace(std::move(currentFrame));
                    currentFrame.resize(m_maxOutgoingBundleSizeBytes);
                    offset = 0;

                    memcpy(&currentFrame.front(),
                            &m_incomingPacketQueue.front(), 
                            m_incomingPacketQueue.front().size());

                    break;
                case RTP_INVALID_HEADER: // discard incoming data
                case RTP_MISMATCH_SSRC: // discard incoming data
                case RTP_INVALID_VERSION: // discard incoming data
                default:
                    LOG_ERROR(subprocess) << "Unknown return type " << packetStatus;
                
            }
            m_incomingPacketQueue.pop();

            // LOG_INFO(subprocess) << "sizeof outgoing queue: " <<  m_OutgoingPacketQueue.size();
        } else {
            // boost::this_thread::sleep_for(boost::chrono::microseconds(1));
        }
    }

    
}

// Add current frame to the outgoing queue to be bundled and sent
void BpSendStream::PushFrame(std::vector<uint8_t> & currentFrame, size_t &offset)
{
    m_OutgoingPacketQueue.emplace(std::move(currentFrame));
    currentFrame.resize(m_maxOutgoingBundleSizeBytes);
    offset = 0;

    // increment sequence number
    // m_OutgoingPacketQueue.`
}

void BpSendStream::CreateFrame(std::vector<uint8_t> & currentFrame, size_t &offset)
{
    // fill header with outgoing Dtn Rtp header information
    memcpy(&currentFrame.front(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));

    offset = sizeof(rtp_header);
    
    memcpy(&currentFrame.front(), 
            &m_incomingPacketQueue.front() + offset,  // skip the incoming header for our own instead
            m_incomingPacketQueue.front().size() - offset);
    
    offset = m_incomingPacketQueue.front().size(); // assumes that this had a header that we skipped
}

void BpSendStream::Concatenate(padded_vector_uint8_t &incomingRtpFrame, std::vector<uint8_t>  &currentFrame, size_t &offset)
{
    if ((offset + m_incomingPacketQueue.front().size()) < m_maxOutgoingBundleSizeBytes) { // concatenate if we have enough space
        memcpy(&currentFrame.at(offset),
                &m_incomingPacketQueue.front() + sizeof(rtp_header), // skip incoming
                m_incomingPacketQueue.front().size() - sizeof(rtp_header));  // concatenate current frame with incoming rtp frame (do not want to repeat header)
        offset += m_incomingPacketQueue.front().size() - sizeof(rtp_header);
    } else { // not enough space to concatenate, send separately
        // copy out current frame 
        m_OutgoingPacketQueue.emplace(std::move(currentFrame));
        currentFrame.resize(m_maxOutgoingBundleSizeBytes);
        offset = 0;

        // set header for next frame using the outgoing DtnRtp 

        // add incoming packet to next frame
        // memcpy(&currentFrame.front(), 
                // &m_incomingPacketQueue.front(),
                // m_incomingPacketQueue.front().size());
        // offset = m_incomingPacketQueue.front().size();
    }
}

void BpSendStream::DeleteCallback()
{
    
}


uint64_t BpSendStream::GetNextPayloadLength_Step1() 
{
    if (m_OutgoingPacketQueue.empty()) {
        LOG_INFO(subprocess) << "waiting for data";
        return UINT64_MAX; // wait for data
    } 

    return (uint64_t) m_OutgoingPacketQueue.front().size();
}

bool BpSendStream::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    // copy out rtp packet
    memcpy(destinationBuffer, &m_OutgoingPacketQueue.front(), m_OutgoingPacketQueue.front().size());

    // pop outgoing queue
    m_OutgoingPacketQueue.pop();

    return true;
}

/**
 * If TryWaitForDataAvailable returns true, BpSourcePattern will move to export data (Step1 and Step2). 
 * If TryWaitForDataAvailable returns false, BpSourcePattern will recall this function after a timeout
*/
bool BpSendStream::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_OutgoingPacketQueue.empty()) {
        return GetNextQueueTimeout(timeout);
    }
    return true; //
}

bool BpSendStream::GetNextQueueTimeout(const boost::posix_time::time_duration& timeout)
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    bool inWaitForPacketState = m_OutgoingPacketQueue.empty();

    if (inWaitForPacketState) {
        m_queueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
    }

    return inWaitForPacketState;
}

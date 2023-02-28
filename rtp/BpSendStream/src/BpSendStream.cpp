#include "BpSendStream.h"
#include "Logger.h"
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpSendStream::BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, uint64_t numCircularBufferVectors, size_t maxOutgoingBundleSizeBytes) : BpSourcePattern(),
    m_running(true),
    m_incomingRtpStreamPort(incomingRtpStreamPort),
    m_maxIncomingUdpPacketSizeBytes(maxIncomingUdpPacketSizeBytes),
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
    m_incomingDtnRtpPtr = std::make_shared<DtnRtp>();
    m_outgoingDtnRtpPtr = std::make_shared<DtnRtp>();

    m_processingThread = boost::make_unique<boost::thread>(boost::bind(boost::bind(&BpSendStream::ProcessIncomingBundlesThread, this))); 
    
    // processing will begin almost immediately, so call this when our callback and rtp session objects are already initialized
    m_bundleSinkPtr = std::make_shared<UdpBundleSink>(m_ioService, m_incomingRtpStreamPort, 
        boost::bind(&BpSendStream::WholeBundleReadyCallback, this, boost::placeholders::_1),
        numCircularBufferVectors, 
        maxIncomingUdpPacketSizeBytes, 
        boost::bind(&BpSendStream::DeleteCallback, this));
    m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioService));
    ThreadNamer::SetIoServiceThreadName(m_ioService, "ioServiceBpUdpSink");
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
    // copy out bundle to local vector for processing
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

    while (m_running) {
        if (m_incomingPacketQueue.size() > 0) {
            // process incoming data
            m_incomingDtnRtpPtr->PacketHandler(m_incomingPacketQueue.front());
            

            m_incomingPacketQueue.pop();
        } else {
            // boost::this_thread::sleep_for(boost::chrono::microseconds(1));
        }
    }

    
}

void BpSendStream::DeleteCallback()
{
    
}


uint64_t BpSendStream::GetNextPayloadLength_Step1() 
{
    // if (m_outgoin)
    return UINT64_MAX;
}

bool BpSendStream::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    return false;
}
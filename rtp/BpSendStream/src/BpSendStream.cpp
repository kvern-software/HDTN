#include "BpSendStream.h"

BpSendStream::BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, uint64_t numCircularBufferVectors, size_t maxOutgoingUdpPacketSize) : BpSourcePattern(),
    m_running(true),
    m_incomingRtpStreamPort(incomingRtpStreamPort),
    m_maxIncomingUdpPacketSizeBytes(maxIncomingUdpPacketSizeBytes)
{
    InitializeUdpSink(&m_ioService, 
            m_incomingRtpStreamPort, 
            boost::bind(&BpSendStream::WholeBundleReadyCallback, this, boost::placeholders::_1),
            numCircularBufferVectors,
            maxIncomingUdpPacketSizeBytes,
            boost::bind(&BpSendStream::DeleteCallback, this));

}

BpSendStream::~BpSendStream()
{
}

int BpSendStream::InitializeUdpSink(boost::asio::io_service  * ioService, uint16_t udpPort, 
        const WholeBundleReadyCallbackUdp_t &wholeBundleReadyCallback,
        const unsigned int numCircularBufferVectors, const unsigned int maxUdpPacketSizeBytes, 
        const NotifyReadyToDeleteCallback_t &notifyReadyToDeleteCallback)
{
    // processing will begin almost immediately, so call this when our callback and rtp session objects are already initialized
    m_bundleSink = std::make_shared<UdpBundleSink>(*ioService, udpPort, wholeBundleReadyCallback,
        numCircularBufferVectors, 
        maxUdpPacketSizeBytes, notifyReadyToDeleteCallback);

    if (m_bundleSink == nullptr)
        return -1;
    
    return 0;
}

/**
 * DtnRtp objects keep track of the RTP related paremeters such as sequence number and stream identifiers. 
 * The information in the header can be used to enhance audio/video (AV) playback.
 * Here, we have a queue for the incoming and outgoing RTP streams. 
 * BpSendStream has the ability to reduce RTP related overhead by concatenating RTP 
 * packets. The concatenation of these packets follows the guidelines presented in the CCSDS 
 * red book "SPECIFICATION FOR RTP AS TRANSPORT FOR AUDIO AND VIDEO OVER DTN CCSDS 766.3-R-1"
*/
void BpSendStream::InitializeDtnRtp(rtp_format_t fmt, std::shared_ptr<std::atomic<std::uint32_t>> ssrc, size_t rtp_mtu)
{
    m_incomingDtnRtp = std::make_shared<DtnRtp>(fmt, ssrc, UINT64_MAX);
    m_outgoingDtnRtp = std::make_shared<DtnRtp>(fmt, ssrc, UINT64_MAX);
}


void BpSendStream::WholeBundleReadyCallback(padded_vector_uint8_t &wholeBundleVec)
{
    // copy out bundle to local vector for processing
    // std::move(m_incomingPacketQueue.)
    m_incomingPacketQueue.emplace_back(5);
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
    while (m_running)
    {



    }

    
    Stop(); // stop BpSourcePattern
}

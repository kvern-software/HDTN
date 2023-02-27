#pragma once

#include "UdpBundleSink.h"
#include "app_patterns/BpSourcePattern.h"
#include "Rtp.h"
class BpSendStream : public BpSourcePattern
{
public:
    typedef boost::function<void(padded_vector_uint8_t & wholeBundleVec)> WholeBundleReadyCallbackUdp_t;
    typedef boost::function<void()> NotifyReadyToDeleteCallback_t;


    BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, uint64_t numCircularBufferVectors, size_t maxOutgoingUdpPacketSize);
    ~BpSendStream();

    boost::asio::io_service m_ioService; // socket uses this to grab data from incoming rtp stream

    int InitializeUdpSink(boost::asio::io_service * ioService,
        uint16_t udpPort,
        const WholeBundleReadyCallbackUdp_t & wholeBundleReadyCallback,
        const unsigned int numCircularBufferVectors,
        const unsigned int maxUdpPacketSizeBytes,
        const NotifyReadyToDeleteCallback_t & notifyReadyToDeleteCallback);

    void InitializeDtnRtp(rtp_format_t fmt, 
            std::shared_ptr<std::atomic<std::uint32_t>> ssrc, 
            size_t rtp_mtu);

    void ProcessIncomingBundlesThread();

    void WholeBundleReadyCallback(padded_vector_uint8_t & wholeBundleVec); // incoming udp packets come in here
    void DeleteCallback(); // gets called on socket shutdow, optional to do anything with it

    std::shared_ptr<UdpBundleSink> m_bundleSink; 
    std::shared_ptr<DtnRtp> m_outgoingDtnRtp;
    std::shared_ptr<DtnRtp> m_incomingDtnRtp;

    padded_vector_uint8_t m_incomingPacketQueue;
    
protected:
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;

private:
    uint64_t m_bundleSizeBytes;
    uint64_t m_bpGenSequenceNumber;
    volatile bool m_running;

    uint64_t m_incomingRtpStreamPort;
    uint64_t m_maxIncomingUdpPacketSizeBytes; // passed in via config file, should be greater than or equal to the RTP stream source maximum packet size

    // use the same ssrc for incoming and going, assume we only have 1 BpSendStream per media source per CCSDS
    // the ssrc is assigned when the first UDP packet arrives to the UdpHandler. If m_ssrc is unassigned, it gets assigned. 
    // if the incoming ssrc != assigned ssrc, then the packet is discarded. 
    std::shared_ptr<std::atomic<std::uint32_t>> m_ssrc;
};

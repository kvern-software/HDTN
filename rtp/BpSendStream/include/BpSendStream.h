#pragma once

#include "UdpBundleSink.h"
#include "app_patterns/BpSourcePattern.h"
#include "DtnRtp.h"

typedef boost::function<void(padded_vector_uint8_t & wholeBundleVec)> WholeBundleReadyCallbackUdp_t;
typedef boost::function<void()> NotifyReadyToDeleteCallback_t;

class BpSendStream : public BpSourcePattern
{
public:

    BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, size_t numCircularBufferVectors, size_t maxOutgoingBundleSizeBytes);
    ~BpSendStream();


    void ProcessIncomingBundlesThread(); // worker thread that calls RTP packet handler

    void WholeBundleReadyCallback(padded_vector_uint8_t & wholeBundleVec); // incoming udp packets come in here
    void DeleteCallback(); // gets called on socket shutdow, optional to do anything with it

    boost::asio::io_service m_ioService; // socket uses this to grab data from incoming rtp stream
    
    std::shared_ptr<UdpBundleSink> m_bundleSinkPtr; 
    std::shared_ptr<DtnRtp> m_outgoingDtnRtpPtr;
    std::shared_ptr<DtnRtp> m_incomingDtnRtpPtr;

    std::queue<padded_vector_uint8_t> m_incomingPacketQueue;
    
protected:
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;

private:
    uint64_t m_maxOutgoingBundleSizeBytes;
    uint64_t m_bpGenSequenceNumber;
    volatile bool m_running;

    uint64_t m_incomingRtpStreamPort;
    uint64_t m_maxIncomingUdpPacketSizeBytes; // passed in via config file, should be greater than or equal to the RTP stream source maximum packet size




    std::unique_ptr<boost::thread> m_processingThread;
    std::unique_ptr<boost::thread> m_ioServiceThreadPtr;
};

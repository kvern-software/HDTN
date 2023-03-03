#pragma once

#include "UdpBundleSink.h"
#include "app_patterns/BpSourcePattern.h"
#include "DtnRtp.h"

typedef boost::function<void(padded_vector_uint8_t & wholeBundleVec)> WholeBundleReadyCallbackUdp_t;
typedef boost::function<void()> NotifyReadyToDeleteCallback_t;

class BpSendStream : public BpSourcePattern
{
public:

    BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, size_t numCircularBufferVectors, size_t maxOutgoingBundleSizeBytes,  uint64_t numFifoBuffers);
    ~BpSendStream();


    void ProcessIncomingBundlesThread(); // worker thread that calls RTP packet handler

    void WholeBundleReadyCallback(padded_vector_uint8_t & wholeBundleVec); // incoming udp packets come in here
    void DeleteCallback(); // gets called on socket shutdow, optional to do anything with it
    void Concatenate(padded_vector_uint8_t &incomingRtpFrame, std::vector<uint8_t> &currentFrame, size_t &offset);
    void CreateFrame(std::vector<uint8_t> & currentFrame, size_t &offset);
    void PushFrame(std::vector<uint8_t> & currentFrame, size_t &offset);

    boost::asio::io_service m_ioService; // socket uses this to grab data from incoming rtp stream
    
    std::shared_ptr<UdpBundleSink> m_bundleSinkPtr; 
    std::shared_ptr<DtnRtp> m_outgoingDtnRtpPtr;
    std::shared_ptr<DtnRtp> m_incomingDtnRtpPtr;

    std::queue<padded_vector_uint8_t> m_incomingPacketQueue; // consider making this a pre allocated vector
    std::queue<std::vector<uint8_t>> m_OutgoingPacketQueue;

protected:
    virtual bool TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) override;
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;

private:
    bool GetNextQueueTimeout(const boost::posix_time::time_duration& timeout);

    volatile bool m_running;

    uint64_t m_maxIncomingUdpPacketSizeBytes; // passed in via config file, should be greater than or equal to the RTP stream source maximum packet size
    uint64_t m_incomingRtpStreamPort;
    uint64_t m_maxOutgoingBundleSizeBytes;
    uint64_t m_bpGenSequenceNumber;


    boost::mutex m_queueMutex;     
    boost::condition_variable m_queueCv;
    
    std::unique_ptr<boost::thread> m_processingThread;
    std::unique_ptr<boost::thread> m_ioServiceThreadPtr;
};

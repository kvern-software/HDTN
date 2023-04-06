#pragma once

#include "UdpBundleSink.h"
#include "app_patterns/BpSourcePattern.h"
#include "DtnRtp.h"

// typedef boost::function<void(padded_vector_uint8_t & wholeBundleVec)> WholeBundleReadyCallbackUdp_t;
// typedef boost::function<void()> NotifyReadyToDeleteCallback_t;

class BpSendStream : public BpSourcePattern
{
public:

    BpSendStream(size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, 
            size_t numCircularBufferVectors, size_t maxOutgoingBundleSizeBytes, bool enableRtpConcatentation, std::string sdpFile,  uint64_t sdpInterval_ms);
    ~BpSendStream();

    void ProcessIncomingBundlesThread(); // worker thread that calls RTP packet handler
    void WholeBundleReadyCallback(padded_vector_uint8_t & wholeBundleVec); // incoming udp packets come in here
    void DeleteCallback(); // gets called on socket shutdow, optional to do anything with it
    void Concatenate(padded_vector_uint8_t &incomingRtpFrame);
    void CreateFrame();
    void PushFrame();
 
    boost::asio::io_service m_ioService; // socket uses this to grab data from incoming rtp stream
    
    std::shared_ptr<UdpBundleSink> m_bundleSinkPtr; 
    std::shared_ptr<DtnRtp> m_outgoingDtnRtpPtr;
    std::shared_ptr<DtnRtp> m_incomingDtnRtpPtr;

    boost::circular_buffer<padded_vector_uint8_t> m_incomingCircularPacketQueue; // consider making this a pre allocated vector
    boost::circular_buffer<padded_vector_uint8_t> m_outgoingCircularFrameQueue;

protected:
    virtual bool TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) override;
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;

private:
    bool TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout);
    bool SdpTimerThread();
    bool GetNextIncomingPacketTimeout(const boost::posix_time::time_duration& timeout);
    bool GetNextOutgoingPacketTimeout(const boost::posix_time::time_duration& timeout);

    padded_vector_uint8_t m_currentFrame;  
    size_t m_offset = 0;

    volatile bool m_running;
    
    uint64_t m_maxIncomingUdpPacketSizeBytes; // passed in via config file, should be greater than or equal to the RTP stream source maximum packet size
    uint64_t m_incomingRtpStreamPort;
    uint64_t m_maxOutgoingBundleSizeBytes;
    uint64_t m_bpGenSequenceNumber;

    bool m_enableRtpConcatentation;

    boost::mutex m_outgoingQueueMutex;   
    boost::mutex m_incomingQueueMutex;     
    boost::mutex m_sdpMutex;

    boost::condition_variable m_outgoingQueueCv;
    boost::condition_variable m_incomingQueueCv;
    boost::condition_variable m_sdpCv;

    std::unique_ptr<boost::thread> m_processingThread;
    std::unique_ptr<boost::thread> m_ioServiceThreadPtr;
    std::unique_ptr<boost::thread> m_sdpThread;

    bool m_sendSdp = true;
    std::string m_sdpFileStr;
    uint64_t m_sdpInterval_ms;
    

    uint64_t m_totalRtpPacketsReceived = 0; // counted when received from udp sink
    uint64_t m_totalRtpPacketsSent = 0; // counted when send to bundler
    uint64_t m_totalRtpPacketsQueued = 0; // counted when pushed into outgoing queue
    uint64_t m_totalConcatenationsPerformed = 0; // counted when a packet is successfully concatenated 
    uint64_t m_totalMarkerBits = 0;
    uint64_t m_totalTimestampChanged = 0;
    uint64_t m_totalIncomingCbOverruns = 0;
    uint64_t m_totalOutgoingCbOverruns = 0;

};


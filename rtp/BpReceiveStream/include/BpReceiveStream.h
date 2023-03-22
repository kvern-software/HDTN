#pragma once

#include "app_patterns/BpSinkPattern.h"
#include "UdpBatchSender.h"

#include "DtnRtp.h"

class BpReceiveStream : public BpSinkPattern {
public:
    BpReceiveStream(size_t numCircularBufferVectors, const std::string& remoteHostname, const uint16_t remotePort, uint16_t maxOutgoingRtpPacketSizeByte, std::string ffmpegCommand);
    virtual ~BpReceiveStream() override;
    void ProcessIncomingBundlesThread(); // worker thread 

protected:
    virtual bool ProcessPayload(const uint8_t * data, const uint64_t size) override;
    
private:
    int ReadSdpFileFromString(std::string sdpFileString);
    int ExecuteFFmpegInstance();

    bool TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout);
    bool GetNextIncomingPacketTimeout(const boost::posix_time::time_duration& timeout);

    bool TryWaitForSuccessfulSend(const boost::posix_time::time_duration &timeout);
    bool GetSuccessfulSendTimeout(const boost::posix_time::time_duration &timeout);

    void OnSentRtpPacketCallback(bool success, std::shared_ptr<std::vector<UdpSendPacketInfo> >& udpSendPacketInfoVecSharedPtr, const std::size_t numPacketsSent);


    int SendUdpPacket(const std::vector<uint8_t>& message);
    volatile bool m_running; // exit condition
    
    boost::circular_buffer<padded_vector_uint8_t> m_incomingCircularPacketQueue; // incoming rtp frames from HDTN put here
    std::shared_ptr<DtnRtp> m_outgoingDtnRtpPtr; // export our rtp frames using this object
    
    // inbound config
    size_t m_numCircularBufferVectors;

    // outbound config
    uint16_t m_outgoingRtpPort;
    uint16_t m_maxOutgoingRtpPacketSizeBytes;
    uint16_t m_maxOutgoingRtpPayloadSizeBytes;
    std::string m_sdpFileString;
    // outbound udp
	boost::asio::io_service io_service;
    boost::asio::ip::udp::socket socket;
    std::string m_ffmpegCommand;
    
    boost::asio::ip::udp::endpoint m_udpEndpoint;
    std::shared_ptr<UdpBatchSender> m_udpBatchSenderPtr;
    boost::mutex m_sentPacketsMutex;
    boost::condition_variable m_cvSentPacket; // notify when UdpBatchSender has sent out our RTP frames to network
    volatile bool m_sentPacketsSuccess;

    // multithreading 
    boost::condition_variable m_incomingQueueCv;
    boost::mutex m_incomingQueueMutex;     
    std::unique_ptr<boost::thread> m_processingThread;

    
    
    // book keeping
    uint64_t m_totalRtpPacketsReceived = 0; 
    uint64_t m_totalRtpPacketsSent = 0; 
    uint64_t m_totalRtpPacketsFailedToSend = 0;
    uint64_t m_totalRtpBytesSent = 0;
    uint64_t m_totalRtpPacketsQueued = 0;


};

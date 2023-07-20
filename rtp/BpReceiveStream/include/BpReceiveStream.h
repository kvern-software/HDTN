#pragma once

#include "app_patterns/BpSinkPattern.h"
#include "UdpBatchSender.h"

#include "DtnRtp.h"

#include "GStreamerAppSrcOutduct.h"

typedef enum {
    UDP_OUTDUCT = 0,
    GSTREAMER_APPSRC_OUTDUCT = 1
} BpRecvStreamOutductTypes;


struct bp_recv_stream_params_t {
    std::string rtpDestHostname;
    uint16_t rtpDestPort;
    uint16_t maxOutgoingRtpPacketSizeBytes;
    uint8_t outductType;
    std::string shmSocketPath;
    std::string gstCaps;
};

class BpReceiveStream : public BpSinkPattern {
public:
    BpReceiveStream(size_t numCircularBufferVectors, bp_recv_stream_params_t params);
    virtual ~BpReceiveStream() override;

protected:
    virtual bool ProcessPayload(const uint8_t * data, const uint64_t size) override;
    
private:
    void ProcessIncomingBundlesThread(); // worker thread 

    // int TranslateBpSdpToInSdp(std::string sdp);

    bool TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout);
    bool GetNextIncomingPacketTimeout(const boost::posix_time::time_duration& timeout);

    bool TryWaitForSuccessfulSend(const boost::posix_time::time_duration &timeout);
    bool GetSuccessfulSendTimeout(const boost::posix_time::time_duration &timeout);

    void OnSentRtpPacketCallback(bool success, std::shared_ptr<std::vector<UdpSendPacketInfo> >& udpSendPacketInfoVecSharedPtr, const std::size_t numPacketsSent);
    int SendUdpPacket(padded_vector_uint8_t & message);

    volatile bool m_running; // exit condition
    
    // inbound config
    boost::circular_buffer<padded_vector_uint8_t> m_incomingBundleQueue; // incoming rtp frames from HDTN put here
    size_t m_numCircularBufferVectors;

    // outbound config
    std::string m_outgoingRtpHostname;
    uint16_t m_outgoingRtpPort;
    uint16_t m_maxOutgoingRtpPacketSizeBytes;
    uint16_t m_maxOutgoingRtpPayloadSizeBytes;

    // outbound udp outduct
	boost::asio::io_service io_service;
    boost::asio::ip::udp::socket socket;
    boost::asio::ip::udp::endpoint m_udpEndpoint;
    std::shared_ptr<UdpBatchSender> m_udpBatchSenderPtr;
    boost::mutex m_sentPacketsMutex;
    boost::condition_variable m_cvSentPacket; // notify when UdpBatchSender has sent out our RTP frames to network
    volatile bool m_sentPacketsSuccess;

    // outbound gstreamer outduct
    uint8_t m_outductType;
    std::unique_ptr<GStreamerAppSrcOutduct> m_gstreamerAppSrcOutductPtr;

    // multithreading 
    boost::condition_variable m_incomingQueueCv;
    boost::mutex m_incomingQueueMutex;     
    std::unique_ptr<boost::thread> m_processingThread;

    // book keeping
    uint64_t m_totalRtpPacketsReceived = 0; 
    uint64_t m_totalRtpPacketsSent = 0; 
    uint64_t m_totalRtpPacketsFailedToSend = 0;
    uint64_t m_totalRtpBytesSent = 0;
};

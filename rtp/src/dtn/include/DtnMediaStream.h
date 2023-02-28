// Enables DTN compatibility with the uvgRTP media_stream class

#pragma once


#include "../../video_driver/VideoDriver.h"

#include "DtnEncoder.h"
#include "DtnMedia.h"
#include "DtnUtil.h"
#include "DtnRtp.h"
#include "DtnFrameQueue.h"

#include <memory>


/**
 * A DtnMediaStream is an RTP stream over a DTN using HDTN.
 * The DtnMediaStream realizes the functionality provided by
 * the Real Time Protocol. It can 
 *  - receive information about the stream (RTCP)
 *  - send data to be streamed (to bundler) via an autmatically exported frame_queue
 *  - receive streamed data (unbundled) and queue for playback
 * 
 * 
 * 
 * 
*/
class DtnMediaStream
{
private:
    std::string m_cname;
    size_t m_rtpMTU; // max transmission unit of the RTP packets themselves. Determines concatenation of frames
    rtp_format_t m_fmt;

    // int m_fpsNumerator, m_fpsDenominator;

    // std::unique_ptr<DtnMedia> m_media; // media type (H264, H265, ...)
    bool m_initialized;

    // networking
    std::string m_remoteAddress; // this is the ip address we want to send to
    std::string m_localAddress; // we get information from rtcp from this address. note, the information is provided by a HDTN component, not by binding to a socket
    uint16_t m_remotePort; // destination port
    uint16_t m_sourcePort; // not sending from this socket, handled by HDTN



    // These objects handle the background tasks of the media stream such as receiving stream information, receiving unbundled frames, tracking the rtp variables
    std::shared_ptr<DtnRtp> m_DtnRtp; // this keeps track of all the pertinent information provided in an rtp frame

    // frame queues
    size_t m_frameQueueSize; // this is the max number of frames in queue, enforced by this object
    std::shared_ptr<DtnFrameQueue> m_outgoingFrameQueue = nullptr;
    std::shared_ptr<DtnFrameQueue> m_IncomingFrameQueue = nullptr;

    std::shared_ptr<std::atomic<uint32_t>> m_ssrc;

    boost::mutex m_queueMutex;

    rtp_modes_t m_operating_mode;

    std::unique_ptr<boost::thread> m_ProcessingPayloadThread;



public:
    DtnMediaStream(std::string cname, rtp_modes_t operating_mode);

    ~DtnMediaStream();
    boost::mutex * GetMutex();

    // void RtcpRunner(uint8_t *buffer, size_t size); 

    // configure the stream 
    int Init(rtp_format_t fmt, // int fps_numerator, int fps_denominator, 
            size_t frameQueueSize, std::string localAddress, std::string remoteAddress, uint16_t srcPort, uint16_t remotePort,
            size_t rtp_mtu);
        
    

    void PushFrame(buffer * img_buffer); // push video data to be queued. register this as callback for and video drivers. makes copy out
    void ReceivePayload(const uint8_t * data, const uint64_t size);
    void PayloadProcessor();

    std::shared_ptr<DtnFrameQueue> GetOutgoingFrameQueuePtr();
    std::shared_ptr<DtnFrameQueue> GetIncomingFrameQueuePtr();
    std::shared_ptr<DtnRtp> GetDtnRtpPtr();
    size_t GetFrameQueueSize();

};

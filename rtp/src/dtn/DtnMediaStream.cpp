#include "DtnMediaStream.h"
#include "Logger.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

static std::deque<buffer> payloadBuffers;

DtnMediaStream::DtnMediaStream(std::string cname, rtp_modes_t operating_mode) :
    m_cname(cname),
    m_operating_mode(operating_mode)
{
    
};

DtnMediaStream::~DtnMediaStream()
{
    m_queueMutex.unlock();
};

int DtnMediaStream::Init(rtp_format_t fmt, //int fps_numerator, //int fps_denominator, 
            size_t frameQueueSize, std::string localAddress, std::string remoteAddress, uint16_t srcPort, uint16_t remotePort,
            size_t rtp_mtu)
{
    m_fmt = fmt; 
    // m_fpsNumerator = fps_numerator;
    // m_fpsDenominator = fps_denominator;
    m_frameQueueSize = frameQueueSize;
    m_ssrc = std::make_shared<std::atomic<uint32_t>>(GenRandom());
    m_remoteAddress = remoteAddress;
    m_localAddress = localAddress;
    m_remotePort = remotePort;
    m_sourcePort = srcPort;
    m_rtpMTU = rtp_mtu;

    m_DtnRtp = std::make_shared<DtnRtp>(m_rtpMTU);

    m_outgoingFrameQueue = std::make_shared<DtnFrameQueue>(m_frameQueueSize);
    m_IncomingFrameQueue = std::make_shared<DtnFrameQueue>(m_frameQueueSize);

    if (m_operating_mode == RTP_SEND_RECV)
    {
        LOG_INFO(subprocess) << "Starting bi directional RTP stream with frame queue of size " << m_frameQueueSize;
        m_IncomingFrameQueue = nullptr;
        return 0;
    }

    if (m_operating_mode == RTP_RECV_ONLY)
    {
        LOG_INFO(subprocess) << "Starting unidirectional RTP receiving stream with frame queue of size " << m_frameQueueSize;
        m_outgoingFrameQueue = nullptr;
        LOG_INFO(subprocess) << "Starting payload processing thread" << m_frameQueueSize;
        m_ProcessingPayloadThread = boost::make_unique<boost::thread>(
                boost::bind(&DtnMediaStream::PayloadProcessor, this)); //create and start the worker thread
    }
    
    if (m_operating_mode == RTP_SEND_ONLY)
    {
        LOG_INFO(subprocess) << "Starting unidirectional RTP sending stream with frame queue of size " << m_frameQueueSize;
    }

    return 0;
}

boost::mutex * DtnMediaStream::GetMutex()
{
    return &m_queueMutex;
}

void DtnMediaStream::PushFrame(buffer * img_buffer)
{
// LOG_INFO(subprocess) << "push lock";
    m_queueMutex.lock();
    
    rtp_frame frame;

    m_DtnRtp->FillHeader(&frame);

    m_outgoingFrameQueue->PushFrame(img_buffer, &frame); // this should now hold a queue of rtp packets, not just raw frames
    
    m_DtnRtp->IncSentPkts(); // note that the dtnRtp packets is not necessarily the number of packets sent over the line, just the number of packets sent through the object to be put into frames (for internal stats)
    // m_DtnRtp->IncSequence(); // ONLY INCREMENT SEQUENCE FOR PACKETS WE SEND

    // LOG_INFO(subprocess) << "Pushed frame";
    m_queueMutex.unlock();
// LOG_INFO(subprocess) << "unlocked";

}


/**
 * Use this function to push data from the sink to our INcoming frame queue.
*/
void DtnMediaStream::ReceivePayload(const uint8_t * data, const uint64_t size)
{
    payloadBuffers.emplace_back();
    payloadBuffers.back().allocate(size);
    payloadBuffers.back().copy((void * ) data);
}

std::shared_ptr<DtnFrameQueue> DtnMediaStream::GetOutgoingFrameQueuePtr()
{
    return m_outgoingFrameQueue;
}
std::shared_ptr<DtnFrameQueue> DtnMediaStream::GetIncomingFrameQueuePtr()
{
    return m_IncomingFrameQueue;
}

size_t DtnMediaStream::GetFrameQueueSize()
{
    return m_frameQueueSize;
}

std::shared_ptr<DtnRtp> DtnMediaStream::GetDtnRtpPtr()
{
    return m_DtnRtp;
}

/**
 * This function handles the conversion from raw payload to rtp frames
 * It is possible, and likely, that there is only one rtp frame per packet
*/
void DtnMediaStream::PayloadProcessor()
{
    LOG_DEBUG(subprocess) << "BEGIN PROCESSING THREAD";

    while (1)
    {
        if (payloadBuffers.size() > 0)
        {
            // LOG_DEBUG(subprocess) << "Got buffer";

            // process rtp packet
            m_DtnRtp->PacketHandler(payloadBuffers.front().length, payloadBuffers.front().start, 0, m_IncomingFrameQueue);            

            payloadBuffers.pop_front();
        } else {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            // LOG_DEBUG(subprocess) << "Waiting...";
        }
    


    }

    // begine processing 
}
#include "DtnMediaStream.h"
#include "Logger.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

DtnMediaStream::DtnMediaStream(std::string cname) :
    m_cname(cname)
{
};

DtnMediaStream::~DtnMediaStream()
{
};

int DtnMediaStream::Init(rtp_format_t fmt, int fps_numerator, int fps_denominator, 
            size_t frameQueueSize, std::string localAddress, std::string remoteAddress, uint16_t srcPort, uint16_t remotePort)
{
    m_fmt = fmt; 
    m_fpsNumerator = fps_numerator;
    m_fpsDenominator = fps_denominator;
    m_frameQueueSize = frameQueueSize;
    // m_ssrc(std::make_shared<std::atomic<uint32_t>>(GenRandom())),
    m_remoteAddress = remoteAddress;
    m_localAddress = localAddress;
    m_remotePort = remotePort;
    m_sourcePort = srcPort;

    m_outgoingFrameQueue = std::make_shared<DtnFrameQueue>(m_frameQueueSize); // TODO determine if we need outgoing and incoming queues
    // m_IncomingFrameQueue = nullptr;
    return 0;
}


void DtnMediaStream::PushFrame(buffer * img_buffer)
{
    m_outgoingFrameQueue->PushFrame(img_buffer);
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
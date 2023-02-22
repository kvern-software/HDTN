#include "MediaSink.h"
#include "Logger.h"

#include "RtpFrame.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

MediaSink::MediaSink(std::shared_ptr<DtnMediaStream> dtnMediaStream) : BpSinkPattern(),
    m_DtnMediaStreamPtr(dtnMediaStream)
{
}

 
MediaSink::~MediaSink()
{

}

bool MediaSink::ProcessPayload(const uint8_t * data, const uint64_t size)
{
    // LOG_INFO(subprocess) << "PROCESS PAYLOAD";
    m_DtnMediaStreamPtr->ReceivePayload(data, size);
    return true;
}

// void  MediaSink::RegisterCallback(const ExportFrameCallback_t & exportDataCallback)
// {
    // m_exportDataCallback = exportDataCallback;
// }

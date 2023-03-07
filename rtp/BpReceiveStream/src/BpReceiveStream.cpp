#include "BpReceiveStream.h"
#include "Logger.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpReceiveStream::BpReceiveStream(uint16_t outgoingRtpPort)
{
}

BpReceiveStream::~BpReceiveStream()
{
    
}

bool BpReceiveStream::ProcessPayload(const uint8_t *data, const uint64_t size)
{

    LOG_DEBUG(subprocess) << "Got bundle of size " << size;

    rtp_frame * frame= (rtp_frame *) data;

    frame->print_header();


    return false;
}

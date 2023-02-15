#include <string.h>
#include <iostream>
#include "MediaSource.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

struct bpgen_hdr {
    uint64_t seq;
    uint64_t tsc;
    timespec abstime;
};

MediaSource::MediaSource(uint64_t bundleSizeBytes) :
    BpSourcePattern(),
    m_bundleSizeBytes(bundleSizeBytes),
    m_bpGenSequenceNumber(0)
{
    mediaAppPtr = std::make_shared<MediaApp>();
    videoDriverPtr = std::make_shared<VideoDriver>(boost::bind(&MediaSource::ExportFrame, this, boost::placeholders::_1));
    fragmenterPtr = std::make_shared<Fragmenter>(bundleSizeBytes);
    compressorPtr = std::make_shared<Compressor>(); 
}

MediaSource::~MediaSource() {}

void MediaSource::ExportFrame(buffer *buf) {
    rawFrameBuffer.start = malloc(buf->length);
    rawFrameBuffer.length = buf->length;
    memcpy(rawFrameBuffer.start, buf->start, rawFrameBuffer.length);
     
    // this is using a fifo policy, maybe change this later?
    compressedFrameBuffer.length = compressorPtr->CalculateCompressBound(rawFrameBuffer.length); // allocate memory in destination first
    compressedFrameBuffer.start = malloc(compressedFrameBuffer.length);

    compressorPtr->Compress((unsigned char*)compressedFrameBuffer.start, &compressedFrameBuffer.length, (unsigned char*)rawFrameBuffer.start, rawFrameBuffer.length); // compress here
    // LOG_INFO(subprocess) << "Uncompressed size in bytes: " << rawFrameBuffer.length;
    // LOG_INFO(subprocess) << "Compressed size in bytes: " << compressedFrameBuffer.length;

    fragmenterPtr->Fragment(&compressedFrameBuffer, rawFrameBuffer.length); // framents location given 
}

uint64_t MediaSource::GetNextPayloadLength_Step1() {
    if (fragmenterPtr->HasFragments()) {
        std::cout << "got size " << fragmenterPtr->GetNextFragmentSize() << std::endl;

        return fragmenterPtr->GetNextFragmentSize();
        // return UINT64_MAX;
    } 

    return UINT64_MAX;
}

bool MediaSource::CopyPayload_Step2(uint8_t * destinationBuffer) {
    memcpy(destinationBuffer, fragmenterPtr->GetNextFragment(), fragmenterPtr->GetNextFragmentSize());
    fragmenterPtr->PopFragment();
    std::cout << "copied" << std::endl;

    return true;
}

bool MediaSource::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) {

    std::cout << "waited" << std::endl;
    
    boost::this_thread::sleep(timeout);
    return 0;
}
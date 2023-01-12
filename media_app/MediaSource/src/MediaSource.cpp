#include <string.h>
#include <iostream>
#include "MediaSource.h"

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

}

MediaSource::~MediaSource() {}


uint64_t MediaSource::GetNextPayloadLength_Step1() {
    if (video_driver_enabled && videoDriver.image_buffers[0].start != nullptr) {
        std::cout << videoDriver.image_buffers[0].length << std::endl;
        return videoDriver.image_buffers[0].length;
    } else {
        return 0;
    }
}

bool MediaSource::CopyPayload_Step2(uint8_t * destinationBuffer) {
    // bpgen_hdr bpGenHeader;
    // bpGenHeader.seq = m_bpGenSequenceNumber++;
    if (video_driver_enabled && mediaApp.rawFrameBuffer.location != nullptr)  {
        memcpy(destinationBuffer, mediaApp.rawFrameBuffer.location, mediaApp.rawFrameBuffer.size);
        std::cout << "copied" << std::endl;
        return true;
    } 

    return true;
}

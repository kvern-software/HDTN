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

    if (video_driver_enabled) {
        videoDriver.QueueBuffer();  
        videoDriver.DequeueBuffer();
        std::cout << videoDriver.bufferinfo.bytesused << std::endl;
        return videoDriver.bufferinfo.bytesused;
    } else {
        return m_bundleSizeBytes;
    }
}

bool MediaSource::CopyPayload_Step2(uint8_t * destinationBuffer) {
    // bpgen_hdr bpGenHeader;
    // bpGenHeader.seq = m_bpGenSequenceNumber++;
    memcpy(destinationBuffer, videoDriver.image_data, videoDriver.bufferinfo.bytesused);
    std::cout << "copied" << std::endl;
    return true;
}

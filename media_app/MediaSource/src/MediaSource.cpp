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
    return m_bundleSizeBytes;
}

bool MediaSource::CopyPayload_Step2(uint8_t * destinationBuffer) {
    bpgen_hdr bpGenHeader;
    bpGenHeader.seq = m_bpGenSequenceNumber++;
    memcpy(destinationBuffer, &bpGenHeader, sizeof(bpgen_hdr));
    return true;
}

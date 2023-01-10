#pragma once
// #include "codec/Cbhe.h"
// #include "LtpFragmentSet.h"

#include "app_patterns/BpSourcePattern.h"
#include "VideoDriver.h"
#include "MediaApp.h"

class MediaSource : public BpSourcePattern
{
private:
    MediaSource();
public:
    MediaSource(uint64_t bundleSizeBytes);
    virtual ~MediaSource() override;
    
    MediaApp mediaApp;
    VideoDriver videoDriver;

    uint64_t file_number=0;
    std::string saveFileFullFilename;
    bool video_driver_enabled;
protected:
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;
private:
    uint64_t m_bundleSizeBytes;
    uint64_t m_bpGenSequenceNumber;
};




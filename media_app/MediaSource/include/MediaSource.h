#pragma once

#include "app_patterns/BpSourcePattern.h"
#include "VideoDriver.h"
#include "MediaApp.h"
#include "Fragmenter.h"
#include "Compressor.h"
#include "Logger.h"

class MediaSource : public BpSourcePattern
{
private:
    MediaSource();
public:
    MediaSource(uint64_t bundleSizeBytes);
    virtual ~MediaSource() override;
    
    void ExportFrame(buffer * buf);

    std::shared_ptr<MediaApp> mediaAppPtr;
    std::shared_ptr<VideoDriver> videoDriverPtr;
    std::shared_ptr<Fragmenter> fragmenterPtr;
    std::shared_ptr<Compressor> compressorPtr;

    buffer rawFrameBuffer;
    buffer compressedFrameBuffer;

    uint64_t file_number=0;
    std::string saveFileFullFilename;
    bool video_driver_enabled;
protected:
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;
    virtual bool TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) override;
private:
    uint64_t m_bundleSizeBytes;
    uint64_t m_bpGenSequenceNumber;
};



// Enables DTN compatibility with the uvgRTP media_stream class

#pragma once

#include "app_patterns/BpSourcePattern.h"

#include "VideoDriver.h"
#include "DtnMedia.h"
#include "DtnEncoder.h"
/**
 * A DtnMediaStream is an RTP stream over a DTN using HDTN.
 * The DtnMediaStream is currently only one directional as it 
 * inherits from BpSourcePattern which is inherently one directional.
 * 
 * 
 * 
*/
class DtnMediaStreamSource : BpSourcePattern
{
private:
    std::string m_cname;
    rtp_format_t m_fmt;
    int m_rceFlags;
    // std::unique_ptr<DtnMedia> m_media; // media type (H264, H265, ...)
    bool m_initialized;
    int m_fpsNumerator, m_fpsDenominator;
    
    size_t m_frameQueueSize;

    bool m_runningFromSigHandler;
    uint32_t m_bundleSizeBytes;

    void CreateVideoDriverPtr();
public:
    DtnMediaStreamSource(std::string cname, rtp_format_t fmt, int rce_flags, int fps_numerator, int fps_denominator, size_t frameQueueSize) :
        BpSourcePattern(),
        m_cname(cname), m_fmt(fmt), m_rceFlags(rce_flags), m_fpsNumerator(fps_numerator), m_fpsDenominator(fps_denominator), m_frameQueueSize(frameQueueSize) {};

    ~DtnMediaStreamSource(){};

    void StartComponents(); // starts frame queue, video driver, encoder
    int Run(int argc, const char* argv[], volatile bool & running, bool useSignalHandler);
    
    std::shared_ptr<DtnFrameQueue> m_dtnFrameQueuePtr;
    std::shared_ptr<VideoDriver> m_videoDriverPtr;
    std::shared_ptr<DtnEncoder> m_dtnEncoderPtr;



protected:
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;
    virtual bool TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) override;
    virtual bool ProcessNonAdminRecordBundlePayload(const uint8_t * data, const uint64_t size) override;
};

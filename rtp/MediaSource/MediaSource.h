#pragma once
 
/**
 * This class inherits from BpSourcePattern. We hook up our DtnMediaStream to the source.
 * The source will bundle and send the data insude the media stream frame buffer when the buffer is
 * filled. 
*/

#include "app_patterns/BpSourcePattern.h"
#include "Logger.h"

#include "DtnMediaStream.h"
#include "DtnRtpFrame.h"

class MediaSource : public BpSourcePattern
{
public:
    MediaSource(uint64_t bundleSizeBytes, std::shared_ptr<DtnMediaStream> dtnMediaStream);
    virtual ~MediaSource() override;
    
    std::shared_ptr<DtnMediaStream> m_DtnMediaStreamPtr;

    uint64_t file_number=0;
    std::string saveFileFullFilename;


protected:
    virtual uint64_t GetNextPayloadLength_Step1() override;
    virtual bool CopyPayload_Step2(uint8_t * destinationBuffer) override;
    virtual bool TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) override;
    virtual bool ProcessNonAdminRecordBundlePayload(const uint8_t * data, const uint64_t size)  override;
private:
    uint64_t m_bundleSizeBytes;
    uint64_t m_bpGenSequenceNumber;

    boost::mutex m_queueMutex;
};


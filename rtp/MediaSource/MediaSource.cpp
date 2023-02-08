#include "MediaSource.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

MediaSource::MediaSource(uint64_t bundleSizeBytes) : BpSourcePattern(), m_bundleSizeBytes(bundleSizeBytes)
{

}

MediaSource::~MediaSource() 
{

}

uint64_t MediaSource::GetNextPayloadLength_Step1() 
{
    LOG_INFO(subprocess) << "payload length step one: " << m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSizeBytes() << std::endl;
    return (uint64_t) m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSizeBytes();
}

bool MediaSource::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    memcpy(destinationBuffer, &m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue(),
            m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSizeBytes());
    LOG_INFO(subprocess) << "copied out";

    // clear queue
    m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->ClearQueue();
    return true;
}

bool MediaSource::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) 
{
    if (m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSize() != m_DtnMediaStreamPtr->GetFrameQueueSize())
    {
        // wait for full queue
        return m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetNextQueueTimeout(timeout);
    } 
    
    // send data, queue is full
    return true;
}

bool MediaSource::ProcessNonAdminRecordBundlePayload(const uint8_t * data, const uint64_t size) 
{
return 0;
}

#include "MediaSource.h"
#include <Logger.h>

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

MediaSource::MediaSource(uint64_t bundleSizeBytes, std::shared_ptr<DtnMediaStream> dtnMediaStreamPtr) : BpSourcePattern(), m_bundleSizeBytes(bundleSizeBytes)
{
    m_DtnMediaStreamPtr = dtnMediaStreamPtr;
}

MediaSource::~MediaSource() 
{

}


uint64_t MediaSource::GetNextPayloadLength_Step1() 
{
    // lock frame queue mutex (GETS UNLOCKED AFTER COPY)
    LOG_INFO(subprocess) << "Copy Lock";
    m_DtnMediaStreamPtr->GetMutex()->lock();


    if (m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSize() != m_DtnMediaStreamPtr->GetFrameQueueSize())
    {
        LOG_INFO(subprocess) << "WAIT FOR PAYLOAD, UNLOCKED";
        m_DtnMediaStreamPtr->GetMutex()->unlock();
        return UINT64_MAX;
    } 
    else 
    {
        LOG_INFO(subprocess) << "payload length step one: " << m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSizeBytes() << std::endl;
        return (uint64_t) m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSizeBytes();

    }
}

bool MediaSource::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    LOG_INFO(subprocess) << "COPYING OUT";
    uint64_t offset = 0;
    for (size_t i=0; i<m_DtnMediaStreamPtr->GetFrameQueueSize(); i++)
    {
        // LOG_INFO(subprocess) << m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSize();

        // since we are actually about to send these rtp frames, we need to increase their sequence number
        m_DtnMediaStreamPtr->GetDtnRtpPtr()->UpdateSequence(&m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front());

        // LOG_INFO(subprocess) << ntohs(m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front().header.seq);
        
        m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front().print_header();

        uint64_t numBytesToCopy =  sizeof(rtp_header) + m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front().payload.length;

        // make copy of each element
        memcpy(destinationBuffer + offset, &m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front().header, sizeof(rtp_header)); // get header
        
        memcpy(destinationBuffer + sizeof(rtp_header) + offset ,  // get payload
                m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front().payload.start,   
                m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front().payload.length);


        offset += numBytesToCopy;

        // rtp_frame tmp_frame;
        // rtp_frame * frame_ptr = (rtp_frame * ) &m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetQueue().front();
        // memcpy(&tmp_frame, frame_ptr, sizeof(rtp_header));

        // rtp_frame tmp_frame;
        // rtp_frame * frame_ptr = (rtp_frame * ) destinationBuffer;
        // memcpy(&tmp_frame, frame_ptr, sizeof(rtp_header));

        // tmp_frame.print_header();
        m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->PopFrame();
        LOG_INFO(subprocess) << "copied out img size " << numBytesToCopy - sizeof(rtp_header) << " TOTAL SIZE " << numBytesToCopy;

    }

    // unlock mutex
    m_DtnMediaStreamPtr->GetMutex()->unlock();
    // LOG_INFO(subprocess) << "copy UNLOCK";
    return true;
}

bool MediaSource::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) 
{
    if (m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetCurrentQueueSize() != m_DtnMediaStreamPtr->GetFrameQueueSize())
    {
        // wait for full queue
        LOG_INFO(subprocess) << "waiting for data";

        return m_DtnMediaStreamPtr->GetOutgoingFrameQueuePtr()->GetNextQueueTimeout(timeout);
    } 
    
    // send data, queue is full
    LOG_INFO(subprocess) << "try wait success";

    return true;
}

bool MediaSource::ProcessNonAdminRecordBundlePayload(const uint8_t * data, const uint64_t size) 
{
return 0;
}

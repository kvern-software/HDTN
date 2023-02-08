#include "DtnFrameQueue.h"
#include "Logger.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

DtnFrameQueue::DtnFrameQueue(size_t queueSize) : m_queueSize(queueSize) 
{
};

DtnFrameQueue::~DtnFrameQueue()
{
};

// get reference to first element in frame queue
buffer& DtnFrameQueue::GetNextFrame() 
{
    return m_frameQueue.front();
}

// pops oldest frame in queue
void DtnFrameQueue::PopFrame()
{
    m_totalBytesInQueue -= m_frameQueue.front().length; 
    m_frameQueue.pop();
}

// adds new frame to end of queue
void DtnFrameQueue::PushFrame(buffer * image_buffer) 
{
    if (m_frameQueue.size() >= m_queueSize)
        PopFrame();
    
    m_frameQueue.push(*image_buffer);
    m_totalBytesInQueue += m_frameQueue.back().length;
}   

size_t DtnFrameQueue::GetCurrentQueueSize()
{
    return m_frameQueue.size();
}

size_t DtnFrameQueue::GetCurrentQueueSizeBytes()
{
    return m_totalBytesInQueue;
}

std::queue<buffer>& DtnFrameQueue::GetQueue() 
{
    return m_frameQueue;
}

void DtnFrameQueue::ClearQueue()
{
    m_frameQueue = std::queue<buffer>();
}

bool DtnFrameQueue::GetNextQueueReady()
{
    if (m_frameQueue.size() != m_queueSize)
        return false;
    
    return true; // if queue has filled to proper size
}

bool DtnFrameQueue::GetNextQueueTimeout(const boost::posix_time::time_duration& timeout)
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    if (!GetNextQueueReady()) {
        m_queueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
    }

    return GetNextQueueReady();
}
#pragma once

#include "VideoDriver.h"
#include <queue>

class DtnFrameQueue
{
private:
    std::queue<buffer> m_frameQueue;
    size_t m_queueSize; // number of rtp packets in queue
    size_t m_totalBytesInQueue = 0; // raw bytes in queue

    boost::mutex m_queueMutex;
    boost::condition_variable m_queueCv;
public:
    DtnFrameQueue(size_t queueSize) : m_queueSize(queueSize) 
    {
    };

    ~DtnFrameQueue(){};

    buffer& GetNextFrame();
    void PopFrame();
    void PushFrame(buffer * image_buffer);
    void ClearQueue();

    size_t GetCurrentQueueSize(); // number of rtp packets 
    size_t GetCurrentQueueSizeBytes(); // number of raw bytes across all packets in queue
    std::queue<buffer>& GetQueue(); // reference to our queue, for copying out

    // for monitoring if queue is full
    bool GetNextQueueReady();
    bool GetNextQueueTimeout(const boost::posix_time::time_duration& timeout);
};



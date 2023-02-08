#pragma once
#include "VideoDriver.h"
#include "DtnFrameQueue.h"

class DtnEncoder
{
private:
    std::shared_ptr<DtnFrameQueue> m_frameQueue;

public:
    DtnEncoder(std::shared_ptr<DtnFrameQueue> frameQueue) :
        m_frameQueue(frameQueue)
        {};

    ~DtnEncoder(){};

    void FrameCallback(buffer * image_buffer);
};



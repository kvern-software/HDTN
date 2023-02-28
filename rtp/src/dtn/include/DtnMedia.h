#pragma once
#include "../../video_driver/VideoDriver.h"
#include "DtnContext.h"

#include "DtnFrameQueue.h"

// class DtnMedia
// {


// private:
//     /* data */
//     std::shared_ptr<DtnContext> m_DtnContext;
//     std::shared_ptr<DtnFrameQueue> m_DtnFrameQueue;

// public:
//     DtnMedia(
//         std::shared_ptr<DtnContext> dtnContext, std::shared_ptr<DtnFrameQueue> dtnFrameQueue)
//         : m_DtnContext(dtnContext), m_DtnFrameQueue(dtnFrameQueue) {};

//     ~DtnMedia() {};


//     rtp_error_t PushFrame(buffer * image_buffer, int rtp_flags);


// protected:
//     virtual rtp_error_t PushMediaFrame(buffer * image_buffer, int rtp_flags);

// };

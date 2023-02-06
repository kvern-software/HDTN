#include <uvgrtp/lib.hh>

#include "DtnContext.h"

int main(int argc, const char* argv[])
{

    DtnContext dtnContext(10000);
    
    std::shared_ptr<DtnSession> dtnSession = dtnContext.CreateDtnSession();
   
    std::shared_ptr<DtnMediaStreamSource> dtnMediaStream = dtnSession->CreateDtnMediaStreamSource(RTP_FORMAT_H265,0);

    // inialize the BpSourcePattern with configs read in from the config file
    // dtnMediaStream->Start(...);



    // initialize collection of video data
    dtnMediaStream->StartComponents();
    dtnMediaStream->m_videoDriverPtr->device = "/dev/video0";//video_device;
    dtnMediaStream->m_videoDriverPtr->OpenFD();
    dtnMediaStream->m_videoDriverPtr->CheckDeviceCapability();
    dtnMediaStream->m_videoDriverPtr->SetImageFormat(DEFAULT_VIDEO_CAPTURE, 720, 1080, 
            V4L2_PIX_FMT_HEVC, DEFAULT_FIELD);
    dtnMediaStream->m_videoDriverPtr->SetBufferQueueSize(30);
    dtnMediaStream->m_videoDriverPtr->RequestBuffer(V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP);
    dtnMediaStream->m_videoDriverPtr->AllocateLocalBuffers();    
    dtnMediaStream->m_videoDriverPtr->Start(); // will fill buffers in dtnMediaSteam


    bool running;
    dtnMediaStream->Run(argc,argv,running, true);
    
    while (1)
    {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
    }

    return 0;
}
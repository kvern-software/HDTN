#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <string>
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/make_unique.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/strict_lock.hpp>

#include "MediaApp.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define DEFAULT_CHUNK_WRITE_SIZE 8192*4

#define DEFAULT_VIDEO_CAPTURE V4L2_BUF_TYPE_VIDEO_CAPTURE
#define DEFAULT_PIXEL_FORMAT V4L2_PIX_FMT_MJPEG
#define DEFAULT_FIELD V4L2_FIELD_NONE
#define DEFAULT_MEMORY_MAP V4L2_MEMORY_MMAP

struct buffer {
        void   *start;
        size_t  length;
};

enum modes {
    FIFO=0,
    NEVER_DROP=1
};

class VideoDriver
{
private:
    std::unique_ptr<boost::thread> m_VideoDriverBufferFillerThreadPtr;
    volatile bool m_running = false;
    MediaApp *m_mediaApp = nullptr;
    int m_mode = 0;
    uint64_t m_frames_per_second;
    /* data */
public:
    VideoDriver();
    ~VideoDriver();

    void Start();
    void Stop();

    // Configuration, done once
    int OpenFD();
    int CheckDeviceCapability();
    int SetImageFormat(unsigned int type, unsigned int width, unsigned int height, 
            unsigned int pixelformat, unsigned int field);
    int SetImageFormat(v4l2_format imageFormat); 
    int SetFramerate(uint64_t frames_per_second);
    void RegisterCallback(MediaApp* mediaApp);
    void SetCaptureMode(int mode);

    // Repeatedly called members for collecting data
    int RequestBuffer(unsigned int type, unsigned int memory);
    void MapMemory();
    void AllocateLocalBuffers();
    int QueryBuffer(unsigned int index);
    int StartVideoStream();
    int EndVideoStream();

    int CaptureFrames();
    int CaptureFramesFIFO();

    // internally called to get video frames from device
    int QueueBuffers(); // queue FPS frames
    int QueueBuffer(int buffer_idx); // single frame queue
    int DequeueBuffers(); // dequeue FPS frames
    int DequeueBuffer(int buffer_idx); // single frame dequeue

    // externally called to prevent video frames to be written to while being read
    int RetrieveFrame(int index, boost::strict_lock<VideoDriver>&);
    
    void BufferFillerThreadFunc();
    int WriteBufferToFile(std::string filePath, unsigned int chunkSize);


    // members
    int fd; // file descriptor 
    boost::filesystem::path device; // path to device e.g. /dev/video0


    // video related members
    v4l2_capability capability;
    v4l2_format imageFormat;

    // char * image_data; // this points to the memory address of the device
    v4l2_requestbuffers requestBuffer = {0};
    buffer *image_buffers; // this holds the image data of multiple frames
    v4l2_buffer bufToQuery; 
};

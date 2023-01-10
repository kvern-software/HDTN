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



struct buffer {
        void   *start;
        size_t  length;
};




#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define DEFAULT_CHUNK_WRITE_SIZE 8192*4

#define DEFAULT_VIDEO_CAPTURE V4L2_BUF_TYPE_VIDEO_CAPTURE
#define DEFAULT_PIXEL_FORMAT V4L2_PIX_FMT_MJPEG
#define DEFAULT_FIELD V4L2_FIELD_NONE
#define DEFAULT_MEMORY_MAP V4L2_MEMORY_MMAP

class VideoDriver
{
private:
    std::unique_ptr<boost::thread> m_VideoDriverBufferFillerThreadPtr;
    volatile bool m_running;

    /* data */
public:
    VideoDriver(/* args */);
    ~VideoDriver();

    void Start(uint64_t frames_per_second);
    void Stop();

    int OpenFD();
    int CheckDeviceCapability();
    int SetImageFormat(unsigned int type, unsigned int width, unsigned int height, 
            unsigned int pixelformat, unsigned int field);
    int SetImageFormat(v4l2_format imageFormat); 
    int RequestBuffer(unsigned int num_buffers, unsigned int type, unsigned int memory);
    int QueryBuffer(unsigned int index);
    void MapMemory();
    int StartVideoStream();
    int EndVideoStream();

    // call these to get data
    int QueueBuffer();
    int DequeueBuffer();

    void BufferFillerThreadFunc(uint64_t frames_per_second);

    int WriteBufferToFile(std::string filePath, unsigned int chunkSize);


    // members
    int fd; // file descriptor 
    boost::filesystem::path device; // path to device e.g. /dev/video0
    char * image_data; // this points to the memory address of the device
    // video related members
    v4l2_capability capability;
    v4l2_format imageFormat;
    v4l2_requestbuffers requestBuffer = {0};
    v4l2_buffer queryBuffer = {0};
    v4l2_buffer bufferinfo; // lets device know which buffer we are talking about
};

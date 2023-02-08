#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstddef>
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

struct buffer {
        void   *start;
        size_t  length;
        int id = 0;
        void allocate(size_t new_length)
        {
            start = malloc(new_length);
            length = new_length;
        }

        void unallocate()
        {
            free(start);
        }

};

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define DEFAULT_CHUNK_WRITE_SIZE 8192*4

#define DEFAULT_VIDEO_CAPTURE V4L2_BUF_TYPE_VIDEO_CAPTURE
#define DEFAULT_PIXEL_FORMAT  V4L2_PIX_FMT_HEVC //V4L2_PIX_FMT_MJPEG 
#define DEFAULT_FIELD V4L2_FIELD_NONE
#define DEFAULT_MEMORY_MAP V4L2_MEMORY_MMAP

class VideoDriver
{

private:
    typedef boost::function<void(buffer * image_buffer)> ExportFrameCallback_t;
    
    std::unique_ptr<boost::thread> m_VideoDriverBufferFillerThreadPtr;
    volatile bool m_running = false;

    uint64_t m_bufferQueueSize;
    ExportFrameCallback_t m_exportFrameCallback;

public:
    VideoDriver(const ExportFrameCallback_t & exportFrameCallback);
    ~VideoDriver();

    int Init(std::string device, uint16_t frame_width, uint16_t frame_height, uint64_t buffer_queue_size);
    void Start();
    void Stop();

    // Configuration, done once
    int OpenFD();
    int CheckDeviceCapability();
    int SetImageFormat(unsigned int type, unsigned int width, unsigned int height, 
            unsigned int pixelformat, unsigned int field);
    int SetImageFormat(v4l2_format imageFormat); 
    int SetBufferQueueSize(uint64_t bufferQueueSize);

    // Repeatedly called members for collecting data
    int RequestBuffer(unsigned int type, unsigned int memory);
    void MapMemory();
    void AllocateLocalBuffers();
    int QueryBuffer(unsigned int index);
    int StartVideoStream();
    int EndVideoStream();


    // internally called to get video frames from device
    int QueueBuffers(); // queue FPS frames
    int QueueBuffer(int buffer_idx); // single frame queue
    int DequeueBuffers(); // dequeue FPS frames
    int DequeueBuffer(int buffer_idx); // single frame dequeue
    
    int CaptureFramesFIFO();

    void BufferFillerThreadFunc();
    // int WriteBufferToFile(std::string filePath, unsigned int chunkSize);

    // members
    int fd; // file descriptor 
    boost::filesystem::path m_device; // path to device e.g. /dev/video0

    // video related members
    v4l2_capability capability;
    v4l2_format imageFormat;

    v4l2_requestbuffers requestBuffer = {0};
    buffer *image_buffers; // this holds the image data of multiple frames
    v4l2_buffer bufToQuery; 
};
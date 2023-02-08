#include "VideoDriver.h"
#include "Logger.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

VideoDriver::VideoDriver(const ExportFrameCallback_t & exportFrameCallback)
{
    // m_exportFrameCallback = ;
    m_exportFrameCallback = exportFrameCallback;
}

VideoDriver::~VideoDriver()
{

}

// initialize video driver to be ready to start capturing frames
int VideoDriver::Init(std::string device, uint16_t frame_width, uint16_t frame_height, uint64_t buffer_queue_size)
{
    m_device = device;
    OpenFD();
    CheckDeviceCapability();
    SetImageFormat(DEFAULT_VIDEO_CAPTURE, frame_width, frame_height, 
            DEFAULT_PIXEL_FORMAT, DEFAULT_FIELD);
    SetBufferQueueSize(buffer_queue_size);
//    SetFramerate(frames_per_second);
//    SetCaptureMode(FIFO); // todo get input come command line
   RequestBuffer(V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP);
   AllocateLocalBuffers();   

   return 0; 
}

void VideoDriver::Start() {
    if (!m_running) {
        m_running = true;
        // std::cout << "starting " << std::endl;
        m_VideoDriverBufferFillerThreadPtr = boost::make_unique<boost::thread>(
        boost::bind(&VideoDriver::BufferFillerThreadFunc, this)); //create and start the worker thread
    }
}

void VideoDriver::Stop() {
    m_running = false; //thread stopping criteria

    if(m_VideoDriverBufferFillerThreadPtr) {
        m_VideoDriverBufferFillerThreadPtr->join();
        m_VideoDriverBufferFillerThreadPtr.reset(); //delete it
    }

    EndVideoStream();
}

/*
    Open a file descriptor to the device specified
    by the device member. Return 0 on success.
*/
int VideoDriver::OpenFD() {
    fd = open(m_device.c_str(), O_RDWR);
    if(fd < 0) {
        LOG_ERROR(subprocess) << "Failed to open device, OPEN";
        return 1;
    }

    LOG_INFO(subprocess) << "Opened video file descriptor";
    return 0;
}

/*
    Determines if the device can capture frames. This is not
    required before streaming.
*/
int VideoDriver::CheckDeviceCapability() {
    if(ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0){
        LOG_ERROR(subprocess) << ("Failed to get device capabilities, VIDIOC_QUERYCAP");
        return 1;
    }

    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOG_ERROR(subprocess) << "not a valid video capture device";
        return -1;    
    }

    if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
        LOG_ERROR(subprocess) << "video capture device does not support  does not support streaming i/o";
        return -1;
    }

    //  VIDIOC_ENUM_FMT()
    // std::cout << fmt << std::endl;

    return 0;
}

/*
    Set the desired image format. Format is based on the v4l2_format
    class. Note that limitations to the format may be enforced by the
    camera. If a higher resolution is requested than a camera supports, 
    then the resolution is set to the maximum of the camera. Any combination
    of lower resolution width and height can be used.
*/
int VideoDriver::SetImageFormat(unsigned int type, unsigned int width, unsigned int height, 
        unsigned int pixelformat, unsigned int field) {
    imageFormat.type = type;
    imageFormat.fmt.pix.width = width;
    imageFormat.fmt.pix.height = height;
    imageFormat.fmt.pix.pixelformat = pixelformat;
    imageFormat.fmt.pix.field = field;

    // inform device of this format 
    if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0){
        LOG_ERROR(subprocess) << "Device could not set format, VIDIOC_S_FMT";
        return 1;
    }

    LOG_INFO(subprocess) << "Set image format";

    return 0;
}

int VideoDriver::SetImageFormat(v4l2_format imageFormat_) {
    imageFormat = imageFormat_;
    // inform device of this format 
    if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0){
        LOG_ERROR(subprocess) << "Device could not set format, VIDIOC_S_FMT";
        return 1;
    }

    LOG_INFO(subprocess) << "Set image format";

    return 0;
}

int VideoDriver::SetBufferQueueSize(uint64_t bufferQueueSize) 
{
    m_bufferQueueSize = bufferQueueSize;
    return 0;
}

/*
    Request buffer from video device
*/
int VideoDriver::RequestBuffer(unsigned int type=V4L2_BUF_TYPE_VIDEO_CAPTURE, unsigned int memory=V4L2_MEMORY_MMAP) {
    requestBuffer.count = m_bufferQueueSize; // num buffers = frames / second
    requestBuffer.type = type; // request a buffer which we an use for capturing frames
    requestBuffer.memory = memory;

    if(ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0){
        LOG_ERROR(subprocess) << "Could not request buffer from device, VIDIOC_REQBUFS";
        return 1;
    }

    LOG_INFO(subprocess) <<  "Requested buffer";

    return 0;
}

void VideoDriver::AllocateLocalBuffers() {
    image_buffers = reinterpret_cast<buffer *>(calloc(requestBuffer.count, sizeof(*image_buffers)));
    if (!image_buffers) {
        LOG_ERROR(subprocess) << "Out of memory";
        exit(EXIT_FAILURE);
    }
}

// use a pointer to point to the newly created buffer
// mmap() will map the memory address of the device to
// an address in memory
void VideoDriver::MapMemory() {
    // image_data = (char *)reinterpret_cast<char*>(mmap(NULL, bufToQuery.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        // fd, bufToQuery.m.offset));
    // memset(image_data, 0, bufToQuery.length);
    for (uint64_t buf_idx = 0; buf_idx < requestBuffer.count; ++buf_idx) {
        QueryBuffer(buf_idx);

        image_buffers[buf_idx].length = bufToQuery.length;
        image_buffers[buf_idx].start =
                mmap(NULL /* start anywhere */,
                    bufToQuery.length,
                    PROT_READ | PROT_WRITE /* required */,
                    MAP_SHARED /* recommended */,
                    fd, bufToQuery.m.offset);
            
        if (MAP_FAILED == image_buffers[m_bufferQueueSize].start) {
            LOG_ERROR(subprocess) << "mmap error";
            exit(EXIT_FAILURE);
        }
    }

    LOG_INFO(subprocess) << "video kernel memory mapped";
}

/*
    Allocates memory for the buffer request
*/
int VideoDriver::QueryBuffer(unsigned int index=0) {
    bufToQuery.type = requestBuffer.type; /// pull general buffer information from the request buffer into the query buffer
    bufToQuery.memory = requestBuffer.memory;
    bufToQuery.index = index;

    if(ioctl(fd, VIDIOC_QUERYBUF, &bufToQuery) < 0){
        LOG_ERROR(subprocess) << "Device did not return the buffer information, VIDIOC_QUERYBUF";
        return 1;
    }

    // LOG_INFO(subprocess) << "Allocated memory for buffer";
    return 0;
}

/*
    Informs kernel we will be requesting buffers to be filled
*/
int VideoDriver::StartVideoStream() {
    // Activate streaming
    int type = requestBuffer.type;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        LOG_ERROR(subprocess) << "Could not start streaming, VIDIOC_STREAMON";
        return 1;
    }

    LOG_INFO(subprocess) << "Started video stream";

    return 0;
}

/*
    Informs kernel we will no longer be requesting frames
*/
int VideoDriver::EndVideoStream() {
    if(ioctl(fd, VIDIOC_STREAMOFF, &requestBuffer.type) < 0){
        LOG_ERROR(subprocess) << "Could not end streaming, VIDIOC_STREAMOFF";
        return 1;
    }

    LOG_INFO(subprocess) << "Ended video stream";
    return 0;
}

/*
    This effectively informs the kernel that we would like our 
    preallocated buffer to be filled with video data.
*/
int VideoDriver::QueueBuffers() {
    for (uint64_t i = 0; i < m_bufferQueueSize; i++) {
        bufToQuery.index = i;
    
        if(ioctl(fd, VIDIOC_QBUF, &bufToQuery) < 0){
            LOG_ERROR(subprocess) << "Could not queue buffer, VIDIOC_QBUF";
            return 1;
        }
    }

    return 0;
}

int VideoDriver::QueueBuffer(int buffer_idx) {
    bufToQuery.index = buffer_idx;
    
    if(ioctl(fd, VIDIOC_QBUF, &bufToQuery) < 0){
        LOG_ERROR(subprocess) << "Could not queue buffer, VIDIOC_QBUF";
        return 1;
    }

    return 0;
}

/*
    This effectively informs the kernel that we would like
    to access the data inside our buffer. Must be called before
    accessing the data through our pointer since the frame gets
    written AFTER dequeing the buffer.
*/
int VideoDriver::DequeueBuffers() {
    for (uint64_t i = 0; i < m_bufferQueueSize; i++) {
        bufToQuery.index = i;
        if(ioctl(fd, VIDIOC_DQBUF, &bufToQuery) < 0){
            LOG_ERROR(subprocess) << "Could not dequeue the buffer, VIDIOC_DQBUF";
            return 1;
        }
    }

    return 0;
}

int VideoDriver::DequeueBuffer(int buffer_idx) {
    bufToQuery.index = buffer_idx;
    if(ioctl(fd, VIDIOC_DQBUF, &bufToQuery) < 0){
        LOG_ERROR(subprocess) << "Could not dequeue the buffer, VIDIOC_DQBUF";
        return 1;
    }
    

    return 0;
}


/*
    Write the image data to a file in storage. Different chunk
    sizes can be used depending on the platform capabilities.
*/
// int VideoDriver::WriteBufferToFile(std::string filePath, unsigned int chunkSize) {
    // LOG_INFO(subprocess) << "Buffer has: " << (double)bufToQuery.bytesused / 1024 << " KBytes of data";

    // std::ofstream outFile;
    // outFile.open(filePath.c_str(), std::ios::binary | std::ios::app);

    // int bufPos = 0, outFileMemBlockSize = 0;  // the position in the buffer and the amoun to copy from the buffer
    // int remainingBufferSize = bufferinfo.bytesused; // the remaining buffer size, is decremented by memBlockSize amount on each loop so we do not overwrite the buffer
    // char* outFileMemBlock = NULL;  // a pointer to a new memory block
    // int itr = 0; // counts the number of iterations

    // while(remainingBufferSize > 0) {
    //     bufPos += outFileMemBlockSize;  // increment the buffer pointer on each loop
    //                                     // initialise bufPos before outFileMemBlockSize so we can start
    //                                     // at the begining of the buffer

    //     outFileMemBlockSize = chunkSize;    // set the output block size to a preferable size. 1024 :)
    //     outFileMemBlock = new char[sizeof(char) * outFileMemBlockSize];

    //     // copy 1024 bytes of data starting from buffer+bufPos
    //     memcpy(outFileMemBlock, image_data+bufPos, outFileMemBlockSize);
    //     outFile.write(outFileMemBlock,outFileMemBlockSize);

    //     // calculate the amount of memory left to read
    //     // if the memory block size is greater than the remaining
    //     // amount of data we have to copy
    //     if(outFileMemBlockSize > remainingBufferSize)
    //         outFileMemBlockSize = remainingBufferSize;

    //     // subtract the amount of data we have to copy from the remaining buffer size
    //     remainingBufferSize -= outFileMemBlockSize;

    //     // display the remaining buffer size
    //     LOG_INFO(subprocess) << itr++ << " Remaining bytes: "<< remainingBufferSize;
    // }

    // // Close the file
    // outFile.close();

    // LOG_INFO(subprocess) << filePath.c_str() << " written to storage";

    // return 0;
// }

int VideoDriver::CaptureFramesFIFO() {
    for (uint64_t i=0; i < m_bufferQueueSize; i++) {
        QueueBuffer(i);
        DequeueBuffer(i);
        // LOG_INFO(subprocess) << "size of image buf: " << image_buffers[i].length;
        m_exportFrameCallback(&image_buffers[i]);
    }

    return 0;
}

void VideoDriver::BufferFillerThreadFunc() {
    MapMemory();
    StartVideoStream();
    while (m_running) {
        CaptureFramesFIFO();
    }
        // boost::this_thread::sleep(boost::posix_time::milliseconds(20));
}

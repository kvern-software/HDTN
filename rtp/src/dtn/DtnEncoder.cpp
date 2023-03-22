#include "DtnEncoder.h"


void DtnEncoder::FrameCallback(buffer * image_buffer) 
{
    // for now just export image to the frame queue, dont encode
    // m_frameQueue->PushFrame(image_buffer);
    std::cout << "pushed frame from encoder" << std::endl;
}


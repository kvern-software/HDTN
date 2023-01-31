#include "Fragmenter.h"


Fragmenter::Fragmenter(uint64_t bundleSizeBytes) 
{
    m_fragmentSizeBytes = bundleSizeBytes - sizeof(fragmentHeader_t); // header overhead size accounted for here
}

Fragmenter::~Fragmenter() 
{

}

bool Fragmenter::HasFragments() 
{
    return !m_fragmentQueue.empty();
}

// NOTE: RETURNS SIZE OF THE ENTIRE PACKET, NOT THE BUFFER LENGTH
size_t Fragmenter::GetNextFragmentSize() 
{ 
    // total size is buffer length + header
    return m_fragmentQueue.front().fragmentBuffer.length + sizeof(fragmentHeader_t); 
}

int Fragmenter::GetNextFragmentID()
{
    return m_fragmentQueue.front().header.fragment_id;
}

fragment_t * Fragmenter::GetNextFragment() 
{
    return &m_fragmentQueue.front();
}


void Fragmenter::PopFragment() 
{
    m_fragmentQueue.pop();
}


/*
Fragment the given buffer into bundleSizeBytes fragments. Push fragments into fragment queue.
The fragmenter is not aware if the buffer was previously compressed.
*/
void Fragmenter::Fragment(buffer * frameBuffer, size_t sourceLengthBytes) 
{
    int bufPos = 0; // the position in the buffer 
    size_t remainingBufferSize = frameBuffer->length; // the remaining buffer size, is decremented by memBlockSize amount on each loop so we do not overwrite the buffer

    uint64_t numberFragmentsRequired = (frameBuffer->length / m_fragmentSizeBytes) + 1;
    
    uint64_t currentID = 0;
    while(remainingBufferSize > 0) {
        m_fragmentQueue.emplace(currentID, numberFragmentsRequired, m_numberFramesFragmented, sourceLengthBytes);
        m_fragmentQueue.back().allocate(m_fragmentSizeBytes);

        currentID++;
        m_numberFragmentsCreated++;

        if (remainingBufferSize < m_fragmentSizeBytes) {
            memcpy(m_fragmentQueue.back().fragmentBuffer.start, ((char*)frameBuffer->start)+bufPos, remainingBufferSize); // last packet
            remainingBufferSize = 0;
            m_numberFramesFragmented++;
            return;
        } else {
            memcpy(m_fragmentQueue.back().fragmentBuffer.start, ((char*)frameBuffer->start)+bufPos, m_fragmentSizeBytes); // all but last
            bufPos += m_fragmentSizeBytes;  // increment the buffer pointer on each loop
        }

        remainingBufferSize -= m_fragmentSizeBytes; // subtract the amount of data we have to copy from the remaining buffer size
    }
}
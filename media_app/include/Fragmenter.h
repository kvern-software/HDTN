#pragma once
#include <stdint.h>
#include <queue>

#include "common.h"
#include <cstdlib> // malloc
#include <cstring> // memcpy

struct fragmentHeader_t {
    uint8_t fragment_id;
    uint8_t numberFragmentsRequired;
    uint64_t frame_id; 
    uint64_t sourceLengthBytes; // if compressed, this is bigger than the current length. must know this to uncompress
};


struct fragment_t {
    fragmentHeader_t header;
    buffer fragmentBuffer;

    fragment_t(uint8_t fragment_id, uint8_t numberFragmentsRequired, uint64_t frame_id, uint64_t sourceLengthBytes)
    {
        header.fragment_id = fragment_id;
        header.numberFragmentsRequired = numberFragmentsRequired;
        header.frame_id = frame_id;
        header.sourceLengthBytes = sourceLengthBytes;
    }

    void allocate(size_t length) 
    {
        fragmentBuffer.start = malloc(length);
        fragmentBuffer.length = length;
    }


};



class Fragmenter 
{
private:
    uint64_t m_fragmentSizeBytes;
    std::queue<fragment_t> m_fragmentQueue;

    uint64_t m_numberFragmentsCreated=0;
    uint64_t m_numberFramesFragmented=0;

public:
    Fragmenter(uint64_t bundleSizeBytes);
    ~Fragmenter();

    bool HasFragments();
    size_t GetNextFragmentSize();
    int GetNextFragmentID();
    fragment_t * GetNextFragment();
    void PopFragment();

    void Fragment(buffer * frameBuffer, size_t sourceLengthBytes);

};


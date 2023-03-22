#pragma once
#include <stdint.h>
#include <deque>

#include "common.h"
#include <cstdlib> // malloc
#include <cstring> // memcpy

#include "Fragmenter.h"
#include "common.h"

class Assembler
{
private:
    std::deque<fragment_t> m_fragmentQueue;

    typedef boost::function<void(buffer * image_buffer)> ExportFrameCallback_t;
    std::unique_ptr<boost::thread> m_AssemblerThreadPtr;
    ExportFrameCallback_t m_exportFrameCallback;

    volatile bool m_running;

    uint8_t m_currentFragmentID;
    uint64_t m_currentFrameID;

public:
    Assembler(const ExportFrameCallback_t & exportFrameCallback);
    ~Assembler();
    Start();
    Stop();
    FragmentAssembler();


};


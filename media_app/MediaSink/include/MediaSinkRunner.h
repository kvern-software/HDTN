#pragma once

#include <stdint.h>


class MediaSinkRunner {
public:
    MediaSinkRunner();
    ~MediaSinkRunner();
    bool Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler);
    void Print();
    uint64_t m_totalBytesRx;

private:
    void MonitorExitKeypressThreadFunction();

    volatile bool m_runningFromSigHandler;
};



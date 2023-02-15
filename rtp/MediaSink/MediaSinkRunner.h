#pragma once

#include <stdint.h>
#include "MediaSink.h"
#include "MediaGui.h"

class MediaSinkRunner {
public:
    MediaSinkRunner();
    ~MediaSinkRunner();
    bool Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler);
    uint64_t m_totalBytesRx;
    uint64_t m_receivedCount;
    uint64_t m_duplicateCount;
    FinalStatsBpSink m_FinalStatsBpSink;

private:
    void MonitorExitKeypressThreadFunction();

    volatile bool m_runningFromSigHandler;
};



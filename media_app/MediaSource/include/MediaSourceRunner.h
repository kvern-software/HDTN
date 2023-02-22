#pragma once

#include <stdint.h>
#include "MediaSource.h"

class MediaSourceRunner {
public:
    MediaSourceRunner();
    ~MediaSourceRunner();
    bool Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler);
    
    uint64_t m_bundleCount;
    uint64_t m_totalBundlesAcked;

    OutductFinalStats m_outductFinalStats;

private:
    void MonitorExitKeypressThreadFunction();

    volatile bool m_runningFromSigHandler;
};



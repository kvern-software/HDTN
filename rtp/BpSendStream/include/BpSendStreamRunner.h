#pragma once

#include "BpSendStream.h"
#include <stdint.h>


class BpSendStreamRunner {
public:
    BpSendStreamRunner();
    ~BpSendStreamRunner();

    std::string ReadSdpFile(const boost::filesystem::path sdpFilePath);
    bool Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler);
    uint64_t m_bundleCount;
    uint64_t m_totalBundlesAcked;

    OutductFinalStats m_outductFinalStats;


private:
    void MonitorExitKeypressThreadFunction();

    volatile bool m_runningFromSigHandler;
};



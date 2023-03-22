
#include <iostream>
#include "MediaSinkRunner.h"
#include "Logger.h"
#include "ThreadNamer.h"

int main(int argc, const char* argv[]) {


    hdtn::Logger::initializeWithProcess(hdtn::Logger::Process::bpsink);
    ThreadNamer::SetThisThreadName("BpSinkMain");
    MediaSinkRunner runner;
    volatile bool running;
    runner.Run(argc, argv, running, true);
    LOG_INFO(hdtn::Logger::SubProcess::none) << "Rx Count, Duplicate Count, Total Count, Total bytes Rx";
    LOG_INFO(hdtn::Logger::SubProcess::none) << runner.m_receivedCount 
        << "," << runner.m_duplicateCount 
        << "," << (runner.m_receivedCount + runner.m_duplicateCount)
        << "," << runner.m_totalBytesRx;
    return 0;

}

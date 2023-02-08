#include "MediaSourceRunner.h"
#include "Logger.h"

int main(int argc, const char* argv[])
{

#if 0
    const char * manualArgv[5] = { "bpgen", "--bundle-rate=200", "--use-tcpcl", "--flow-id=2", NULL };
    argv = manualArgv;
    argc = 4;
#endif
    
    hdtn::Logger::initializeWithProcess(hdtn::Logger::Process::bpgen);
    MediaSourceRunner runner;
    volatile bool running;
    runner.Run(argc, argv, running, true);
    LOG_INFO(hdtn::Logger::SubProcess::none) << "bundle count main: " << runner.m_bundleCount;
    
    return 0;

  


}
#include <iostream>
#include "MediaAppRunner.h"
#include "Logger.h"


int main(int argc, const char* argv[]) {

    hdtn::Logger::initializeWithProcess(hdtn::Logger::Process::bpreceivefile);
    MediaAppRunner runner;
    volatile bool running;
    runner.Run(argc, argv, running, true);
    // runner.Print();
    return 0;

}

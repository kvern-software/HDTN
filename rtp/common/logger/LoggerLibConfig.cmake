get_filename_component(LOGGERLIB_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

#find_dependency seems broken for multiple calls to find_boost, use find_package instead (https://stackoverflow.com/questions/52763112/cmake-boost-find-depedency-config)
#find_dependency(Boost 1.66.0 REQUIRED COMPONENTS log)
find_package(Boost 1.66.0 REQUIRED COMPONENTS log)

if(NOT TARGET HDTN::LoggerLib)
    include("${LOGGERLIB_CMAKE_DIR}/LoggerLibTargets.cmake")
endif()

set(LOGGERLIB_LIBRARIES HDTN::LoggerLib)

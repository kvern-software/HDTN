get_filename_component(UDPDELAYSIMLIB_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

find_dependency(HDTNUtil REQUIRED)
find_dependency(LoggerLib REQUIRED)

#find_dependency seems broken for multiple calls to find_boost, use find_package instead (https://stackoverflow.com/questions/52763112/cmake-boost-find-depedency-config)
#find_dependency(Boost 1.66.0 REQUIRED COMPONENTS program_options)
find_package(Boost 1.66.0 REQUIRED COMPONENTS program_options)

if(NOT TARGET HDTN::UdpDelaySimLib)
    include("${UDPDELAYSIMLIB_CMAKE_DIR}/UdpDelaySimLibTargets.cmake")
endif()

set(UDPDELAYSIMLIB_LIBRARIES HDTN::UdpDelaySimLib)

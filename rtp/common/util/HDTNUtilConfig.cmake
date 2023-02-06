get_filename_component(HDTNUTIL_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

#find_dependency seems broken for multiple calls to find_boost, use find_package instead (https://stackoverflow.com/questions/52763112/cmake-boost-find-depedency-config)
#find_dependency(Boost 1.66.0 REQUIRED COMPONENTS filesystem regex date_time thread)
find_package(Boost 1.66.0 REQUIRED COMPONENTS filesystem regex date_time thread)
find_dependency(LoggerLib REQUIRED)

if(ON)
	find_dependency(OpenSSL REQUIRED)
endif()

if(NOT TARGET HDTN::HDTNUtil)
    include("${HDTNUTIL_CMAKE_DIR}/HDTNUtilTargets.cmake")
endif()

#because HDTNUtil serves as a dependency for all other HDTN libraries, the following lines set settings needed for all
set(libzmq_INCLUDE )
set(libzmq_LIB zmq)

#add compile definitions from hardware_acceleration.cmake used in Sdnv.h, CborUint.h, and Bpv7Crc.h (should be fixed, but for now the package config must export it)
#also add compile definitions from root cmake such as openssl support 
add_compile_definitions(USE_SDNV_FAST;SDNV_SUPPORT_AVX2_FUNCTIONS;USE_CBOR_FAST;USE_CRC32C_FAST;OPENSSL_SUPPORT_ENABLED)

set(HDTNUTIL_LIBRARIES HDTN::HDTNUtil)

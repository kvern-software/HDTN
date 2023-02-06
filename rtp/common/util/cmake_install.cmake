# Install script for directory: /home/kyle/nasa/dev/HDTN/common/util

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kyle/nasa/dev/HDTN/rtp/common/util/libhdtn_util.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/common/util/include/BinaryConversions.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/BundleCallbackFunctionDefines.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/CborUint.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/CircularIndexBufferSingleProducerSingleConsumerConfigurable.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/CpuFlagDetection.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/DeadlineTimer.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/DirectoryScanner.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/EnumAsFlagsMacro.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/Environment.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/ForwardListQueue.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/FragmentSet.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/JsonSerializable.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/LtpClientServiceDataToSend.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/MemoryInFiles.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/PaddedVectorUint8.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/Sdnv.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/SignalHandler.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/TokenRateLimiter.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/TcpAsyncSender.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/TimestampUtil.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/UdpBatchSender.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/Uri.h"
    "/home/kyle/nasa/dev/HDTN/common/util/include/zmq.hpp"
    "/home/kyle/nasa/dev/HDTN/rtp/common/util/hdtn_util_export.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN" TYPE DIRECTORY FILES "/home/kyle/nasa/dev/HDTN/common/util/include/dir_monitor")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/HDTNUtil/HDTNUtilTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/HDTNUtil/HDTNUtilTargets.cmake"
         "/home/kyle/nasa/dev/HDTN/rtp/common/util/CMakeFiles/Export/lib/cmake/HDTNUtil/HDTNUtilTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/HDTNUtil/HDTNUtilTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/HDTNUtil/HDTNUtilTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/HDTNUtil" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/util/CMakeFiles/Export/lib/cmake/HDTNUtil/HDTNUtilTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/HDTNUtil" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/util/CMakeFiles/Export/lib/cmake/HDTNUtil/HDTNUtilTargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/HDTNUtil" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/rtp/common/util/HDTNUtilConfig.cmake"
    "/home/kyle/nasa/dev/HDTN/rtp/common/util/HDTNUtilConfigVersion.cmake"
    )
endif()


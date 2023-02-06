# Install script for directory: /home/kyle/nasa/dev/HDTN/common/bpcodec

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/libbpcodec.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN/codec" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/bpv6.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/bpv7.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/Bpv7Crc.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/BundleViewV6.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/BundleViewV7.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/Cbhe.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/Cose.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/CustodyIdAllocator.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/CustodyTransferManager.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/codec/PrimaryBlock.h"
    "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/bpcodec_export.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Bpcodec/BpcodecTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Bpcodec/BpcodecTargets.cmake"
         "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/CMakeFiles/Export/lib/cmake/Bpcodec/BpcodecTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Bpcodec/BpcodecTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Bpcodec/BpcodecTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Bpcodec" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/CMakeFiles/Export/lib/cmake/Bpcodec/BpcodecTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Bpcodec" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/CMakeFiles/Export/lib/cmake/Bpcodec/BpcodecTargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Bpcodec" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/BpcodecConfig.cmake"
    "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/BpcodecConfigVersion.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/../include/message.hpp"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/../include/stats.hpp"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/libbp_app_patterns_lib.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN/app_patterns" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/app_patterns/BpSinkPattern.h"
    "/home/kyle/nasa/dev/HDTN/common/bpcodec/include/app_patterns/BpSourcePattern.h"
    "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/bp_app_patterns_lib_export.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/BpAppPatternsLib/BpAppPatternsLibTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/BpAppPatternsLib/BpAppPatternsLibTargets.cmake"
         "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/CMakeFiles/Export/lib/cmake/BpAppPatternsLib/BpAppPatternsLibTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/BpAppPatternsLib/BpAppPatternsLibTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/BpAppPatternsLib/BpAppPatternsLibTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/BpAppPatternsLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/CMakeFiles/Export/lib/cmake/BpAppPatternsLib/BpAppPatternsLibTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/BpAppPatternsLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/CMakeFiles/Export/lib/cmake/BpAppPatternsLib/BpAppPatternsLibTargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/BpAppPatternsLib" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/BpAppPatternsLibConfig.cmake"
    "/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/BpAppPatternsLibConfigVersion.cmake"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/apps/cmake_install.cmake")

endif()


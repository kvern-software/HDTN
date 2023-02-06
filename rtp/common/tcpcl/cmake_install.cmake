# Install script for directory: /home/kyle/nasa/dev/HDTN/common/tcpcl

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kyle/nasa/dev/HDTN/rtp/common/tcpcl/libtcpcl_lib.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/BidirectionalLink.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/Tcpcl.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/TcpclBundleSink.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/TcpclBundleSource.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/TcpclV3BidirectionalLink.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/TcpclV4.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/TcpclV4BidirectionalLink.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/TcpclV4BundleSink.h"
    "/home/kyle/nasa/dev/HDTN/common/tcpcl/include/TcpclV4BundleSource.h"
    "/home/kyle/nasa/dev/HDTN/rtp/common/tcpcl/tcpcl_lib_export.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/TcpclLib/TcpclLibTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/TcpclLib/TcpclLibTargets.cmake"
         "/home/kyle/nasa/dev/HDTN/rtp/common/tcpcl/CMakeFiles/Export/lib/cmake/TcpclLib/TcpclLibTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/TcpclLib/TcpclLibTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/TcpclLib/TcpclLibTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/TcpclLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/tcpcl/CMakeFiles/Export/lib/cmake/TcpclLib/TcpclLibTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/TcpclLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/tcpcl/CMakeFiles/Export/lib/cmake/TcpclLib/TcpclLibTargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/TcpclLib" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/rtp/common/tcpcl/TcpclLibConfig.cmake"
    "/home/kyle/nasa/dev/HDTN/rtp/common/tcpcl/TcpclLibConfigVersion.cmake"
    )
endif()


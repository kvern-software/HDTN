# Install script for directory: /home/kyle/nasa/dev/HDTN/common/ltp

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/libltp_lib.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/Ltp.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpBundleSink.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpBundleSource.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpEngine.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpEngineConfig.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpFragmentSet.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpNoticesToClientService.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpRandomNumberGenerator.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpSessionReceiver.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpSessionRecreationPreventer.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpSessionSender.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpTimerManager.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpUdpEngine.h"
    "/home/kyle/nasa/dev/HDTN/common/ltp/include/LtpUdpEngineManager.h"
    "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/ltp_lib_export.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/LtpLib/LtpLibTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/LtpLib/LtpLibTargets.cmake"
         "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/CMakeFiles/Export/lib/cmake/LtpLib/LtpLibTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/LtpLib/LtpLibTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/LtpLib/LtpLibTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/LtpLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/CMakeFiles/Export/lib/cmake/LtpLib/LtpLibTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/LtpLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/CMakeFiles/Export/lib/cmake/LtpLib/LtpLibTargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/LtpLib" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/LtpLibConfig.cmake"
    "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/LtpLibConfigVersion.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ltp-file-transfer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ltp-file-transfer")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ltp-file-transfer"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/ltp/ltp-file-transfer")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ltp-file-transfer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ltp-file-transfer")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ltp-file-transfer")
    endif()
  endif()
endif()


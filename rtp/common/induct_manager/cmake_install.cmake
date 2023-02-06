# Install script for directory: /home/kyle/nasa/dev/HDTN/common/induct_manager

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kyle/nasa/dev/HDTN/rtp/common/induct_manager/libinduct_manager_lib.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/HDTN" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/common/induct_manager/include/Induct.h"
    "/home/kyle/nasa/dev/HDTN/common/induct_manager/include/InductManager.h"
    "/home/kyle/nasa/dev/HDTN/common/induct_manager/include/LtpOverUdpInduct.h"
    "/home/kyle/nasa/dev/HDTN/common/induct_manager/include/StcpInduct.h"
    "/home/kyle/nasa/dev/HDTN/common/induct_manager/include/TcpclInduct.h"
    "/home/kyle/nasa/dev/HDTN/common/induct_manager/include/TcpclV4Induct.h"
    "/home/kyle/nasa/dev/HDTN/common/induct_manager/include/UdpInduct.h"
    "/home/kyle/nasa/dev/HDTN/rtp/common/induct_manager/induct_manager_lib_export.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/InductManagerLib/InductManagerLibTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/InductManagerLib/InductManagerLibTargets.cmake"
         "/home/kyle/nasa/dev/HDTN/rtp/common/induct_manager/CMakeFiles/Export/lib/cmake/InductManagerLib/InductManagerLibTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/InductManagerLib/InductManagerLibTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/InductManagerLib/InductManagerLibTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/InductManagerLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/induct_manager/CMakeFiles/Export/lib/cmake/InductManagerLib/InductManagerLibTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/InductManagerLib" TYPE FILE FILES "/home/kyle/nasa/dev/HDTN/rtp/common/induct_manager/CMakeFiles/Export/lib/cmake/InductManagerLib/InductManagerLibTargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/InductManagerLib" TYPE FILE FILES
    "/home/kyle/nasa/dev/HDTN/rtp/common/induct_manager/InductManagerLibConfig.cmake"
    "/home/kyle/nasa/dev/HDTN/rtp/common/induct_manager/InductManagerLibConfigVersion.cmake"
    )
endif()


#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::UdpLib" for configuration "Release"
set_property(TARGET HDTN::UdpLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::UdpLib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libudp_lib.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::UdpLib )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::UdpLib "${_IMPORT_PREFIX}/lib/libudp_lib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

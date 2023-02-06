#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::UdpDelaySimLib" for configuration "Release"
set_property(TARGET HDTN::UdpDelaySimLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::UdpDelaySimLib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libudp_delay_sim_lib.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::UdpDelaySimLib )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::UdpDelaySimLib "${_IMPORT_PREFIX}/lib/libudp_delay_sim_lib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

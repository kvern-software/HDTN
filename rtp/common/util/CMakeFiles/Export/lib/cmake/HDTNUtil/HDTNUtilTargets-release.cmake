#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::HDTNUtil" for configuration "Release"
set_property(TARGET HDTN::HDTNUtil APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::HDTNUtil PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libhdtn_util.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::HDTNUtil )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::HDTNUtil "${_IMPORT_PREFIX}/lib/libhdtn_util.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

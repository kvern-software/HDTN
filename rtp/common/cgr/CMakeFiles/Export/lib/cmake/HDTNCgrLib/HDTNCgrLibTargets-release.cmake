#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::HDTNCgrLib" for configuration "Release"
set_property(TARGET HDTN::HDTNCgrLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::HDTNCgrLib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libcgr_lib.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::HDTNCgrLib )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::HDTNCgrLib "${_IMPORT_PREFIX}/lib/libcgr_lib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

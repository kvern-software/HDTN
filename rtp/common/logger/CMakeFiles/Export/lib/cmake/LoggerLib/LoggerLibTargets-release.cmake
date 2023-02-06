#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::LoggerLib" for configuration "Release"
set_property(TARGET HDTN::LoggerLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::LoggerLib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/liblog_lib.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::LoggerLib )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::LoggerLib "${_IMPORT_PREFIX}/lib/liblog_lib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

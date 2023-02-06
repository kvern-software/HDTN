#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::BpAppPatternsLib" for configuration "Release"
set_property(TARGET HDTN::BpAppPatternsLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::BpAppPatternsLib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libbp_app_patterns_lib.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::BpAppPatternsLib )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::BpAppPatternsLib "${_IMPORT_PREFIX}/lib/libbp_app_patterns_lib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

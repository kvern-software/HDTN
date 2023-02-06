#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::Bpcodec" for configuration "Release"
set_property(TARGET HDTN::Bpcodec APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::Bpcodec PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libbpcodec.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::Bpcodec )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::Bpcodec "${_IMPORT_PREFIX}/lib/libbpcodec.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

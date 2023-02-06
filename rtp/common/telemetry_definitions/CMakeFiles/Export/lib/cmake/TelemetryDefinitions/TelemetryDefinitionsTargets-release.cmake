#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::TelemetryDefinitions" for configuration "Release"
set_property(TARGET HDTN::TelemetryDefinitions APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::TelemetryDefinitions PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtelemetry_definitions.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::TelemetryDefinitions )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::TelemetryDefinitions "${_IMPORT_PREFIX}/lib/libtelemetry_definitions.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

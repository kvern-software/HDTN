#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HDTN::InductManagerLib" for configuration "Release"
set_property(TARGET HDTN::InductManagerLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(HDTN::InductManagerLib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libinduct_manager_lib.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS HDTN::InductManagerLib )
list(APPEND _IMPORT_CHECK_FILES_FOR_HDTN::InductManagerLib "${_IMPORT_PREFIX}/lib/libinduct_manager_lib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

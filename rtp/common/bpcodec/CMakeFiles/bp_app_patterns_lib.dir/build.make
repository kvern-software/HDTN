# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/kyle/nasa/dev/HDTN

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/kyle/nasa/dev/HDTN/rtp

# Include any dependencies generated for this target.
include common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/depend.make

# Include the progress variables for this target.
include common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/progress.make

# Include the compile flags for this target's objects.
include common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/flags.make

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.o: common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/flags.make
common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.o: ../common/bpcodec/src/app_patterns/BpSourcePattern.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/kyle/nasa/dev/HDTN/rtp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.o"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.o -c /home/kyle/nasa/dev/HDTN/common/bpcodec/src/app_patterns/BpSourcePattern.cpp

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.i"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/kyle/nasa/dev/HDTN/common/bpcodec/src/app_patterns/BpSourcePattern.cpp > CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.i

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.s"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/kyle/nasa/dev/HDTN/common/bpcodec/src/app_patterns/BpSourcePattern.cpp -o CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.s

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.o: common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/flags.make
common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.o: ../common/bpcodec/src/app_patterns/BpSinkPattern.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/kyle/nasa/dev/HDTN/rtp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.o"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.o -c /home/kyle/nasa/dev/HDTN/common/bpcodec/src/app_patterns/BpSinkPattern.cpp

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.i"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/kyle/nasa/dev/HDTN/common/bpcodec/src/app_patterns/BpSinkPattern.cpp > CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.i

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.s"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/kyle/nasa/dev/HDTN/common/bpcodec/src/app_patterns/BpSinkPattern.cpp -o CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.s

# Object files for target bp_app_patterns_lib
bp_app_patterns_lib_OBJECTS = \
"CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.o" \
"CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.o"

# External object files for target bp_app_patterns_lib
bp_app_patterns_lib_EXTERNAL_OBJECTS =

common/bpcodec/libbp_app_patterns_lib.a: common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSourcePattern.cpp.o
common/bpcodec/libbp_app_patterns_lib.a: common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/src/app_patterns/BpSinkPattern.cpp.o
common/bpcodec/libbp_app_patterns_lib.a: common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/build.make
common/bpcodec/libbp_app_patterns_lib.a: common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/kyle/nasa/dev/HDTN/rtp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libbp_app_patterns_lib.a"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && $(CMAKE_COMMAND) -P CMakeFiles/bp_app_patterns_lib.dir/cmake_clean_target.cmake
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/bp_app_patterns_lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/build: common/bpcodec/libbp_app_patterns_lib.a

.PHONY : common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/build

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/clean:
	cd /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec && $(CMAKE_COMMAND) -P CMakeFiles/bp_app_patterns_lib.dir/cmake_clean.cmake
.PHONY : common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/clean

common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/depend:
	cd /home/kyle/nasa/dev/HDTN/rtp && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/kyle/nasa/dev/HDTN /home/kyle/nasa/dev/HDTN/common/bpcodec /home/kyle/nasa/dev/HDTN/rtp /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec /home/kyle/nasa/dev/HDTN/rtp/common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : common/bpcodec/CMakeFiles/bp_app_patterns_lib.dir/depend


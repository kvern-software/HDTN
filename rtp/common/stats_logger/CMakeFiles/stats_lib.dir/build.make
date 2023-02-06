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
include common/stats_logger/CMakeFiles/stats_lib.dir/depend.make

# Include the progress variables for this target.
include common/stats_logger/CMakeFiles/stats_lib.dir/progress.make

# Include the compile flags for this target's objects.
include common/stats_logger/CMakeFiles/stats_lib.dir/flags.make

common/stats_logger/CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.o: common/stats_logger/CMakeFiles/stats_lib.dir/flags.make
common/stats_logger/CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.o: ../common/stats_logger/src/StatsLogger.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/kyle/nasa/dev/HDTN/rtp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object common/stats_logger/CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.o"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.o -c /home/kyle/nasa/dev/HDTN/common/stats_logger/src/StatsLogger.cpp

common/stats_logger/CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.i"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/kyle/nasa/dev/HDTN/common/stats_logger/src/StatsLogger.cpp > CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.i

common/stats_logger/CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.s"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/kyle/nasa/dev/HDTN/common/stats_logger/src/StatsLogger.cpp -o CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.s

# Object files for target stats_lib
stats_lib_OBJECTS = \
"CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.o"

# External object files for target stats_lib
stats_lib_EXTERNAL_OBJECTS =

common/stats_logger/libstats_lib.a: common/stats_logger/CMakeFiles/stats_lib.dir/src/StatsLogger.cpp.o
common/stats_logger/libstats_lib.a: common/stats_logger/CMakeFiles/stats_lib.dir/build.make
common/stats_logger/libstats_lib.a: common/stats_logger/CMakeFiles/stats_lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/kyle/nasa/dev/HDTN/rtp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libstats_lib.a"
	cd /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger && $(CMAKE_COMMAND) -P CMakeFiles/stats_lib.dir/cmake_clean_target.cmake
	cd /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/stats_lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
common/stats_logger/CMakeFiles/stats_lib.dir/build: common/stats_logger/libstats_lib.a

.PHONY : common/stats_logger/CMakeFiles/stats_lib.dir/build

common/stats_logger/CMakeFiles/stats_lib.dir/clean:
	cd /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger && $(CMAKE_COMMAND) -P CMakeFiles/stats_lib.dir/cmake_clean.cmake
.PHONY : common/stats_logger/CMakeFiles/stats_lib.dir/clean

common/stats_logger/CMakeFiles/stats_lib.dir/depend:
	cd /home/kyle/nasa/dev/HDTN/rtp && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/kyle/nasa/dev/HDTN /home/kyle/nasa/dev/HDTN/common/stats_logger /home/kyle/nasa/dev/HDTN/rtp /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger /home/kyle/nasa/dev/HDTN/rtp/common/stats_logger/CMakeFiles/stats_lib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : common/stats_logger/CMakeFiles/stats_lib.dir/depend


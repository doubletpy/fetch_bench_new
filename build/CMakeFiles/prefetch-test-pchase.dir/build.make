# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

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
CMAKE_COMMAND = /usr/local/cmake/bin/cmake

# The command to remove a file.
RM = /usr/local/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hygon/wangzichen/FetchBench-main/fetchbench

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hygon/wangzichen/FetchBench-main/fetchbench/build

# Include any dependencies generated for this target.
include CMakeFiles/prefetch-test-pchase.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/prefetch-test-pchase.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/prefetch-test-pchase.dir/flags.make

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o: CMakeFiles/prefetch-test-pchase.dir/flags.make
CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o: ../src/pchase/pchase_x86.S
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hygon/wangzichen/FetchBench-main/fetchbench/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building ASM object CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o"
	/usr/bin/cc $(ASM_DEFINES) $(ASM_INCLUDES) $(ASM_FLAGS) -o CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o -c /home/hygon/wangzichen/FetchBench-main/fetchbench/src/pchase/pchase_x86.S

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.requires:

.PHONY : CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.requires

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.provides: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.requires
	$(MAKE) -f CMakeFiles/prefetch-test-pchase.dir/build.make CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.provides.build
.PHONY : CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.provides

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.provides.build: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o


CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o: CMakeFiles/prefetch-test-pchase.dir/flags.make
CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o: ../src/pchase/pchase.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hygon/wangzichen/FetchBench-main/fetchbench/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o   -c /home/hygon/wangzichen/FetchBench-main/fetchbench/src/pchase/pchase.c

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/hygon/wangzichen/FetchBench-main/fetchbench/src/pchase/pchase.c > CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.i

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/hygon/wangzichen/FetchBench-main/fetchbench/src/pchase/pchase.c -o CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.s

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.requires:

.PHONY : CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.requires

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.provides: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.requires
	$(MAKE) -f CMakeFiles/prefetch-test-pchase.dir/build.make CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.provides.build
.PHONY : CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.provides

CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.provides.build: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o


# Object files for target prefetch-test-pchase
prefetch__test__pchase_OBJECTS = \
"CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o" \
"CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o"

# External object files for target prefetch-test-pchase
prefetch__test__pchase_EXTERNAL_OBJECTS =

prefetch-test-pchase: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o
prefetch-test-pchase: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o
prefetch-test-pchase: CMakeFiles/prefetch-test-pchase.dir/build.make
prefetch-test-pchase: CMakeFiles/prefetch-test-pchase.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hygon/wangzichen/FetchBench-main/fetchbench/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable prefetch-test-pchase"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/prefetch-test-pchase.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/prefetch-test-pchase.dir/build: prefetch-test-pchase

.PHONY : CMakeFiles/prefetch-test-pchase.dir/build

CMakeFiles/prefetch-test-pchase.dir/requires: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase_x86.S.o.requires
CMakeFiles/prefetch-test-pchase.dir/requires: CMakeFiles/prefetch-test-pchase.dir/src/pchase/pchase.c.o.requires

.PHONY : CMakeFiles/prefetch-test-pchase.dir/requires

CMakeFiles/prefetch-test-pchase.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/prefetch-test-pchase.dir/cmake_clean.cmake
.PHONY : CMakeFiles/prefetch-test-pchase.dir/clean

CMakeFiles/prefetch-test-pchase.dir/depend:
	cd /home/hygon/wangzichen/FetchBench-main/fetchbench/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hygon/wangzichen/FetchBench-main/fetchbench /home/hygon/wangzichen/FetchBench-main/fetchbench /home/hygon/wangzichen/FetchBench-main/fetchbench/build /home/hygon/wangzichen/FetchBench-main/fetchbench/build /home/hygon/wangzichen/FetchBench-main/fetchbench/build/CMakeFiles/prefetch-test-pchase.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/prefetch-test-pchase.dir/depend


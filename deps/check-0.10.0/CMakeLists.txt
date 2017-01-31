#
# Check: a unit test framework for C
#
# Copyright (C) 2011 Mateusz Loskot
# Copyright (C) 2001, 2002 Arien Malec
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
project(check C)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

macro(extract_version file setting_name)
  file(STRINGS ${file} VERSION_NUMBER REGEX "^${setting_name}")
  string(REPLACE "=" ";" VERSION_NUMBER_LIST ${VERSION_NUMBER})
  list(GET VERSION_NUMBER_LIST 1 ${setting_name})
endmacro(extract_version)

extract_version(configure.ac CHECK_MAJOR_VERSION)
extract_version(configure.ac CHECK_MINOR_VERSION)
extract_version(configure.ac CHECK_MICRO_VERSION)

set(CHECK_VERSION
  "${CHECK_MAJOR_VERSION}.${CHECK_MINOR_VERSION}.${CHECK_MICRO_VERSION}")

set(MEMORY_LEAKING_TESTS_ENABLED 1)

###############################################################################
# Set build features
set(CMAKE_BUILD_TYPE Debug)

###############################################################################
# Check system and architecture
if(WIN32)
  if(MSVC60)
    set(WINVER 0x0400)
  else()
    set(WINVER 0x0500)
  endif()
  set(_WIN32_WINNT ${WINVER})
endif(WIN32)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
endif(MSVC)

###############################################################################
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckStructMember)
include(CheckSymbolExists)
include(CheckTypeExists)
include(CheckTypeSize)

###############################################################################
# Check headers
set(INCLUDES "")
macro(ck_check_include_file header var)
  check_include_files("${INCLUDES};${header}" ${var})
  if(${var})
    set(INCLUDES ${INCLUDES} ${header})
  endif(${var})
endmacro(ck_check_include_file)

# Some FreeBSD headers assume sys/types.h was already included.
ck_check_include_file("sys/types.h" HAVE_SYS_TYPES_H)

# Alphabetize the rest unless there's a compelling reason
ck_check_include_file("errno.h" HAVE_ERRNO_H)
ck_check_include_file("inttypes.h" HAVE_INTTYPES_H)
ck_check_include_file("limits.h" HAVE_LIMITS_H)
ck_check_include_file("signal.h" HAVE_SIGNAL_H)
ck_check_include_file("stdarg.h" HAVE_STDARG_H)
ck_check_include_file("stdint.h" HAVE_STDINT_H)
ck_check_include_file("stdlib.h" HAVE_STDLIB_H)
ck_check_include_file("string.h" HAVE_STRING_H)
ck_check_include_file("strings.h" HAVE_STRINGS_H)
ck_check_include_file("sys/time.h" HAVE_SYS_TIME_H)
ck_check_include_file("time.h" HAVE_TIME_H)

###############################################################################
# Check functions
check_function_exists(fork HAVE_FORK)
check_function_exists(getline HAVE_GETLINE)
check_function_exists(getpid HAVE_GETPID)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(localtime_r HAVE_DECL_LOCALTIME_R)
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(mkstemp HAVE_MKSTEMP)
check_function_exists(realloc HAVE_REALLOC)
check_function_exists(setenv HAVE_DECL_SETENV)
check_function_exists(sigaction HAVE_SIGACTION)
check_function_exists(strdup HAVE_DECL_STRDUP)
check_function_exists(strsignal HAVE_DECL_STRSIGNAL)
check_function_exists(_getpid HAVE__GETPID)
check_function_exists(_strdup HAVE__STRDUP)

# printf related checks
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(vasnprintf HAVE_VASNPRINTF)
check_function_exists(asprintf HAVE_ASPRINTF)
check_function_exists(vasprintf HAVE_VASPRINTF)

if(NOT HAVE_SNPRINTF)
    add_definitions(-Dsnprintf=rpl_snprintf)
    set(snprintf rpl_snprintf)
    add_definitions(-Dvsnprintf=rpl_vsnprintf)
    set(vsnprintf rpl_vsnprintf)
    add_definitions(-Dvasnprintf=rpl_vasnprintf)
    set(vasnprintf rpl_vasnprintf)
    add_definitions(-Dasprintf=rpl_asprintf)
    set(asprintf rpl_asprintf)
    add_definitions(-Dvasprintf=rpl_vasprintf)
    set(vasprintf rpl_vasprintf)
else(NOT HAVE_SNPRINTF)
    set(HAVE_SNPRINTF 1)
    add_definitions(-DHAVE_SNPRINTF=1)
    set(HAVE_VSNPRINTF 1)
    add_definitions(-DHAVE_VSNPRINTF=1)
endif(NOT HAVE_SNPRINTF)

if(HAVE_FORK)
    add_definitions(-DHAVE_FORK=1)
    set(HAVE_FORK 1)
else(HAVE_FORK)
    add_definitions(-DHAVE_FORK=0)
    set(HAVE_FORK 0)
endif(HAVE_FORK)

###############################################################################
# Check defines
set(headers "limits.h")

if(HAVE_STDINT_H)
  list(APPEND headers "stdint.h")
endif(HAVE_STDINT_H)

if(HAVE_INTTYPES_H)
  list(APPEND headers "inttypes.h")
endif(HAVE_INTTYPES_H)

check_symbol_exists(INT64_MAX "${headers}" HAVE_INT64_MAX)
check_symbol_exists(INT64_MIN "${headers}" HAVE_INT64_MIN)
check_symbol_exists(UINT32_MAX "${headers}" HAVE_UINT32_MAX)
check_symbol_exists(UINT64_MAX "${headers}" HAVE_UINT64_MAX)
check_symbol_exists(SIZE_MAX "${headers}" HAVE_SIZE_MAX)
check_symbol_exists(SSIZE_MAX "limits.h"   HAVE_SSIZE_MAX)

###############################################################################
# Check struct members

# Check for  tv_sec in struct timeval 
if(NOT HAVE_SYS_TIME_H)
    if(MSVC)
        check_struct_member("struct timeval" tv_sec "Winsock2.h" HAVE_STRUCT_TIMEVAL_TV_SEC)
        check_struct_member("struct timeval" tv_usec "Winsock2.h" HAVE_STRUCT_TIMEVAL_TV_USEC)
        check_struct_member("struct timespec" tv_usec "Winsock2.h" HAVE_STRUCT_TIMESPEC_TV_SEC)
        check_struct_member("struct itimerspec" it_value "Winsock2.h" HAVE_STRUCT_ITIMERSPEC_IT_VALUE)

        if(NOT HAVE_STRUCT_TIMESPEC_TV_SEC)
            add_definitions(-DSTRUCT_TIMESPEC_DEFINITION_MISSING=1)
            set(STRUCT_TIMESPEC_DEFINITION_MISSING 1)
        endif(NOT HAVE_STRUCT_TIMESPEC_TV_SEC)
        
        if(NOT HAVE_STRUCT_ITIMERSPEC_IT_VALUE)
            add_definitions(-DSTRUCT_ITIMERSPEC_DEFINITION_MISSING=1)
            set(STRUCT_ITIMERSPEC_DEFINITION_MISSING 1)
        endif(NOT HAVE_STRUCT_ITIMERSPEC_IT_VALUE)
    endif(MSVC)
endif(NOT HAVE_SYS_TIME_H)


###############################################################################
# Check for integer types
check_type_size("short" SIZE_OF_SHORT)
check_type_size("int" SIZE_OF_INT)
check_type_size("long" SIZE_OF_LONG)
check_type_size("long long" SIZE_OF_LONG_LONG)

check_type_size("unsigned short" SIZE_OF_UNSIGNED_SHORT)
check_type_size("unsigned" SIZE_OF_UNSIGNED)
check_type_size("unsigned long" SIZE_OF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZE_OF_UNSIGNED_LONG_LONG)

check_type_size("__int64" __INT64)
check_type_size("unsigned __int64" UNSIGNED___INT64)

check_type_size(int16_t INT16_T) 
check_type_size(int32_t INT32_T)
check_type_size(int64_t INT64_T)
check_type_size(intmax_t INTMAX_T)
check_type_size(uint8_t UINT8_T) 
check_type_size(uint16_t UINT16_T) 
check_type_size(uint32_t UINT32_T) 
check_type_size(uint64_t UINT64_T)
check_type_size(uintmax_t UINTMAX_T)

#
set(CMAKE_EXTRA_INCLUDE_FILES time.h)
check_type_size(clock_t CLOCK_T)
if(NOT HAVE_CLOCK_T)
  set(clock_t int)
endif(NOT HAVE_CLOCK_T)
unset(CMAKE_EXTRA_INCLUDE_FILES)
#
set(CMAKE_EXTRA_INCLUDE_FILES time.h)
check_type_size(clockid_t CLOCKID_T)
if(NOT HAVE_CLOCKID_T)
  set(clockid_t int)
endif(NOT HAVE_CLOCKID_T)
unset(CMAKE_EXTRA_INCLUDE_FILES)
#
check_type_size(size_t SIZE_T)
if(NOT HAVE_SIZE_T)
  if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(size_t "uint64_t")
  else("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(size_t   "uint32_t")
  endif("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
endif(NOT HAVE_SIZE_T)
#
check_type_size(ssize_t SSIZE_T)
if(NOT HAVE_SSIZE_T)
  if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(ssize_t "int64_t")
  else("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(ssize_t "long")
  endif("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
endif(NOT HAVE_SSIZE_T)
#
check_type_size(pid_t PID_T)
if(NOT HAVE_PID_T)
  if(WIN32)
    set(pid_t "int")
  else(WIN32)
    MESSAGE(FATAL_ERROR "pid_t doesn't exist on this platform?")
  endif(WIN32)
endif(NOT HAVE_PID_T)
#
set(CMAKE_EXTRA_INCLUDE_FILES time.h)
check_type_size(timer_t TIMER_T)
if(NOT HAVE_TIMER_T)
  set(timer_t int)
endif(NOT HAVE_TIMER_T)
unset(CMAKE_EXTRA_INCLUDE_FILES)

###############################################################################
# Check libraries

check_library_exists(m floor "" HAVE_LIBM)
if (HAVE_LIBM)
    set (LIBM "m")
endif (HAVE_LIBM)

check_library_exists(rt clock_gettime "" HAVE_LIBRT)
if (HAVE_LIBRT)
    set(LIBRT "rt")
    ADD_DEFINITIONS(-DHAVE_LIBRT=1)
endif (HAVE_LIBRT)

check_library_exists(subunit subunit_test_start "" HAVE_SUBUNIT)
if (HAVE_SUBUNIT)
    set(SUBUNIT "subunit")
    set(ENABLE_SUBUNIT 1)
    add_definitions(-DENABLE_SUBUNIT=1)
else(HAVE_SUBUNIT)
    set(ENABLE_SUBUNIT 0)
    add_definitions(-DENABLE_SUBUNIT=0)
endif (HAVE_SUBUNIT)


###############################################################################
# Generate "config.h" from "cmake/config.h.cmake"
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DHAVE_CONFIG_H)
set(CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_stdint.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/check_stdint.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/check_stdint.h DESTINATION include)

###############################################################################
# Subdirectories
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)

###############################################################################
# Unit tests
enable_testing()
add_test(NAME check_check COMMAND check_check)

# Only offer to run shell scripts if we may have a working interpreter
if(UNIX OR MINGW OR MSYS)
    add_test(NAME test_output.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests COMMAND sh test_output.sh)
    add_test(NAME test_log_output.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests COMMAND sh test_log_output.sh)
    add_test(NAME test_xml_output.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests COMMAND sh test_xml_output.sh)
    add_test(NAME test_tap_output.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests COMMAND sh test_tap_output.sh)
    add_test(NAME test_check_nofork.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests COMMAND sh test_check_nofork.sh)
    add_test(NAME test_check_nofork_teardown.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests COMMAND sh test_check_nofork_teardown.sh)
endif(UNIX OR MINGW OR MSYS)

#
# Locate Intel Threading Building Blocks include paths and libraries
# CPPunit can be found at http://cppunit.sourceforge.net
# Written by Michael Hammer, michael _at_ derhammer.net

# This module defines
# TBB_INCLUDE_DIR, where to find ptlib.h, etc.
# TBB_LIBRARIES, the libraries to link against to use pwlib.
# TBB_FOUND, If false, don't try to use pwlib.

FIND_PATH(TBB_INCLUDE_DIR tbb/task_scheduler_init.h
    /usr/local/include
    /usr/include
    ${TBB_ROOT}
    ${TBB_ROOT}/include
    $ENV{TBB_ROOT}
    $ENV{TBB_ROOT}/include
    # ${CMAKE_SOURCE_DIR}/dep/tbb/include
)

FIND_LIBRARY(TBB_LIBRARIES
  NAMES
    tbb
  PATHS
    /usr/local/lib
    /usr/lib
    ${TBB_ROOT}
    ${TBB_ROOT}/lib
    $ENV{TBB_ROOT}
    $ENV{TBB_ROOT}/lib
    # ${CMAKE_SOURCE_DIR}/dep/tbb/build/vsproject/ia32/Release
)

FIND_LIBRARY(TBB_EXTRA_LIBRARIES
  NAMES
    tbbmalloc
  PATHS
    /usr/local/lib
    /usr/lib
    ${TBB_ROOT}
    ${TBB_ROOT}/lib
    $ENV{TBB_ROOT}
    $ENV{TBB_ROOT}/lib
    # ${CMAKE_SOURCE_DIR}/dep/tbb/build/vsproject/ia32/Release
)

FIND_LIBRARY(TBB_LIBRARIES_DEBUG
  NAMES
    tbb_debug
  PATHS
    /usr/local/lib
    /usr/lib
    ${TBB_ROOT}
    ${TBB_ROOT}/lib
    $ENV{TBB_ROOT}
    $ENV{TBB_ROOT}/lib
    # ${CMAKE_SOURCE_DIR}/dep/tbb/build/vsproject/ia32/Debug
)

FIND_LIBRARY(TBB_EXTRA_LIBRARIES_DEBUG
  NAMES
    tbbmalloc_debug
  PATHS
    /usr/local/lib
    /usr/lib
    ${TBB_ROOT}
    ${TBB_ROOT}/lib
    $ENV{TBB_ROOT}
    $ENV{TBB_ROOT}/lib
    # ${CMAKE_SOURCE_DIR}/dep/tbb/build/vsproject/ia32/Debug
)

SET(TBB_FOUND 0)
IF(TBB_INCLUDE_DIR)
  IF(TBB_LIBRARIES)
    SET(TBB_FOUND 1)
    MESSAGE(STATUS "Found Intel TBB")
    SET(TBB_LIBRARIES
      ${TBB_LIBRARIES}
      ${TBB_EXTRA_LIBRARIES}
    )
  ENDIF(TBB_LIBRARIES)
ENDIF(TBB_INCLUDE_DIR)

MARK_AS_ADVANCED(
  TBB_INCLUDE_DIR
  TBB_LIBRARIES
  TBB_EXTRA_LIBRARIES
  TBB_LIBRARIES_DEBUG
  TBB_EXTRA_LIBRARIES_DEBUG
)

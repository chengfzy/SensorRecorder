# Find package module for Ueye(iDS Camera) library.
#
# The following variables are set by this module:
#
#   Ueye_FOUND: TRUE if Ueye is found.
#   Ueye_INCLUDE_DIRS: Include directories for Ueye.
#   Ueye_LIBRARIES: Libraries required to link Ueye.
#
# The following variables control the behavior of this module:
#
# Ueye_INCLUDE_DIR_HINTS: List of additional directories in which to search for Ueye includes.
# Ueye_LIBRARY_DIR_HINTS: List of additional directories in which to search for Ueye libraries.

set(Ueye_INCLUDE_DIR_HINTS "" CACHE PATH "Ueye(iDS Camera) include directory")
set(Ueye_LIBRARY_DIR_HINTS "" CACHE PATH "Ueye(iDS Camera) library directory")

unset(Ueye_FOUND)
unset(Ueye_INCLUDE_DIRS)
unset(Ueye_LIBRARIES)

include(FindPackageHandleStandardArgs)

list(APPEND Ueye_CHECK_INCLUDE_DIRS
        /usr/local/include
        /usr/local/homebrew/include
        /opt/local/var/macports/software
        /opt/local/include
        /usr/include)
list(APPEND Ueye_CHECK_PATH_SUFFIXES)

list(APPEND Ueye_CHECK_LIBRARY_DIRS
        /usr/local/lib
        /usr/local/homebrew/lib
        /opt/local/lib
        /usr/lib)
list(APPEND Ueye_CHECK_LIBRARY_SUFFIXES)

find_path(Ueye_INCLUDE_DIRS
        NAMES
        ueye.h
        PATHS
        ${Ueye_INCLUDE_DIR_HINTS}
        ${Ueye_CHECK_INCLUDE_DIRS}
        PATH_SUFFIXES
        ${Ueye_CHECK_PATH_SUFFIXES})
find_library(Ueye_LIBRARIES
        NAMES
        ueye_api
        libueye_api
        PATHS
        ${Ueye_LIBRARY_DIR_HINTS}
        ${Ueye_CHECK_LIBRARY_DIRS}
        PATH_SUFFIXES
        ${Ueye_CHECK_LIBRARY_SUFFIXES})

if (Ueye_INCLUDE_DIRS AND Ueye_LIBRARIES)
    set(Ueye_FOUND TRUE)
    set(Ueye_LIBRARIES ${Ueye_LIBRARIES})
    message(STATUS "Found Ueye(iDS Camera)")
    message(STATUS "  Includes: ${Ueye_INCLUDE_DIRS}")
    message(STATUS "  Libraries: ${Ueye_LIBRARIES}")
else ()
    set(Ueye_FOUND FALSE)
    if (Ueye_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find Ueye")
    endif ()
endif ()

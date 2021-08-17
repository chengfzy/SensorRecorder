# Find package module for ZED Open Capture library.
#
# ZED Open Capture library(https://github.com/stereolabs/zed-open-capture) is the low level camera driver for the ZED 
# stereo camera family, which could not depends on CUDA
#
#
# The following variables are set by this module:
#
#   ZedOpen_FOUND: TRUE if ZED open capture library is found.
#   ZedOpen_INCLUDE_DIRS: Include directories for ZED open capture library.
#   ZedOpen_LIBRARIES: Libraries required to link ZED open capture library.
#
# The following variables control the behavior of this module:
#
# ZedOpen_INCLUDE_DIR_HINTS: List of additional directories in which to search for ZedOpen includes.
# ZedOpen_LIBRARY_DIR_HINTS: List of additional directories in which to search for ZedOpen libraries.

set(ZedOpen_INCLUDE_DIR_HINTS "" CACHE PATH "ZedOpen(iDS Camera) include directory")
set(ZedOpen_LIBRARY_DIR_HINTS "" CACHE PATH "ZedOpen(iDS Camera) library directory")

unset(ZedOpen_FOUND)
unset(ZedOpen_INCLUDE_DIRS)
unset(ZedOpen_LIBRARIES)

include(FindPackageHandleStandardArgs)

# include check dirs and suffixes
list(APPEND ZedOpen_CHECK_INCLUDE_DIRS
    /usr/local/include
    /usr/local/homebrew/include
    /opt/local/var/macports/software
    /usr/include
)
list(APPEND ZedOpen_CHECK_PATH_SUFFIXES)

# lib check dirs and suffixes
list(APPEND ZedOpen_CHECK_LIBRARY_DIRS
    /usr/local/lib
    /usr/local/homebrew/lib
    /usr/lib
)
list(APPEND ZedOpen_CHECK_LIBRARY_SUFFIXES)

# find ZED include dir and libs
find_path(ZedOpen_INCLUDE_DIRS
    NAMES
    zed-open-capture/sensorcapture.hpp
    PATHS
    ${ZedOpen_INCLUDE_DIR_HINTS}
    ${ZedOpen_CHECK_INCLUDE_DIRS}
    PATH_SUFFIXES
    ${ZedOpen_CHECK_PATH_SUFFIXES}
)
find_library(ZedOpen_LIBRARIES
    NAMES
    zed_open_capture
    libzed_open_capture
    PATHS
    ${ZedOpen_LIBRARY_DIR_HINTS}
    ${ZedOpen_CHECK_LIBRARY_DIRS}
    PATH_SUFFIXES
    ${ZedOpen_CHECK_LIBRARY_SUFFIXES}
)

# find libusb-1.0
find_library(USB1_LIBRARIES
    NAMES
    usb-1.0
    libzed_open_capture
    PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
)
# find hidapi
find_path(HIDAPI_INCLUDE_DIRS
    NAMES
    hidapi.h
    PATHS
    /usr/local/include
    /usr/local/homebrew/include
    /opt/local/var/macports/software
    /usr/include
    PATH_SUFFIXES
    hidapi
)
find_library(HIDAPI_LIBRARIES
    NAMES
    hidapi-libusb
    libhidapi-libusb
    PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
)

if (ZedOpen_INCLUDE_DIRS AND ZedOpen_LIBRARIES AND USB1_LIBRARIES AND HIDAPI_INCLUDE_DIRS AND HIDAPI_LIBRARIES)
    set(ZedOpen_FOUND TRUE)
    list(APPEND ZedOpen_INCLUDE_DIRS ${HIDAPI_INCLUDE_DIRS})
    list(APPEND ZedOpen_LIBRARIES ${USB1_LIBRARIES} ${HIDAPI_LIBRARIES})
    set(ZedOpen_LIBS ${ZedOpen_LIBRARIES})
    # add definitions
    add_definitions(-DSENSOR_LOG_AVAILABLE)
    add_definitions(-DSENSORS_MOD_AVAILABLE)
    add_definitions(-DVIDEO_MOD_AVAILABLE)
    
    message(STATUS "Found ZED Open Capture library")
    message(STATUS "  Includes: ${ZedOpen_INCLUDE_DIRS}")
    message(STATUS "  Libraries: ${ZedOpen_LIBRARIES}")
else ()
    set(ZedOpen_FOUND FALSE)
    if (ZedOpen_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find ZED Open Capture library")
    endif ()
endif ()

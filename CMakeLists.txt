cmake_minimum_required(VERSION 3.8)
project(libra LANGUAGES CXX VERSION 1.1.0)
set(CMAKE_CXX_STANDARD 17)

# cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(${PROJECT_SOURCE_DIR}/cmake/CMakeHelper.cmake)

# enable test
enable_testing()

###############################################################################
# Options
###############################################################################
option(DebugTest "Enable debug test info" OFF)
option(BuildTest "Build testing code" OFF)

# default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

###############################################################################
# Libraries
###############################################################################
# find packages for basic libraries
find_package(Gflags REQUIRED)                                   # gflags
find_package(Glog REQUIRED)                                     # glog
find_package(Boost COMPONENTS filesystem date_time REQUIRED)    # boost
find_package(OpenCV REQUIRED)                                   # OpenCV

# dependency include directories and libraries
list(APPEND DEPEND_INCLUDES
    ${GFLAGS_INCLUDE_DIRS}
    ${GLOG_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    )
list(APPEND DEPEND_LIBS
    ${GFLAGS_LIBRARIES}
    ${GLOG_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBRARIES}
    )

# find MyntEye camera library
find_package(mynteyed)      # MyntEye Depth
find_package(TurboJpeg)     # turbojpeg
if (${mynteyed_FOUND} AND ${TurboJpeg_FOUND})
    message(STATUS "Build with MYNT EYE Depth Camera")
    set(WithMyntEyeD ON)
    add_definitions(-DWITH_MYNTEYE_DEPTH)
    # when SDK build with OpenCV, add WITH_OPENCV macro to enable some features depending on OpenCV, such as ToMat().
    if (mynteyed_WITH_OPENCV)
        add_definitions(-DWITH_OPENCV)
    endif ()
    # add dependency
    list(APPEND DEPEND_INCLUDES
        ${TurboJpeg_INCLUDE_DIRS}
        )
    list(APPEND DEPEND_LIBS
        ${TurboJpeg_LIBRARIES}
        mynteye_depth
        )
else ()
    message(STATUS "Build without MYNT EYE Depth Camera")
    set(WithMyntEyeD OFF)
endif ()

# find ZED Open Capture library
find_package(ZedOpen)
if(${ZedOpen_FOUND} AND ${TurboJpeg_FOUND})
    message(STATUS "Build with ZED Camera(Open Capture library)")
    set(WithZedOpen ON)
    add_definitions(-DWITH_ZED_OPEN)
    # add dependency
    list(APPEND DEPEND_INCLUDES
        ${TurboJpeg_INCLUDE_DIRS}
        ${ZedOpen_INCLUDE_DIRS}
        )
    list(APPEND DEPEND_LIBS
        ${TurboJpeg_LIBRARIES}
        ${ZedOpen_LIBRARIES}
        )
else()
    message(STATUS "Build without ZED Camera(Open Capture library)")
    set(WithZedOpen OFF)
endif()

# build testing
if(BuildTest)
    # find gtest
    find_package(GTest)
    if(GTest_FOUND)
        # add dependency
        list(APPEND DEPEND_INCLUDES ${GTEST_INCLUDE_DIRS})
        list(APPEND DEPEND_LIBS ${GTEST_LIBRARIES})
    else()
        set(BuiltTest OFF)
    endif()
endif()
# print info
if(BuildTest)
    message(STATUS "Build testing code")
else()
    message(STATUS "Build without testing")
endif()

###############################################################################
# Project Setting
###############################################################################
# debug test
if (DebugTest)
    message(STATUS "Enable debug test info")
    add_definitions(-DDebugTest)
endif ()

# C++ standard
add_definitions(-DCxxStd=${CMAKE_CXX_STANDARD})

# fmt: header only library
add_definitions(-DFMT_HEADER_ONLY)

# export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# projects build setting
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

###############################################################################
# Project
###############################################################################
# third party include
list(APPEND DEPEND_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/eigen3
    )

# add modules
add_subdirectory(modules)
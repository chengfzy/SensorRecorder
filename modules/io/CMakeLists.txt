# IO module, data recorder, reader and saver
project(io)


# project files
file(GLOB_RECURSE FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
file(GLOB_RECURSE FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)
# remove MyntEye include and src file
if (NOT ${WithMyntEyeD})
    list(REMOVE_ITEM FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/libra/io/MyntEyeRecorder.h)
    list(REMOVE_ITEM FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/MyntEyeRecorder.cpp)
endif ()
# remove ZED file
if(NOT ${WithZedOpen})
    list(REMOVE_ITEM FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/libra/io/ZedOpenRecorder.h)
    list(REMOVE_ITEM FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/ZedOpenRecorder.cpp)
endif()

# build target
add_library(${PROJECT_NAME} SHARED ${FILE_INCLUDES} ${FILE_SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPEND_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS} util core)
add_dependencies(${PROJECT_NAME} util core)
# target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${CMAKE_PROJECT_NAME}_${PROJECT_NAME}
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})

# build project
file(GLOB_RECURSE FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
file(GLOB_RECURSE FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)
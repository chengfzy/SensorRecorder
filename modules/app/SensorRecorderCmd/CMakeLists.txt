# SensorRecorder without GUI
# This app is just an example for MYNT-EYE camera
project(SensorRecorderCmd VERSION 1.0.0)   # App has its own version

# project files
file(GLOB_RECURSE FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
file(GLOB_RECURSE FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)
# build target
add_executable(${PROJECT_NAME} ${FILE_INCLUDES} ${FILE_SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${DEPEND_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS} util core io)
add_dependencies(${PROJECT_NAME} util core io)
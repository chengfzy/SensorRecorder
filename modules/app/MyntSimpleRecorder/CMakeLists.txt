# MYNT-EYE Simple Recorder
project(MyntSimpleRecorder VERSION 1.0.0) 

# build target
add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${DEPEND_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS} util core io)
add_dependencies(${PROJECT_NAME} util core io)
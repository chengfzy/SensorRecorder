# ZED SensorRecorder without GUI, using ZED Open Capture library
project(ZedSensorRecorder VERSION 1.0.0)   # App has its own version

# build target
add_executable(${PROJECT_NAME} ${FILE_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${DEPEND_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS} util core io)
add_dependencies(${PROJECT_NAME} util core io)
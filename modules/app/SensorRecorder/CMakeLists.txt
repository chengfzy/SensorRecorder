# SensorRecorder: record IMU and camera simulataneously
project(SensorRecorder VERSION 1.1.1)   # App has its own version

# Qt project setting
set(CMAKE_AUTOMOC ON)   # instruct cmake to run moc automacically when needed
set(CMAKE_AUTOUIC ON)   # create code from a list of Qt designer ui files
set(CMAKE_AUTORCC ON)   # create code from a resource files

# project files
file(GLOB_RECURSE FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
file(GLOB_RECURSE FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)
file(GLOB_RECURSE FILE_RESS ${CMAKE_CURRENT_SOURCE_DIR}/res/*.qrc)
# remove iDS include and src file
if (NOT ${WithIDS})
    list(REMOVE_ITEM FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/IdsCameraWidget.h)
    list(REMOVE_ITEM FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/IdsCameraWidget.cpp)
endif ()
# remove MyntEye include and src file
if (NOT ${WithMyntEyeD})
    list(REMOVE_ITEM FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/MyntEyeWidget.h)
    list(REMOVE_ITEM FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/MyntEyeWidget.cpp)
endif ()

# build target
add_executable(${PROJECT_NAME} ${FILE_INCLUDES} ${FILE_SRCS} ${FILE_RESS})
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${DEPEND_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS} util core io qt)
add_dependencies(${PROJECT_NAME} util core io qt)
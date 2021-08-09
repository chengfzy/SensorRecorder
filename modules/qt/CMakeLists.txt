# qt utility module
project(qt)

# Qt project setting
set(CMAKE_AUTOMOC ON)   # instruct cmake to run moc automacically when needed
set(CMAKE_AUTOUIC ON)   # create code from a list of Qt designer ui files
set(CMAKE_AUTORCC ON)   # create code from a resource files

# project files
file(GLOB_RECURSE FILE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
file(GLOB_RECURSE FILE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)
file(GLOB_RECURSE FILE_RESS ${CMAKE_CURRENT_SOURCE_DIR}/res/*.qrc)

# build target
add_library(${PROJECT_NAME} SHARED ${FILE_INCLUDES} ${FILE_SRCS} ${FILE_RESS})
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPEND_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS})
# target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${CMAKE_PROJECT_NAME}_${PROJECT_NAME}
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})
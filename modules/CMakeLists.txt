# project include
list(APPEND DEPEND_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/util/include
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/io/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cv/include
    )
if (${WithQt})
    list(APPEND DEPEND_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/qt/include)
endif ()

# libraries
add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(io)

# qt libraries
if (${WithQt})
    add_subdirectory(qt)
endif ()

# applications
add_subdirectory(app)
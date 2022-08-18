# project include
list(APPEND DEPEND_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/util/include
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/io/include
    )

# libraries
add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(io)

# test
if(BuildTest)
    add_subdirectory(test)
endif()

# applications
add_subdirectory(app)
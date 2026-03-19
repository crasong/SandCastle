include(CTest)
#include(CMakePrintHelpers)
enable_testing()

# Define game engine sources
set(SOURCES 
    main.cpp
    Game.cpp
)

set(HEADERS 
    Game.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE )
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)

# Shader compilation - reruns whenever any .hlsl file changes
file(GLOB HLSL_SOURCES ${PROJECT_SOURCE_DIR}/Content/Shaders/Source/*.hlsl)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders.stamp
    DEPENDS ${HLSL_SOURCES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/Content/Shaders/Source
    COMMAND ${PROJECT_SOURCE_DIR}/Content/Shaders/Source/compile.sh
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/Content ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Content
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/shaders.stamp
    COMMENT "Compiling and deploying shaders..."
)

add_custom_target(Shaders DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders.stamp)
add_dependencies(${PROJECT_NAME} Shaders)

# Copy Content (for non-shader asset changes)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMENT "Copying Content..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/Content $<TARGET_FILE_DIR:${PROJECT_NAME}>/Content
)
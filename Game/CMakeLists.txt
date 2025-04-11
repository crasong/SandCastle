include(CTest)
#include(CMakePrintHelpers)
enable_testing()

# Define game engine sources
set(SOURCES 
    main.cpp
    Game.cpp
    Player.cpp
    Scene.cpp
    Components/BaseComponent.cpp
    Components/TransformComponent.cpp
)

set(HEADERS 
    Game.h
    GameObject.h
    Player.h
    Scene.h
    Components/BaseComponent.h
    Components/TransformComponent.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE )
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)

# Copy Content
#cmake_print_variables(PROJECT_SOURCE_DIR "$<CONFIGURATION>" CMAKE_CURRENT_BINARY_DIR)
Add_Custom_Command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMENT "Copying Content"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/Content $<TARGET_FILE_DIR:${PROJECT_NAME}>/Content
)
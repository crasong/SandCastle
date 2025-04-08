include(CTest)
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

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)
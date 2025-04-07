include(CTest)
enable_testing()

# Define game engine sources
set(SOURCES 
    main.cpp
    Engine.cpp
    Game.cpp
)

set(HEADERS 
    Engine.h 
    Game.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
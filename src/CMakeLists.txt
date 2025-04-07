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

# Needed to prevent SDL from overriding main()
#target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)

# Include SDL headers
#target_include_directories(${PROJECT_NAME} PRIVATE ${sdl3_SOURCE_DIR}/include)
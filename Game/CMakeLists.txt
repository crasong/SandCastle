include(CTest)
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

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)
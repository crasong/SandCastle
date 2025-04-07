add_library(Engine STATIC
            Engine.cpp
            Engine.h)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC vendor)
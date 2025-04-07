# Important: build SDL3 as a static lib
set(SDL3_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL3_FORCE_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

# Get Vulkan
find_package(Vulkan REQUIRED)

# Setup vendor interface
add_library(vendor INTERFACE)
target_link_libraries(vendor INTERFACE SDL3::SDL3)

# Add imgui
target_sources(vendor INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_internal.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_demo.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_draw.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_tables.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_widgets.cpp)
target_include_directories(vendor INTERFACE
                            ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/
                            ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/backends)

target_link_libraries(vendor INTERFACE Vulkan::Vulkan)
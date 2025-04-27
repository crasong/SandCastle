# Add SDL
# Important: build SDL3 as a dynamic lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

# Add SDL Image
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL_image)

# Add GLM
add_subdirectory(glm)

# Add Assimp
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL_PDB ON CACHE BOOL "" FORCE)
add_subdirectory(assimp)

# Setup vendor interface
add_library(vendor INTERFACE)

# Link Libraries
target_link_libraries(vendor INTERFACE 
        assimp
        glm::glm
        SDL3::SDL3
        SDL3_image::SDL3_image
        )

# Link imgui
target_sources(vendor INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imconfig.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_internal.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_demo.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_draw.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_tables.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/imgui_widgets.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/backends/imgui_impl_sdl3.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/backends/imgui_impl_sdl3.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/backends/imgui_impl_sdlgpu3.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/backends/imgui_impl_sdlgpu3.cpp)
target_include_directories(vendor INTERFACE
                            ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/
                            ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/backends)

target_compile_definitions(vendor INTERFACE
                GLM_ENABLE_EXPERIMENTAL
                GLM_FORCE_DEPTH_ZERO_TO_ONE
                GLM_FORCE_RADIANS
                IMGUI_DEFINE_MATH_OPERATORS
        )
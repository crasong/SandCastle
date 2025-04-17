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

# Get Vulkan
add_subdirectory(Vulkan-Headers)
add_subdirectory(volk)

# Add Assimp
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL_PDB ON CACHE BOOL "" FORCE)
add_subdirectory(assimp)

# Setup vendor interface
add_library(vendor INTERFACE)

# Link Libraries
target_link_libraries(vendor INTERFACE 
        SDL3::SDL3
        SDL3_image::SDL3_image
        glm::glm
        assimp
        Vulkan::Headers
        volk)

# Link imgui
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

target_compile_definitions(vendor INTERFACE 
        VK_NO_PROTOTYPES
        VOLK_IMPLEMENTATION
        VULKAN_HPP_NO_CONSTRUCTORS=1
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        )
cmake_minimum_required(VERSION 3.5.0)
project(SandCastle 
        VERSION 0.1.0 
        LANGUAGES C CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add vendor and source directories
add_subdirectory(vendor)
add_subdirectory(Engine)
add_subdirectory(Game)
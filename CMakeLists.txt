cmake_minimum_required(VERSION 3.5.0)
project(SandCastle 
        VERSION 0.1.0 
        LANGUAGES C CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor)
# Add your own source directory
add_subdirectory(src)
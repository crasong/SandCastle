cmake_minimum_required(VERSION 3.5.0)
project(SandCastle 
        VERSION 0.1.0 
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# By configuring CMake with -DDOWNLOAD_DEPENDENCIES=ON/OFF,
# users can choose between downloading dependencies or using system libraries
option(DOWNLOAD_DEPENDENCIES "Download dependencies" TRUE)

if(DOWNLOAD_DEPENDENCIES)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/external")
    set(FETCHCONTENT_QUIET FALSE)
    # Add SDL3 via FetchContent
    include(FetchContent)

    # Fetch SDL3 source
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG "main"  # or use "main"
        GIT_PROGRESS TRUE
    )
    
    # Important: build SDL3 as a static lib
    set(SDL3_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDL3_FORCE_STATIC ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(SDL3)
else()
    # find_package looks for already-installed system packages.
    # Configure with `-DCMAKE_PREFIX_PATH="/path/to/package1;/path/to/package2"`
    # to add search paths.
    find_package(SDL3 CONFIG REQUIRED)
endif()

# Add your own source directory
add_subdirectory(src)
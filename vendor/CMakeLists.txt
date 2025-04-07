# Important: build SDL3 as a static lib
set(SDL3_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL3_FORCE_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

add_library(vendor INTERFACE)
target_link_libraries(vendor INTERFACE SDL3::SDL3)
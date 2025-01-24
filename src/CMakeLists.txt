cmake_minimum_required(VERSION 3.5.0)
project(SandCastle_core VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

#add_executable(SandCastle main.cpp)

#set_property(TARGET SandCastle PROPERTY CXX_STANDARD_REQUIRED 23)

file(TO_NATIVE_PATH "${STAGED_INSTALL_PREFIX}/lib/cmake/glfw3" glfw3_DIR)
file(TO_NATIVE_PATH "${STAGED_INSTALL_PREFIX}/lib/cmake/glew" GLEW_DIR)

message(STATUS "glfw3_DIR is... ${glfw3_DIR}")
message(STATUS "GLEW_DIR is... ${GLEW_DIR}")

find_package(glfw3 REQUIRED CONFIG)

set(GLEW_USE_STATIC_LIBS ON)
find_package(GLEW REQUIRED CONFIG)

set(SRCS main.cpp)

if(APPLE)
    # create bundle on apple compiles
    add_executable(main MACOSX_BUNDLE ${SRCS})

    # Set a custom plist file for the app bundle - needed for Mac OS Retina display
    set_target_properties(main PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
elseif(WIN32)
    add_executable(main ${SRCS})
else()
    # No WIN32! We don't want WinMain() with GLFW.
    add_executable(main ${SRCS} main.exe.manifest)
endif()

target_link_libraries(main PRIVATE GLEW::glew_s glfw)
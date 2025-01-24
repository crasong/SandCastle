cmake_minimum_required(VERSION 3.5.0)
project(SandCastle VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(SandCastle main.cpp)

set_property(TARGET SandCastle PROPERTY CXX_STANDARD_REQUIRED 23)
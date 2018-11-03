cmake_minimum_required(VERSION 2.8)

project(dcsv)
file(GLOB_RECURSE sources "*.cpp" "*.h")
set(CMAKE_C_COMPILER "/usr/local/clang_6.0.0/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/clang_6.0.0/bin/clang++")
SET(CMAKE_CXX_FLAGS "-std=c++11")

add_executable(${PROJECT_NAME} ${sources})

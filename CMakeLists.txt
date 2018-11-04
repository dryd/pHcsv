cmake_minimum_required(VERSION 2.8)

project(pHcsv)
file(GLOB_RECURSE sources "*.cpp" "*.h")
set(CMAKE_C_COMPILER "/usr/local/clang_6.0.0/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/clang_6.0.0/bin/clang++")
SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS "-Wall")
SET(CMAKE_CXX_FLAGS "-Werror")

add_executable(${PROJECT_NAME} ${sources})

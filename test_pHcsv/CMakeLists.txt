cmake_minimum_required(VERSION 2.8)

project(test_pHcsv)
SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS "-Wall")
SET(CMAKE_CXX_FLAGS "-Werror")

file(GLOB_RECURSE test_sources "test_pHcsv.cpp")
add_executable(${PROJECT_NAME} ${test_sources})

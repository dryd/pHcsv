cmake_minimum_required(VERSION 2.8)

project(test_pHpool)
SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS "-Wall")
SET(CMAKE_CXX_FLAGS "-Werror")

file(GLOB_RECURSE test_sources "test_pHpool.cpp")
add_executable(${PROJECT_NAME} ${test_sources})
target_link_libraries(${PROJECT_NAME} pthread)

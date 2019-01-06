cmake_minimum_required(VERSION 2.8)

project(test_pH)

SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS "-Wall")
SET(CMAKE_CXX_FLAGS "-Werror")

add_definitions(-DTESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data")
include_directories(src)

add_executable("test_pHcsv" "test_pHcsv/test_pHcsv.cpp")

add_executable("test_pHcsvthread" "test_pHcsvthread/test_pHcsvthread.cpp")
target_link_libraries("test_pHcsvthread" pthread)

add_executable("test_pHpool" "test_pHpool/test_pHpool.cpp")
target_link_libraries("test_pHpool" pthread)

add_executable("test_pHad" "test_pHad/test_pHad.cpp")

add_executable("test_pHlp" "test_pHlp/test_pHlp.cpp")

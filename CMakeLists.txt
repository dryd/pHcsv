cmake_minimum_required(VERSION 2.8)

project(test_pH)

SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS "-Wall")
SET(CMAKE_CXX_FLAGS "-Werror")

add_definitions(-DTESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data")
include_directories(src)

file(GLOB test_pHcsv_sources "test_pHcsv/test_pHcsv.cpp")
add_executable("test_pHcsv" ${test_pHcsv_sources})

file(GLOB test_pHcsvthread_sources "test_pHcsvthread/test_pHcsvthread.cpp")
add_executable("test_pHcsvthread" ${test_pHcsvthread_sources})
target_link_libraries("test_pHcsvthread" pthread)

file(GLOB test_pHpool_sources "test_pHpool/test_pHpool.cpp")
add_executable("test_pHpool" ${test_pHpool_sources})
target_link_libraries("test_pHpool" pthread)

file(GLOB test_pHad_sources "test_pHad/test_pHad.cpp")
add_executable("test_pHad" ${test_pHad_sources})

cmake_minimum_required(VERSION 3.10)
project(gaden_decompress)

set(CMAKE_CXX_FLAGS "-w -O3 -g ${CMAKE_CXX_FLAGS}")

add_executable(decompress src/comp.cpp)
target_link_libraries(decompress gaden)
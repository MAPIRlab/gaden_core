cmake_minimum_required(VERSION 3.10)
project(OccupancyGridToSTL)

include_directories(
    include
)

add_executable(OccupancyGridToSTL src/OccupancyGridToSTL.cpp)
target_link_libraries(OccupancyGridToSTL gaden)

add_executable(AsciiToBin src/AsciiToBin.cpp)
target_link_libraries(AsciiToBin gaden)
cmake_minimum_required(VERSION 3.5)
project(gaden)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP)
find_package(fmt)
find_package(ZLIB)

include_directories(include)
include_directories(third_party/DDA/include)
include_directories(third_party/DDA/third_party/glm)

add_subdirectory(third_party/yaml-cpp)

add_library(gaden SHARED
    src/CoppeliaSim.cpp
    src/Environment.cpp
    src/EnvironmentConfiguration.cpp
    src/PlaybackSimulation.cpp
    src/Preprocessing.cpp
    src/EnvironmentConfigMetadata.cpp
    src/RunningSimulation.cpp
    src/Simulation.cpp
    src/WindSequence.cpp
)

target_link_libraries(gaden 
    yaml-cpp::yaml-cpp 
    OpenMP::OpenMP_CXX 
    fmt 
    ZLIB::ZLIB
)

# Utility executables for dealing with different file formats involved in gaden simulation 
add_subdirectory(utils/STL)
add_subdirectory(utils/decompress)
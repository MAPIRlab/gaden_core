cmake_minimum_required(VERSION 3.5)
project(gaden_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-glldb -O0")

# generate a python module with bindings to access the library from your python scripts
set(GENERATE_PYTHON_BINDINGS OFF) # requires cppyy. See the readme!

find_package(OpenMP)
find_package(fmt)
find_package(ZLIB)

add_subdirectory(third_party/yaml-cpp)

add_library(gaden SHARED
    src/CoppeliaSim.cpp
    src/Environment.cpp
    src/EnvironmentConfiguration.cpp
    src/Scene.cpp
    src/PlaybackSimulation.cpp
    src/Preprocessing.cpp
    src/EnvironmentConfigMetadata.cpp
    src/RunningSimulation.cpp
    src/Simulation.cpp
    src/WindSequence.cpp
)

target_include_directories(gaden PUBLIC
    include
    third_party/DDA/include
    third_party/DDA/third_party/glm
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


# Generate python bindings
if(GENERATE_PYTHON_BINDINGS)
    include(cmake/PythonBindings.cmake)
endif()
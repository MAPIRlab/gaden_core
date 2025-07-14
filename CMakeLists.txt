cmake_minimum_required(VERSION 3.5)
project(gaden_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-glldb -O0")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# generate a python module with bindings to access the library from your python scripts
set(GENERATE_PYTHON_BINDINGS OFF) # requires cppyy. See the readme!

find_package(OpenMP)
find_package(fmt)
find_package(ZLIB)
find_package(OpenCL)

add_subdirectory(third_party/yaml-cpp)
add_subdirectory(third_party/compute)

add_library(gaden SHARED
    src/CoppeliaSim.cpp
    src/MVSim.cpp
    src/Environment.cpp
    src/EnvironmentConfiguration.cpp
    src/Scene.cpp
    src/PlaybackSimulation.cpp
    src/Preprocessing.cpp
    src/EnvironmentConfigMetadata.cpp
    src/RunningSimulation.cpp
    src/Simulation.cpp
    src/GasSource.cpp
    src/WindSequence.cpp
    src/GPU/GPUAcceleration.cpp
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
    OpenCL::OpenCL
)

# Utility executables for dealing with different file formats involved in gaden simulation 
add_subdirectory(utils/STL)
add_subdirectory(utils/decompress)


# Generate python bindings
if(GENERATE_PYTHON_BINDINGS)
    include(cmake/PythonBindings.cmake)
endif()

# Compile with coppelia scene generation support
include(cmake/TryFindCoppelia.cmake)


#profiling
if(PROFILING)
    find_package(Tracy)
    target_compile_definitions(gaden PUBLIC TRACY_ENABLE)
    target_link_libraries(gaden Tracy::TracyClient)
endif()
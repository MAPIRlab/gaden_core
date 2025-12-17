cmake_minimum_required(VERSION 3.5)
project(gaden_core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# generate a python module with bindings to access the library from your python scripts
set(GENERATE_PYTHON_BINDINGS OFF) # requires cppyy. See the readme!
set(PROFILING OFF) # adds Tracy as a dependency

find_package(OpenMP REQUIRED)
find_package(fmt REQUIRED)
find_package(ZLIB REQUIRED)


set(CMAKE_DEBUG_POSTFIX "") # to prevent linking errors with yaml-cpp if compiling in debug mode for the first time
add_subdirectory(third_party/yaml-cpp)
add_subdirectory(third_party/compute)
set(BSC_BUILD_SHARED_LIB ON)
add_subdirectory(third_party/libbsc)

add_library(gaden SHARED
    src/CoppeliaSim.cpp
    src/MVSim.cpp
    src/Gazebo.cpp
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
    bsc::libbsc
)
target_include_directories(gaden PUBLIC third_party/libbsc)

# Utility executables for dealing with different file formats involved in gaden simulation 
add_subdirectory(utils/STL)
add_subdirectory(utils/decompress)

# Compile with coppelia scene generation support
include(cmake/TryFindCoppelia.cmake)



# Generate python bindings
if(GENERATE_PYTHON_BINDINGS)
    include(cmake/PythonBindings.cmake)
endif()

find_package(OpenCL QUIET)
if(OpenCL_FOUND)
    message("[gaden_core] Found OpenCL ${OpenCL_VERSION_MAJOR}.${OpenCL_VERSION_MINOR}. Compiling with GPU acceleration support.")
    target_compile_definitions(gaden PUBLIC GPU_ACCELERATION=1)
    target_link_libraries(gaden OpenCL::OpenCL)
endif()

if(PROFILING)
    find_package(Tracy REQUIRED)
    target_compile_definitions(gaden PUBLIC TRACY_ENABLE)
    target_link_libraries(gaden Tracy::TracyClient)
endif()
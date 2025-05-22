cmake_minimum_required(VERSION 3.5)
project(gaden)
set(CMAKE_CXX_STANDARD 20)


include_directories(include)
include_directories(third_party/DDA/include)
include_directories(third_party/DDA/third_party/glm)


add_library(gaden 
    src/Environment.cpp
    src/EnvironmentConfiguration.cpp
    src/PlaybackSimulation.cpp
    src/Preprocessing.cpp
    src/RunningSimulation.cpp
    src/Simulation.cpp
    src/WindSequence.cpp
)
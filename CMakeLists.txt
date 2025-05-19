cmake_minimum_required(VERSION 3.5)
project(gaden)

include_directories(include)
include_directories(third_party/DDA/include)
include_directories(third_party/DDA/third_party/glm)

add_library(gaden src/Environment.cpp)
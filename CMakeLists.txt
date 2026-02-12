cmake_minimum_required(VERSION 3.16)
project(feen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(feen
    include/feen/resonator.h
)

target_include_directories(feen PUBLIC include)

# Optional test executable
# add_executable(test sim/test_resonator.cpp)
# target_link_libraries(test feen)

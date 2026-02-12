cmake_minimum_required(VERSION 3.16)

project(feen 
    VERSION 3.0.0
    DESCRIPTION "A Wave-Native Physics-Informed Programming Substrate"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Define as an INTERFACE library (because it's header-only)
add_library(feen INTERFACE)

# 2. Tell CMake where the headers are
# This replaces the target_include_directories block
target_include_directories(feen 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 3. Physical Validation Testing Suite
option(FEEN_BUILD_TESTS "Build the physical validation suite" ON)

if(FEEN_BUILD_TESTS)
    enable_testing()

    add_executable(phys_test tests/test_resonator.cpp)
    
    # Link as INTERFACE
    target_link_libraries(phys_test PRIVATE feen)

    add_test(NAME ResonatorPhysicsValidation COMMAND phys_test)
endif()

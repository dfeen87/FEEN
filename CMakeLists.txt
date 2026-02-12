cmake_minimum_required(VERSION 3.16)

# Project Name: FEEN (Wave-Native Programming Language) 
project(feen 
    VERSION 3.0.0
    DESCRIPTION "A Wave-Native Physics-Informed Programming Substrate"
    LANGUAGES CXX
)

# 1. Project-wide C++ Standards
# FEEN requires C++17 for structured bindings and advanced physics modeling 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 2. Build Options
option(FEEN_BUILD_TESTS "Build the physical validation suite" ON)

# 3. Core Library: The Simulation Layer 
# This library implements the Wave Engine (damped harmonic oscillators) 
add_library(feen STATIC
    src/resonator.cpp
)

# 4. Include Directories
# Public includes for the 'feen/resonator.h' namespaced include
target_include_directories(feen 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 5. Physical Validation Testing Suite 
# Phase 3 of the Roadmap requires a Simulation Test Suite 
if(FEEN_BUILD_TESTS)
    enable_testing()

    # Create the physical validation runner
    add_executable(phys_test tests/test_resonator.cpp)
    
    # Link to the core wave engine
    target_link_libraries(phys_test PRIVATE feen)

    # Register tests for CTest / GitHub Actions
    # This test verifies: Decay logic, Bistability, and Spectral Isolation 
    add_test(NAME ResonatorPhysicsValidation COMMAND phys_test)
endif()

# 6. Documentation for the roadmap
message(STATUS "----------------------------------------------------")
message(STATUS "FEEN Project Configuration:")
message(STATUS "  - Status: Phase 3 (Simulation Layer) ")
message(STATUS "  - Target: Damped Wave Equation Implementation [cite: 22]")
message(STATUS "  - Testing: ${FEEN_BUILD_TESTS}")
message(STATUS "----------------------------------------------------")

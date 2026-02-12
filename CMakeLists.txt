cmake_minimum_required(VERSION 3.16)

project(feen
    VERSION 3.0.0
    DESCRIPTION "A Wave-Native Physics-Informed Programming Substrate"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# FEEN Core (Header-only)
# ---------------------------------------------------------------------------

add_library(feen INTERFACE)

target_include_directories(feen
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------------------
# Optional Dependencies (via vcpkg or system)
# ---------------------------------------------------------------------------

find_package(pybind11 CONFIG QUIET)

# ---------------------------------------------------------------------------
# Python Bindings (optional)
# ---------------------------------------------------------------------------

option(FEEN_BUILD_PYTHON "Build Python bindings (pyfeen)" ON)

if (FEEN_BUILD_PYTHON AND pybind11_FOUND)
    message(STATUS "Building Python bindings (pyfeen)")
    add_subdirectory(python)
elseif (FEEN_BUILD_PYTHON)
    message(WARNING "pybind11 not found — Python bindings disabled")
endif()

# ---------------------------------------------------------------------------
# Physical Validation Testing Suite
# ---------------------------------------------------------------------------

option(FEEN_BUILD_TESTS "Build the physical validation suite" ON)

if (FEEN_BUILD_TESTS)
    enable_testing()

    add_executable(phys_test tests/test_resonator.cpp)
    target_link_libraries(phys_test PRIVATE feen)

    add_test(
        NAME ResonatorPhysicsValidation
        COMMAND phys_test
    )
endif()

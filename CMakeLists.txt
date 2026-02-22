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
# Python Bindings (optional)
# ---------------------------------------------------------------------------

option(FEEN_BUILD_PYTHON "Build Python bindings (pyfeen)" ON)

if (FEEN_BUILD_PYTHON)
    # Python3 with Development.Module is required to compile extension modules.
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

    # Try standard cmake search paths first (vcpkg, system, user-installed).
    find_package(pybind11 CONFIG QUIET)

    # Fallback: locate pybind11 via the active Python interpreter.
    # This handles the common case where pybind11 was installed with pip.
    if (NOT pybind11_FOUND)
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
            OUTPUT_VARIABLE _pybind11_cmake_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE _pybind11_result
        )
        if (_pybind11_result EQUAL 0 AND _pybind11_cmake_dir)
            set(pybind11_DIR "${_pybind11_cmake_dir}")
            find_package(pybind11 CONFIG QUIET)
        endif()
    endif()

    # Hard failure: missing pybind11 is caught at configure time, not runtime.
    if (NOT pybind11_FOUND)
        message(FATAL_ERROR
            "FEEN_BUILD_PYTHON=ON but pybind11 was not found.\n"
            "Install pybind11 (e.g. 'pip install pybind11') and re-run cmake, "
            "or disable Python bindings with -DFEEN_BUILD_PYTHON=OFF.")
    endif()

    message(STATUS "Building Python bindings (pyfeen)")
    add_subdirectory(python)
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

    add_executable(spiral_time_test tests/test_spiral_time.cpp)
    target_link_libraries(spiral_time_test PRIVATE feen)

    add_test(
        NAME SpiralTimeObserverValidation
        COMMAND spiral_time_test
    )

    add_executable(hardware_adapter_test tests/test_hardware_adapter.cpp)
    target_link_libraries(hardware_adapter_test PRIVATE feen)

    add_test(
        NAME HardwareAdapterValidation
        COMMAND hardware_adapter_test
    )
endif()

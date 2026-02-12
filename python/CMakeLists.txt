pybind11_add_module(pyfeen
    pyfeen.cpp
)

target_link_libraries(pyfeen PRIVATE feen)

target_include_directories(pyfeen PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(pyfeen PROPERTIES
    OUTPUT_NAME "pyfeen"
)

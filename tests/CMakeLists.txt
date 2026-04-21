
enable_testing()

add_executable(ailee_metric_test test_ailee_metric.cpp)
target_link_libraries(ailee_metric_test PRIVATE feen)

add_test(
    NAME AileeMetricValidation
    COMMAND ailee_metric_test
)

add_executable(phys_test test_resonator.cpp)
target_link_libraries(phys_test PRIVATE feen)

add_test(
    NAME ResonatorPhysicsValidation
    COMMAND phys_test
)

add_executable(spiral_time_test test_spiral_time.cpp)
target_link_libraries(spiral_time_test PRIVATE feen)

add_test(
    NAME SpiralTimeObserverValidation
    COMMAND spiral_time_test
)

add_executable(hardware_adapter_test test_hardware_adapter.cpp)
target_link_libraries(hardware_adapter_test PRIVATE feen)

add_test(
    NAME HardwareAdapterValidation
    COMMAND hardware_adapter_test
)

add_executable(energy_mesh_test test_energy_mesh.cpp)
target_link_libraries(energy_mesh_test PRIVATE feen)

add_test(
    NAME EnergyMeshValidation
    COMMAND energy_mesh_test
)

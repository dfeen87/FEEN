
enable_testing()

add_executable(ailee_metric_test test_ailee_metric.cpp)
target_link_libraries(ailee_metric_test PRIVATE feen)

add_test(
    NAME AileeMetricValidation
    COMMAND ailee_metric_test
)

add_executable(resonator_network_test test_resonator_network.cpp)
target_link_libraries(resonator_network_test PRIVATE feen)

add_test(
    NAME ResonatorNetworkValidation
    COMMAND resonator_network_test
)

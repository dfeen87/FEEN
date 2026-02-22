
enable_testing()

add_executable(ailee_metric_test test_ailee_metric.cpp)
target_link_libraries(ailee_metric_test PRIVATE feen)

add_test(
    NAME AileeMetricValidation
    COMMAND ailee_metric_test
)

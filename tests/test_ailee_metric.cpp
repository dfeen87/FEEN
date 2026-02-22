
#include <feen/ailee/metric.h>
#include <iostream>
#include <cmath>
#include <cstdlib>

// Simple check macro to replace assert, ensuring tests run in Release mode
#define CHECK(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Check failed: " << message << "\n" \
                      << "File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

void test_initialization() {
    feen::ailee::AileeParams params{0.1, 1.0, 1.0, 1.0};
    feen::ailee::AileeMetric metric(params);
    CHECK(metric.delta_v() == 0.0, "Initial delta_v should be 0.0");
    std::cout << "Test 1: Initialization passed." << std::endl;
}

void test_single_step() {
    feen::ailee::AileeParams params{0.1, 1.0, 1.0, 1.0};
    feen::ailee::AileeMetric metric(params);
    feen::ailee::AileeSample sample{1.0, 0.0, 0.0, 1.0, 1.0};
    metric.integrate(sample);
    // delta_v = isp * eta * exp(-alpha * v0^2) * integral
    // integral = (p_input * exp(-alpha * w^2) * exp(2 * alpha * v0 * v) / mass) * dt
    // integral = (1.0 * exp(0) * exp(0) / 1.0) * 1.0 = 1.0
    // delta_v = 1.0 * 1.0 * exp(-0.1) * 1.0 = exp(-0.1)
    double expected = std::exp(-0.1);
    CHECK(std::abs(metric.delta_v() - expected) < 1e-6, "Single step delta_v mismatch");
    std::cout << "Test 2: Single step passed." << std::endl;
}

void test_overflow_protection() {
    feen::ailee::AileeParams params{1.0, 1.0, 1.0, 1.0};
    feen::ailee::AileeMetric metric(params);

    // Test clamping for very large positive exponent
    feen::ailee::AileeSample sample_large_pos{1.0, 0.0, 1000.0, 1.0, 1.0};
    // v=1000, 2*alpha*v0*v = 2000 > 700 (limit)
    metric.integrate(sample_large_pos);

    // Check if result is finite (not inf)
    CHECK(std::isfinite(metric.delta_v()), "Result should be finite (overflow protection)");

    metric.reset();

    // Test clamping for very large negative exponent (should approach zero but remain finite)
    feen::ailee::AileeSample sample_large_neg{1.0, 1000.0, 0.0, 1.0, 1.0};
    // w=1000, -alpha*w^2 = -1000000 < -700 (limit)
    metric.integrate(sample_large_neg);

    CHECK(std::isfinite(metric.delta_v()), "Result should be finite (underflow protection)");

    std::cout << "Test 3: Overflow protection passed." << std::endl;
}

int main() {
    test_initialization();
    test_single_step();
    test_overflow_protection();
    return 0;
}

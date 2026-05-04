#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <limits>

#include "domains/internet/InternetDomain.hpp"

// Simple testing framework since tests are run individually without CTest
#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Test failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            exit(1); \
        } \
    } while(0)

void test_severed_edge_failover() {
    std::cout << "Running test_severed_edge_failover..." << std::endl;

    FEEN::InternetDomain::HarmonicRouter router;

    feen::ResonatorConfig config;
    config.frequency_hz = 1000.0;
    config.q_factor = 200.0;

    // Add 3 nodes: source (0), target_1 (1), target_2 (2)
    router.add_node(config, 100.0, 10.0); // Node 0
    router.add_node(config, 100.0, 10.0); // Node 1
    router.add_node(config, 100.0, 10.0); // Node 2

    // Add edges
    router.add_edge(0, 1, 10.0); // Edge 0: 0 -> 1 (Length 10)
    router.add_edge(0, 2, 20.0); // Edge 1: 0 -> 2 (Length 20)

    // We want to test routing from 0 -> 2.
    // Path A: 0 -> 1 -> 2 (total edge length: 10 + 5 = 15)
    // Path B: 0 -> 2 (total edge length: 20)

    router.add_edge(1, 2, 5.0); // Edge 2: 1 -> 2 (Length 5)

    // Initially, path A (0 -> 1 -> 2) should be chosen because total length 15 < 20.
    // (Node impedances are equal).
    std::vector<std::size_t> path_initial = router.find_path(0, 2);
    CHECK(path_initial.size() == 3);
    CHECK(path_initial[0] == 0);
    CHECK(path_initial[1] == 1);
    CHECK(path_initial[2] == 2);

    // Sever edge 0 (0 -> 1)
    router.get_edge(0).is_severed = true;

    // Recalculate path. Edge 0 has infinite impedance, so it must route via path B (0 -> 2 directly).
    std::vector<std::size_t> failover_path = router.find_path(0, 2);
    CHECK(failover_path.size() == 2);
    CHECK(failover_path[0] == 0);
    CHECK(failover_path[1] == 2);

    std::cout << "test_severed_edge_failover passed!" << std::endl;
}

void test_ddos_mitigation() {
    std::cout << "Running test_ddos_mitigation..." << std::endl;

    feen::ResonatorConfig config;
    config.frequency_hz = 1000.0;
    config.q_factor = 200.0;

    FEEN::InternetDomain::RouterNode node(config, 1000.0, 50.0); // High bandwidth, low load

    // Initial impedance should be low (50 / 1000 = 0.05)
    CHECK(node.impedance() < 1.0);
    CHECK(!node.was_ddos_mitigated());

    // Inject massive noise (simulated DDoS)
    double parasitic_amp = 50.0;
    double parasitic_freq = 5000.0;

    // Evaluate traffic. The massive amplitude should artificially spike load by 50 * 1e6 = 50,000,000
    // Impedance becomes > 50,000. Threshold is 1e3.
    node.evaluate_inbound_traffic(parasitic_amp, parasitic_freq);

    // It should have neutralized the DDoS and cleared the load.
    CHECK(node.was_ddos_mitigated());
    CHECK(node.impedance() < 1.0); // Restored to normal

    std::cout << "test_ddos_mitigation passed!" << std::endl;
}

void test_ddos_load_not_corrupted_on_high_bandwidth() {
    std::cout << "Running test_ddos_load_not_corrupted_on_high_bandwidth..." << std::endl;

    feen::ResonatorConfig config;
    config.frequency_hz = 1000.0;
    config.q_factor = 200.0;

    // Very high bandwidth: impedance after simulated spike stays below threshold.
    FEEN::InternetDomain::RouterNode node(config, 1e15, 50.0);

    double initial_impedance = node.impedance();
    node.evaluate_inbound_traffic(50.0, 5000.0);

    // Load must not have been permanently corrupted even though threshold was not crossed.
    CHECK(std::abs(node.impedance() - initial_impedance) < 1e-10);
    CHECK(!node.was_ddos_mitigated());

    std::cout << "test_ddos_load_not_corrupted_on_high_bandwidth passed!" << std::endl;
}

void test_input_validation() {
    std::cout << "Running test_input_validation..." << std::endl;

    feen::ResonatorConfig config;
    config.frequency_hz = 1000.0;
    config.q_factor = 200.0;

    // RouterNode: invalid bandwidth
    try {
        FEEN::InternetDomain::RouterNode bad(config, 0.0, 10.0);
        std::cerr << "Expected invalid_argument for zero bandwidth\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    try {
        FEEN::InternetDomain::RouterNode bad(config, -5.0, 10.0);
        std::cerr << "Expected invalid_argument for negative bandwidth\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    try {
        FEEN::InternetDomain::RouterNode bad(config, std::numeric_limits<double>::infinity(), 10.0);
        std::cerr << "Expected invalid_argument for infinite bandwidth\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    // RouterNode: invalid load
    try {
        FEEN::InternetDomain::RouterNode bad(config, 100.0, -1.0);
        std::cerr << "Expected invalid_argument for negative load\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    try {
        FEEN::InternetDomain::RouterNode bad(config, 100.0, std::numeric_limits<double>::quiet_NaN());
        std::cerr << "Expected invalid_argument for NaN load\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    // HarmonicRouter::add_edge: invalid length
    FEEN::InternetDomain::HarmonicRouter router;
    router.add_node(config, 100.0, 10.0);
    router.add_node(config, 100.0, 10.0);

    try {
        router.add_edge(0, 1, 0.0);
        std::cerr << "Expected invalid_argument for zero edge length\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    try {
        router.add_edge(0, 1, -1.0);
        std::cerr << "Expected invalid_argument for negative edge length\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    try {
        router.add_edge(0, 1, std::numeric_limits<double>::quiet_NaN());
        std::cerr << "Expected invalid_argument for NaN edge length\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    // HarmonicRouter::tick: invalid dt
    try {
        router.tick(0.0);
        std::cerr << "Expected invalid_argument for zero tick dt\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    try {
        router.tick(-1e-3);
        std::cerr << "Expected invalid_argument for negative tick dt\n";
        exit(1);
    } catch (const std::invalid_argument&) {}

    std::cout << "test_input_validation passed!" << std::endl;
}

void test_tick_advances_state() {
    std::cout << "Running test_tick_advances_state..." << std::endl;

    feen::ResonatorConfig config;
    config.frequency_hz = 1000.0;
    config.q_factor = 200.0;

    FEEN::InternetDomain::HarmonicRouter router;
    router.add_node(config, 100.0, 10.0);

    // Inject energy so there is something to evolve.
    router.get_node(0).get_core().inject(1.0, 0.0);

    double x_before = router.get_node(0).get_core().x();
    double t_before = router.get_node(0).get_core().t();

    router.tick(1e-4);

    double x_after = router.get_node(0).get_core().x();
    double t_after = router.get_node(0).get_core().t();

    CHECK(std::isfinite(x_after));
    CHECK(std::abs(t_after - (t_before + 1e-4)) < 1e-12);
    // State should have evolved (position changed from injected value).
    CHECK(x_before != x_after);

    std::cout << "test_tick_advances_state passed!" << std::endl;
}

int main() {
    test_severed_edge_failover();
    test_ddos_mitigation();
    test_ddos_load_not_corrupted_on_high_bandwidth();
    test_input_validation();
    test_tick_advances_state();
    return 0;
}


#include <feen/network.h>
#include <iostream>
#include <cassert>
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

void test_network_creation() {
    feen::ResonatorNetwork net;
    CHECK(net.size() == 0, "Initial size should be 0");

    feen::ResonatorConfig cfg;
    cfg.name = "node0";
    cfg.frequency_hz = 1000.0;
    cfg.q_factor = 100.0;
    cfg.beta = 0.0;

    feen::Resonator r(cfg);
    net.add_node(r);

    CHECK(net.size() == 1, "Size should be 1 after add_node");
    std::cout << "Test 1: Creation passed." << std::endl;
}

void test_coupling() {
    feen::ResonatorNetwork net;
    feen::ResonatorConfig cfg;
    cfg.name = "node";
    cfg.frequency_hz = 1000.0;
    cfg.q_factor = 100.0;
    cfg.beta = 0.0;

    net.add_node(feen::Resonator(cfg));
    net.add_node(feen::Resonator(cfg));

    net.add_coupling(0, 1, 0.5);
    CHECK(std::abs(net.coupling(0, 1) - 0.5) < 1e-6, "Coupling should be 0.5");
    CHECK(net.coupling(1, 0) == 0.0, "Reverse coupling should be 0.0");

    net.set_coupling(0, 1, 1.0);
    CHECK(std::abs(net.coupling(0, 1) - 1.0) < 1e-6, "Coupling should be 1.0 after set");

    std::cout << "Test 2: Coupling passed." << std::endl;
}

void test_tick() {
    feen::ResonatorNetwork net;
    feen::ResonatorConfig cfg;
    cfg.name = "osc";
    cfg.frequency_hz = 1.0; // Slow for testing
    cfg.q_factor = 10.0;
    cfg.beta = 0.0;

    net.add_node(feen::Resonator(cfg));

    net.node(0).inject(1.0);
    double initial_x = net.node(0).x();

    net.tick_parallel(0.1);

    CHECK(net.time_s() > 0.0, "Time should advance");
    CHECK(net.node(0).x() != initial_x, "State should change");

    std::cout << "Test 3: Tick passed." << std::endl;
}

int main() {
    test_network_creation();
    test_coupling();
    test_tick();
    return 0;
}

#include <feen/resonator.h>
#include <feen/network.h>

#include <chrono>
#include <iostream>

using namespace feen;
using clock = std::chrono::high_resolution_clock;

void benchmark_rk4_speed() {
    ResonatorConfig cfg{"bench", 1e6, 1e4};
    Resonator r(cfg);
    r.inject(1e-6);

    auto start = clock::now();
    for (int i = 0; i < 1'000'000; ++i)
        r.tick(1e-9);
    auto end = clock::now();

    std::cout << "RK4 time: "
              << std::chrono::duration<double>(end - start).count()
              << " s\n";
}

void benchmark_network_scaling() {
    const int N = 128;
    ResonatorConfig cfg{"node", 1e6, 1e4};

    ResonatorNetwork net;
    for (int i = 0; i < N; ++i)
        net.add_node(Resonator(cfg));

    auto start = clock::now();
    for (int i = 0; i < 1000; ++i)
        net.tick_parallel(1e-9);
    auto end = clock::now();

    std::cout << "Network tick time: "
              << std::chrono::duration<double>(end - start).count()
              << " s\n";
}

void measure_cache_efficiency() {
    benchmark_network_scaling();
}

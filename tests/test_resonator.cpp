#include <iostream>
#include <cmath>
#include "../include/feen/resonator.h"

using namespace feen;

int main() {

    std::cout << "==== FEEN Resonator Test ====\n\n";

    // ---------------------------------------------------------------------
    // 1. Monostable Decay Test
    // ---------------------------------------------------------------------

    ResonatorConfig mono_cfg;
    mono_cfg.name = "mono";
    mono_cfg.frequency_hz = 1000.0;
    mono_cfg.q_factor = 200.0;
    mono_cfg.beta = 1e-4;

    Resonator mono(mono_cfg);

    mono.inject(1.0);

    double dt = 1e-6;
    for (int i = 0; i < 500000; ++i)
        mono.tick(dt);

    std::cout << "Monostable energy after decay: "
              << mono.total_energy() << "\n";
    std::cout << "SNR: " << mono.snr() << "\n\n";


    // ---------------------------------------------------------------------
    // 2. Bistable Equilibrium Test
    // ---------------------------------------------------------------------

    ResonatorConfig bi_cfg;
    bi_cfg.name = "bistable";
    bi_cfg.frequency_hz = 1000.0;
    bi_cfg.q_factor = 200.0;
    bi_cfg.beta = -1e8;  // Strong bistability

    Resonator bistable(bi_cfg);

    // Expected stable wells:
    // x* = ±ω₀ / sqrt(|β|)
    double omega0 = 2.0 * M_PI * bi_cfg.frequency_hz;
    double expected_well =
        omega0 / std::sqrt(std::abs(bi_cfg.beta));

    std::cout << "Expected well location: ±"
              << expected_well << "\n";

    bistable.inject(expected_well);

    for (int i = 0; i < 100000; ++i)
        bistable.tick(dt);

    std::cout << "Actual position after settling: "
              << bistable.total_energy() << "\n";

    std::cout << "Barrier height: "
              << bistable.barrier_height() << "\n";

    std::cout << "Switching time estimate: "
              << bistable.switching_time() << " s\n";

    std::cout << "Switching OK? "
              << (bistable.switching_time_ok() ? "YES" : "NO")
              << "\n\n";


    // ---------------------------------------------------------------------
    // 3. Spectral Isolation Test
    // ---------------------------------------------------------------------

    ResonatorConfig a_cfg;
    a_cfg.name = "A";
    a_cfg.frequency_hz = 1000.0;
    a_cfg.q_factor = 500;
    a_cfg.beta = 0;

    ResonatorConfig b_cfg = a_cfg;
    b_cfg.name = "B";
    b_cfg.frequency_hz = 1050.0;

    Resonator A(a_cfg);
    Resonator B(b_cfg);

    double iso = Resonator::isolation_db(A, B);

    std::cout << "Isolation (dB): " << iso << "\n";

    std::cout << "\n==== Test Complete ====\n";
}

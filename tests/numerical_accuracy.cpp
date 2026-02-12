#include <feen/resonator.h>
#include <feen/sim/integrators.h>

#include <cmath>
#include <iostream>

using namespace feen;

void test_energy_drift(double duration_s) {
    ResonatorConfig cfg{"drift", 1e6, 1e6};
    Resonator r(cfg);
    r.inject(1e-6);

    double E0 = r.total_energy();
    double dt = 1e-9;
    int steps = static_cast<int>(duration_s / dt);

    for (int i = 0; i < steps; ++i)
        r.tick(dt);

    double drift = std::abs(r.total_energy() - E0) / E0;
    std::cout << "Energy drift: " << drift << "\n";
}

void test_phase_error() {
    ResonatorConfig cfg{"phase", 1e6, 1e6};
    Resonator r(cfg);
    r.inject(1e-6);

    double dt = 1e-9;
    for (int i = 0; i < 10000; ++i)
        r.tick(dt);

    double expected_phase = TWO_PI * cfg.frequency_hz * r.t();
    double actual_phase = std::atan2(-r.v(), r.x());

    std::cout << "Phase error: "
              << std::abs(expected_phase - actual_phase) << "\n";
}

void compare_integrators() {
    ResonatorConfig cfg{"cmp", 1e6, 1e6};

    Resonator r1(cfg);
    Resonator r2(cfg);

    r1.inject(1e-6);
    r2.inject(1e-6);

    RK45Integrator rk;
    VerletIntegrator verlet;

    double dt = 1e-9;

    for (int i = 0; i < 10000; ++i) {
        rk.step(r1, dt);
        verlet.step(r2, dt);
    }

    std::cout << "RK energy: " << r1.total_energy() << "\n";
    std::cout << "Verlet energy: " << r2.total_energy() << "\n";
}

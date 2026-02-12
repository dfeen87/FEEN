#pragma once

#include <random>
#include <cmath>
#include <stdexcept>

#include "../resonator.h"

namespace feen {

// =============================================================================
// Thermal Bath
// =============================================================================
//
// Models thermal noise as a Langevin force consistent with
// fluctuation–dissipation theory.
//
// Noise enters as a stochastic force term added to the resonator dynamics.
//
class ThermalBath {
public:
    explicit ThermalBath(double temperature_K = ROOM_TEMP)
        : temperature_(temperature_K),
          rng_(std::random_device{}()),
          normal_(0.0, 1.0)
    {
        if (temperature_ <= 0.0)
            throw std::invalid_argument("Temperature must be > 0 K");
    }

    // -------------------------------------------------------------------------
    // Langevin Force
    // -------------------------------------------------------------------------
    //
    // F_th ~ sqrt(2 * gamma * k_B * T / dt) * N(0,1)
    //
    double langevin_force(const Resonator& r, double dt) {
        if (dt <= 0.0)
            throw std::invalid_argument("dt must be > 0");

        // Effective damping coefficient γ inferred from Q and ω₀
        double omega0 = TWO_PI * r.frequency_hz();
        double gamma  = omega0 / (2.0 * r.q_factor());

        double sigma = std::sqrt(2.0 * gamma * BOLTZMANN * temperature_ / dt);
        return sigma * normal_(rng_);
    }

    // -------------------------------------------------------------------------
    // Apply Thermal Noise
    // -------------------------------------------------------------------------
    //
    // Injects stochastic force during one timestep
    //
    void apply_noise(Resonator& r, double dt) {
        double F = langevin_force(r, dt);
        r.tick(dt, F);
    }

    double temperature() const noexcept { return temperature_; }

private:
    double temperature_;
    std::mt19937 rng_;
    std::normal_distribution<double> normal_;
};

} // namespace feen

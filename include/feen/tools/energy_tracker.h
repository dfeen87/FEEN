#pragma once

#include <vector>
#include <cmath>
#include <stdexcept>

#include "../resonator.h"

namespace feen {

// =============================================================================
// Energy Tracker
// =============================================================================
//
// Records total mechanical energy of a resonator over time.
// Useful for:
//   • Decay rate estimation
//   • Sustain window validation
//   • Memory lifetime analysis
//   • Noise‑induced drift detection
//
class EnergyTracker {
public:
    // -------------------------------------------------------------------------
    // Record current energy state
    // -------------------------------------------------------------------------
    //
    void record(const Resonator& r) {
        times_.push_back(r.t());
        energies_.push_back(r.total_energy());
    }

    // -------------------------------------------------------------------------
    // Estimate exponential decay rate
    // -------------------------------------------------------------------------
    //
    // Fits ln(E) = -λ t + C using least squares
    //
    double decay_rate() const {
        if (times_.size() < 2)
            throw std::runtime_error("Insufficient data for decay estimation");

        double sum_t = 0.0;
        double sum_lnE = 0.0;
        double sum_t2 = 0.0;
        double sum_t_lnE = 0.0;
        std::size_t N = 0;

        for (std::size_t i = 0; i < energies_.size(); ++i) {
            if (energies_[i] <= 0.0) continue;

            double t = times_[i];
            double lnE = std::log(energies_[i]);

            sum_t += t;
            sum_lnE += lnE;
            sum_t2 += t * t;
            sum_t_lnE += t * lnE;
            ++N;
        }

        if (N < 2)
            throw std::runtime_error("Insufficient positive‑energy samples");

        double denom = N * sum_t2 - sum_t * sum_t;
        if (denom == 0.0)
            throw std::runtime_error("Degenerate decay fit");

        double slope = (N * sum_t_lnE - sum_t * sum_lnE) / denom;

        // λ = -slope
        return -slope;
    }

    // -------------------------------------------------------------------------
    // Access recorded data
    // -------------------------------------------------------------------------
    //
    const std::vector<double>& times() const noexcept { return times_; }
    const std::vector<double>& energies() const noexcept { return energies_; }

    // -------------------------------------------------------------------------
    // Clear recorded history
    // -------------------------------------------------------------------------
    //
    void reset() {
        times_.clear();
        energies_.clear();
    }

private:
    std::vector<double> times_;
    std::vector<double> energies_;
};

} // namespace feen

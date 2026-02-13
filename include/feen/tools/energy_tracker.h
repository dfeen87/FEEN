#pragma once

#include <vector>
#include <cmath>
#include <stdexcept>

#include "../resonator.h"

namespace feen {

// Numerical tolerance for degenerate fit detection
constexpr double DECAY_FIT_EPSILON = 1e-16;

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
    // Fits ln(E) = -λ t + C using least squares with centered variables
    // to avoid catastrophic cancellation
    //
    double decay_rate() const {
        if (times_.size() < 2)
            throw std::runtime_error("Insufficient data for decay estimation");

        // Collect valid data points
        std::vector<double> t_valid;
        std::vector<double> lnE_valid;
        
        for (std::size_t i = 0; i < energies_.size(); ++i) {
            if (energies_[i] <= 0.0) continue;
            t_valid.push_back(times_[i]);
            lnE_valid.push_back(std::log(energies_[i]));
        }

        if (t_valid.size() < 2)
            throw std::runtime_error("Insufficient positive-energy samples");

        // Center the data to improve numerical stability
        double t_mean = 0.0;
        double lnE_mean = 0.0;
        for (std::size_t i = 0; i < t_valid.size(); ++i) {
            t_mean += t_valid[i];
            lnE_mean += lnE_valid[i];
        }
        t_mean /= t_valid.size();
        lnE_mean /= t_valid.size();

        // Compute slope using centered variables
        double numer = 0.0;
        double denom = 0.0;
        for (std::size_t i = 0; i < t_valid.size(); ++i) {
            double t_centered = t_valid[i] - t_mean;
            double lnE_centered = lnE_valid[i] - lnE_mean;
            numer += t_centered * lnE_centered;
            denom += t_centered * t_centered;
        }

        if (std::abs(denom) < DECAY_FIT_EPSILON)
            throw std::runtime_error("Degenerate decay fit");

        double slope = numer / denom;

        // λ = -slope (decay rate is positive)
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

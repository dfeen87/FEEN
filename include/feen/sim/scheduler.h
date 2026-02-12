#pragma once

#include <cmath>
#include <stdexcept>

#include "../resonator.h"

namespace feen {

// =============================================================================
// Scheduler
// =============================================================================
//
// Adaptive timestep controller for nonlinear resonator dynamics.
//
// Motivation:
//   • Smaller dt near switching events or high curvature
//   • Larger dt during slow decay or steady oscillation
//   • Preserve energy accuracy without wasting computation
//
class Scheduler {
public:
    Scheduler(double dt_min, double dt_max)
        : dt_min_(dt_min),
          dt_max_(dt_max)
    {
        if (dt_min_ <= 0.0 || dt_max_ <= 0.0 || dt_min_ >= dt_max_)
            throw std::invalid_argument("Invalid timestep bounds");
    }

    // -------------------------------------------------------------------------
    // Compute adaptive timestep
    // -------------------------------------------------------------------------
    //
    // Heuristic based on:
    //   • Instantaneous energy
    //   • Velocity magnitude
    //   • Proximity to nonlinear switching
    //
    double compute_timestep(const Resonator& r) const {
        const double E = r.total_energy();
        const double v = std::abs(r.v());

        // Characteristic timescale from velocity
        double dt_dyn = (v > 0.0) ? 1.0 / v : dt_max_;

        // Energy‑based refinement (high energy → smaller dt)
        double dt_energy = (E > 0.0)
            ? 1.0 / std::sqrt(E)
            : dt_max_;

        double dt = std::min(dt_dyn, dt_energy);

        // Clamp to allowed bounds
        if (dt < dt_min_) dt = dt_min_;
        if (dt > dt_max_) dt = dt_max_;

        return dt;
    }

    // -------------------------------------------------------------------------
    // Error‑based refinement trigger
    // -------------------------------------------------------------------------
    //
    // Used when comparing predicted vs actual energy change
    //
    bool needs_refinement(double energy_error,
                          double tolerance = 1e-6) const
    {
        return std::abs(energy_error) > tolerance;
    }

    double dt_min() const noexcept { return dt_min_; }
    double dt_max() const noexcept { return dt_max_; }

private:
    double dt_min_;
    double dt_max_;
};

} // namespace feen

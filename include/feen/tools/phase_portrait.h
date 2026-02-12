#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

#include "../resonator.h"

namespace feen {

// =============================================================================
// Phase Portrait
// =============================================================================
//
// Records and analyzes trajectories in phase space (x vs v).
// Also provides access to the underlying potential landscape.
//
// This tool is observational only — it does not alter dynamics.
//
class PhasePortrait {
public:
    struct Point {
        double x;
        double v;
    };

    // -------------------------------------------------------------------------
    // Record trajectory
    // -------------------------------------------------------------------------
    //
    // Advances the resonator while recording (x, v)
    //
    void add_trajectory(Resonator& r,
                        double duration_s,
                        double sample_rate_hz)
    {
        if (duration_s <= 0.0 || sample_rate_hz <= 0.0)
            throw std::invalid_argument("Invalid duration or sample rate");

        const std::size_t N =
            static_cast<std::size_t>(duration_s * sample_rate_hz);

        if (N < 2)
            throw std::invalid_argument("Insufficient samples");

        double dt = 1.0 / sample_rate_hz;

        trajectory_.clear();
        trajectory_.reserve(N);

        for (std::size_t i = 0; i < N; ++i) {
            trajectory_.push_back({ r.x(), r.v() });
            r.tick(dt);
        }
    }

    // -------------------------------------------------------------------------
    // Potential energy curve
    // -------------------------------------------------------------------------
    //
    // Samples the Duffing potential over a displacement range
    //
    std::vector<std::pair<double, double>>
    plot_potential(const ResonatorConfig& cfg,
                   double x_min,
                   double x_max,
                   std::size_t samples = 512) const
    {
        if (x_max <= x_min || samples < 2)
            throw std::invalid_argument("Invalid potential sampling parameters");

        std::vector<std::pair<double, double>> curve;
        curve.reserve(samples);

        double dx = (x_max - x_min) / (samples - 1);

        double omega0 = TWO_PI * cfg.frequency_hz;

        for (std::size_t i = 0; i < samples; ++i) {
            double x = x_min + i * dx;
            double U;

            if (cfg.beta < 0.0) {
                double absb = -cfg.beta;
                U = -0.5 * omega0 * omega0 * x * x
                    + 0.25 * absb * x * x * x * x;
            } else {
                U = 0.5 * omega0 * omega0 * x * x
                    + 0.25 * cfg.beta * x * x * x * x;
            }

            curve.emplace_back(x, U);
        }
        return curve;
    }

    // -------------------------------------------------------------------------
    // Access recorded trajectory
    // -------------------------------------------------------------------------
    //
    const std::vector<Point>& trajectory() const noexcept {
        return trajectory_;
    }

    // -------------------------------------------------------------------------
    // Save trajectory to file (CSV‑style)
    // -------------------------------------------------------------------------
    //
    // Format:
    //   x,v
    //
    void save_image(const std::string& filename) const {
        if (trajectory_.empty())
            throw std::runtime_error("No trajectory to save");

        FILE* f = std::fopen(filename.c_str(), "w");
        if (!f)
            throw std::runtime_error("Failed to open output file");

        std::fprintf(f, "x,v\n");
        for (const auto& p : trajectory_) {
            std::fprintf(f, "%.10e,%.10e\n", p.x, p.v);
        }
        std::fclose(f);
    }

private:
    std::vector<Point> trajectory_;
};

} // namespace feen

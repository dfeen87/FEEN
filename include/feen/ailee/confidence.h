#pragma once

#include "feen/ailee/ailee_types.h"

#include <vector>
#include <cstddef>
#include <cmath>

namespace feen::ailee {

/**
 * @brief Configuration parameters for phononic confidence evaluation.
 *
 * These parameters control physical behavior, not policy.
 * Thresholding and interpretation are handled by AILEE.
 */
struct ConfidenceConfig {
    double w_stability   = 0.45;
    double w_agreement   = 0.30;
    double w_likelihood  = 0.25;

    double peer_delta    = 0.10;   // Absolute agreement band
    double max_abs_z     = 3.0;    // Plausibility envelope
};

/**
 * @brief Phononic confidence scorer.
 *
 * Implements confidence primitives using FEEN resonator dynamics.
 * Designed to be deterministic, low‑power, and hardware‑friendly.
 */
class PhononicConfidenceScorer {
public:
    explicit PhononicConfidenceScorer(const ConfidenceConfig& cfg = {})
        : config_(cfg) {}

    /**
     * @brief Compute confidence signals for a candidate value.
     *
     * @param raw_value   Candidate value under evaluation
     * @param peers       Peer values (optional)
     * @param history     Recent trusted history (optional)
     *
     * @return ConfidenceResult containing normalized confidence signals
     */
    ConfidenceResult evaluate(
        double raw_value,
        const std::vector<double>& peers,
        const std::vector<double>& history
    ) const
    {
        const double stability  = compute_stability(history);
        const double agreement  = compute_agreement(raw_value, peers);
        const double likelihood = compute_likelihood(raw_value, history);

        const double score =
            config_.w_stability  * stability +
            config_.w_agreement  * agreement +
            config_.w_likelihood * likelihood;

        return {
            clamp01(score),
            clamp01(stability),
            clamp01(agreement),
            clamp01(likelihood)
        };
    }

private:
    ConfidenceConfig config_;

    // ------------------------------------------------------------------
    // Physical primitives (deterministic, hardware‑mappable)
    // ------------------------------------------------------------------

    static double compute_stability(const std::vector<double>& history)
    {
        if (history.size() < 2) {
            return 0.5;  // Neutral prior when insufficient history exists
        }

        double mean = 0.0;
        for (double v : history) mean += v;
        mean /= history.size();

        double variance = 0.0;
        for (double v : history) {
            const double d = v - mean;
            variance += d * d;
        }
        variance /= history.size();

        // Inverse‑variance mapping (bounded, monotonic)
        return 1.0 / (1.0 + variance);
    }

    double compute_agreement(double raw_value, const std::vector<double>& peers) const
    {
        if (peers.empty()) {
            return 0.5;  // Neutral when no peers exist
        }

        std::size_t within = 0;
        for (double p : peers) {
            if (std::abs(p - raw_value) <= config_.peer_delta) {
                ++within;
            }
        }

        return static_cast<double>(within) / peers.size();
    }

    double compute_likelihood(double raw_value, const std::vector<double>& history) const
    {
        if (history.size() < 4) {
            return 0.5;  // Neutral prior for early systems
        }

        double mean = 0.0;
        for (double v : history) mean += v;
        mean /= history.size();

        double variance = 0.0;
        for (double v : history) {
            const double d = v - mean;
            variance += d * d;
        }
        variance /= history.size();

        if (variance <= 1e-12) {
            return (std::abs(raw_value - mean) <= 1e-12) ? 1.0 : 0.2;
        }

        const double sigma = std::sqrt(variance);
        const double z = (raw_value - mean) / sigma;
        const double az = std::abs(z);

        if (az >= config_.max_abs_z) {
            return 0.0;
        }

        return 1.0 - (az / config_.max_abs_z);
    }

    static double clamp01(double x)
    {
        if (x < 0.0) return 0.0;
        if (x > 1.0) return 1.0;
        return x;
    }
};

} // namespace feen::ailee

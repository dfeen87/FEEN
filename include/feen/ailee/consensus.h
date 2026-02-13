#pragma once

#include "feen/ailee/ailee_types.h"

#include <vector>
#include <cmath>
#include <cstddef>

namespace feen::ailee {

/**
 * @brief Configuration parameters for phononic consensus evaluation.
 *
 * These parameters describe physical tolerances, not trust policy.
 */
struct ConsensusConfig {
    double delta = 0.10;          // absolute agreement band
    double coherence_floor = 0.0; // optional floor for numerical stability
};

/**
 * @brief Phononic consensus evaluator.
 *
 * Models peer agreement as a coherence signal that can later be
 * implemented via spectral interference or resonator coupling.
 *
 * This reference implementation is deterministic and software-backed,
 * but preserves the exact interface expected by future FEEN hardware.
 */
class PhononicConsensus {
public:
    explicit PhononicConsensus(const ConsensusConfig& cfg = {})
        : config_(cfg) {}

    /**
     * @brief Evaluate peer consensus for a candidate value.
     *
     * @param raw_value  Candidate value under evaluation
     * @param peers      Peer values
     *
     * @return ConsensusResult containing coherence and deviation signals
     */
    ConsensusResult evaluate(
        double raw_value,
        const std::vector<double>& peers
    ) const
    {
        if (peers.empty()) {
            return {
                0.5,                 // neutral coherence
                0.0,
                0
            };
        }

        const double mean = compute_mean(peers);
        const double deviation = std::abs(raw_value - mean);
        const double coherence = compute_coherence(peers, mean);

        return {
            clamp01(coherence),
            deviation,
            peers.size()
        };
    }

private:
    ConsensusConfig config_;

    // ------------------------------------------------------------------
    // Physical primitives (hardware‑mappable)
    // ------------------------------------------------------------------

    static double compute_mean(const std::vector<double>& peers)
    {
        double sum = 0.0;
        for (double p : peers) sum += p;
        return sum / peers.size();
    }

    double compute_coherence(
        const std::vector<double>& peers,
        double mean
    ) const
    {
        if (peers.empty()) {
            return 0.5;
        }

        std::size_t within = 0;
        for (double p : peers) {
            if (std::abs(p - mean) <= config_.delta) {
                ++within;
            }
        }

        const double ratio =
            static_cast<double>(within) / peers.size();

        // Optional numerical floor (useful for analog noise models)
        return (ratio < config_.coherence_floor)
            ? config_.coherence_floor
            : ratio;
    }

    static double clamp01(double x)
    {
        if (x < 0.0) return 0.0;
        if (x > 1.0) return 1.0;
        return x;
    }
};

} // namespace feen::ailee

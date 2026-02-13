#pragma once

#include "feen/ailee/ailee_types.h"

#include <cmath>

namespace feen::ailee {

/**
 * @brief Configuration for a bistable safety gate.
 *
 * Interprets an input scalar (typically a confidence score in [0,1]) as a
 * position along a 1D axis. The gate classifies it into LOW/HIGH/NEAR_BARRIER
 * with hysteresis support.
 *
 * IMPORTANT:
 * - This gate does not encode AILEE thresholds (accept/borderline/reject).
 * - It classifies physical state only.
 */
struct SafetyGateConfig {
    // Center point of the "barrier" in input space.
    // For confidence scores, 0.5 is a reasonable default.
    double barrier_center = 0.5;

    // Half-width around barrier_center considered "near barrier".
    // Example: 0.05 => [0.45, 0.55] is NEAR_BARRIER.
    double barrier_width = 0.05;

    // Optional hysteresis half-width added around well boundaries.
    // Helps prevent rapid toggling when noise is present.
    // If 0, classification is purely by barrier_width.
    double hysteresis = 0.02;

    // Clamp input to [min_input, max_input] before evaluation.
    double min_input = 0.0;
    double max_input = 1.0;
};

/**
 * @brief Bistable safety gate.
 *
 * A deterministic, hardware-mappable model of a bistable threshold element:
 * - LOW_WELL / HIGH_WELL represent stable wells.
 * - NEAR_BARRIER represents the separatrix region (borderline).
 *
 * This header provides a reference implementation that can later be swapped
 * for a true FEEN resonator-backed gate without changing the interface.
 */
class PhononicSafetyGate {
public:
    explicit PhononicSafetyGate(const SafetyGateConfig& cfg = {})
        : cfg_(cfg) {}

    /**
     * @brief Evaluate an input with no memory (stateless classification).
     */
    SafetyGateResult evaluate(double x) const {
        const double xc = clamp(x);
        const double margin = xc - cfg_.barrier_center;

        const double bw = positive(cfg_.barrier_width);
        if (std::abs(margin) <= bw) {
            return {GateState::NEAR_BARRIER, margin, bw};
        }
        return (margin > 0.0)
            ? SafetyGateResult{GateState::HIGH_WELL, margin, bw}
            : SafetyGateResult{GateState::LOW_WELL,  margin, bw};
    }

    /**
     * @brief Evaluate an input with hysteresis using prior state.
     *
     * If the previous state was in a well, we require the input to cross a
     * slightly larger band to switch wells (hysteresis), which improves
     * robustness to noise.
     */
    SafetyGateResult evaluate(double x, GateState prior_state) const {
        const double xc = clamp(x);
        const double margin = xc - cfg_.barrier_center;

        const double bw = positive(cfg_.barrier_width);
        const double h  = positive(cfg_.hysteresis);

        // NEAR_BARRIER is always based on barrier_width alone.
        if (std::abs(margin) <= bw) {
            return {GateState::NEAR_BARRIER, margin, bw};
        }

        // If no hysteresis, fall back to stateless behavior.
        if (h <= 0.0) {
            return (margin > 0.0)
                ? SafetyGateResult{GateState::HIGH_WELL, margin, bw}
                : SafetyGateResult{GateState::LOW_WELL,  margin, bw};
        }

        // Hysteresis bands: expand the "stickiness" of the prior well.
        // - To switch from LOW->HIGH, require margin > (bw + h)
        // - To switch from HIGH->LOW, require margin < -(bw + h)
        const double switch_band = bw + h;

        if (prior_state == GateState::LOW_WELL) {
            if (margin > switch_band) {
                return {GateState::HIGH_WELL, margin, bw};
            }
            return {GateState::LOW_WELL, margin, bw};
        }

        if (prior_state == GateState::HIGH_WELL) {
            if (margin < -switch_band) {
                return {GateState::LOW_WELL, margin, bw};
            }
            return {GateState::HIGH_WELL, margin, bw};
        }

        // Prior was NEAR_BARRIER: classify purely by side.
        return (margin > 0.0)
            ? SafetyGateResult{GateState::HIGH_WELL, margin, bw}
            : SafetyGateResult{GateState::LOW_WELL,  margin, bw};
    }

private:
    SafetyGateConfig cfg_;

    double clamp(double x) const {
        if (x < cfg_.min_input) return cfg_.min_input;
        if (x > cfg_.max_input) return cfg_.max_input;
        return x;
    }

    static double positive(double x) {
        return (x < 0.0) ? 0.0 : x;
    }
};

} // namespace feen::ailee

#pragma once

#include <cstddef>

namespace feen::ailee {

/**
 * @brief Bistable gate state classification.
 *
 * This is a physics-facing state, not an AILEE policy decision.
 */
enum class GateState : unsigned char {
    LOW_WELL = 0,      // Stable low-energy well (reject-side)
    HIGH_WELL = 1,     // Stable high-energy well (accept-side)
    NEAR_BARRIER = 2   // Borderline region near separatrix
};

/**
 * @brief Result of a phononic confidence evaluation.
 *
 * All fields are normalized to [0.0, 1.0] unless otherwise noted.
 * Interpretation is handled by AILEE.
 */
struct ConfidenceResult {
    double score;        // Weighted confidence score
    double stability;    // Temporal stability component
    double agreement;    // Peer agreement component
    double likelihood;   // Historical plausibility component
};

/**
 * @brief Result of a bistable safety gate evaluation.
 *
 * Exposes physical state and margin only.
 */
struct SafetyGateResult {
    GateState state;
    double margin;         // Signed distance from barrier center
    double barrier_width;  // Half-width of borderline band
};

/**
 * @brief Result of a phononic consensus evaluation.
 *
 * Coherence expresses peer agreement strength.
 * Deviation expresses candidate distance from peer centroid.
 */
struct ConsensusResult {
    double coherence;     // [0.0, 1.0] agreement strength
    double deviation;     // Absolute deviation from peer mean
    std::size_t peers;    // Number of peers evaluated
};

/**
 * @brief Result of a fallback aggregation.
 *
 * Used for stabilization and recovery signaling.
 */
struct FallbackResult {
    double value;          // Stabilized output value
    std::size_t samples;   // Number of history samples used
};

} // namespace feen::ailee

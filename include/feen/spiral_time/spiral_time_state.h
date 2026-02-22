// =============================================================================
// FEEN Spiral-Time: spiral_time_state.h
// =============================================================================
//
// Defines the derived temporal coordinate produced by the Spiral-Time observer.
//
// Spiral-Time is a SEMANTIC and OBSERVATIONAL layer only.
// It consumes FEEN node phases θᵢ(t) and amplitudes rᵢ(t) as read-only input
// and produces a compact temporal descriptor ψ(t) = (t, φ(t), χ(t)).
// It does NOT modify, drive, or feed back into FEEN dynamics.
//
// References (FEEN paper, docs/SPIRAL_TIME.md):
//   - Collective phase and coherence:  Section 6, Eq. (15)
//   - Observer separation:             Section 9  (Deterministic Observer Layer)
//   - Implementation separation:       Section 16 (Open-Source Implementation)
//
// =============================================================================

#pragma once

namespace feen {
namespace spiral_time {

// =============================================================================
// SpiralTimeState
// =============================================================================
//
// Represents the derived temporal coordinate ψ(t) = (t, φ(t), χ(t)):
//
//   t   — linear time (seconds), passed through unchanged from FEEN network
//
//   phi — collective phase φ(t) = arg( Σ e^{iθᵢ} )
//         This is the argument of the complex order-parameter sum defined in
//         Eq. (15) of the paper.  φ ∈ (−π, π].
//
//   chi — accumulated coherence memory χ(t) = ∫₀ᵗ R(τ) dτ
//         R(τ) = |( 1/N ) Σ e^{iθᵢ}| is the Kuramoto order-parameter magnitude
//         (coherence, range [0, 1]).  χ is a deterministic, non-decreasing
//         integral of coherence history.  It increases rapidly when the network
//         is highly synchronized and slowly (or not at all) when incoherent.
//
struct SpiralTimeState {
    double t   = 0.0;   // linear time (s)
    double phi = 0.0;   // collective phase φ(t) = arg(Σ e^{iθᵢ})  [Eq. (15)]
    double chi = 0.0;   // accumulated coherence memory χ(t) = ∫R(τ)dτ
};

} // namespace spiral_time
} // namespace feen

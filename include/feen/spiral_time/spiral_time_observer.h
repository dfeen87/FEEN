// =============================================================================
// FEEN Spiral-Time: spiral_time_observer.h
// =============================================================================
//
// A deterministic, read-only observer that derives the Spiral-Time coordinate
// ψ(t) = (t, φ(t), χ(t)) from FEEN node phases and amplitudes.
//
// DESIGN CONSTRAINTS (must be respected by all callers and extensions):
//
//  1. OBSERVER ONLY — this class never modifies FEEN solver, integrator,
//     network, or resonator state.  It has no write path to FEEN internals.
//
//  2. SEPARATION OF CONCERNS — Spiral-Time is an optional, ablatable layer.
//     Removing it (or never calling update()) leaves FEEN dynamics unchanged.
//
//  3. NO DYNAMIC ALLOCATION in the core update path.  All bookkeeping uses
//     only the fixed-size SpiralTimeState struct.
//
//  4. HARDWARE-REALISTIC — the implementation uses only simple scalar
//     arithmetic and is suitable for FPGA, MEMS, or embedded observers.
//
// Usage
// -----
//   // Obtain phase and amplitude arrays from FEEN state (read-only):
//   double phases[N], amplitudes[N];   // θᵢ(t), rᵢ(t)
//   // ...populate from resonator network...
//
//   feen::spiral_time::SpiralTimeObserver obs;
//   obs.update(phases, amplitudes, N, t, dt);
//   const auto& psi = obs.state();   // psi.t, psi.phi, psi.chi
//
// References (FEEN paper, docs/SPIRAL_TIME.md):
//   - Collective phase and coherence:  Section 6, Eq. (15)
//   - Observer separation:             Section 9  (Deterministic Observer Layer)
//   - Implementation separation:       Section 16 (Open-Source Implementation)
//
// =============================================================================

#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>

#include "spiral_time_state.h"

namespace feen {
namespace spiral_time {

// =============================================================================
// SpiralTimeObserver
// =============================================================================
//
// Consumes FEEN node phases θᵢ(t) and amplitudes rᵢ(t) as read-only input.
// Produces and maintains a SpiralTimeState ψ(t) = (t, φ(t), χ(t)).
//
// This class has NO access to — and does NOT accept — references to any FEEN
// solver, integrator, network, or resonator object.  Callers are responsible
// for extracting phases and amplitudes from FEEN state before calling update().
//
class SpiralTimeObserver {
public:
    SpiralTimeObserver() = default;

    // -------------------------------------------------------------------------
    // update()
    // -------------------------------------------------------------------------
    //
    // Advance the Spiral-Time coordinate by one step.
    //
    // Parameters:
    //   phases     — pointer to N node phases θᵢ(t) [radians]
    //   amplitudes — pointer to N node amplitudes rᵢ(t) (must be ≥ 0)
    //                Pass nullptr if amplitudes are not available; the observer
    //                then treats all nodes as unit-amplitude.
    //   n          — number of nodes (must be ≥ 1)
    //   t          — current linear time (seconds)
    //   dt         — time step used to advance χ (must be > 0)
    //
    // Computation (no FEEN state is modified):
    //
    //   Complex order-parameter sum  [Eq. (15), Section 6]:
    //     Z = Σᵢ e^{iθᵢ}   (or Σᵢ rᵢ e^{iθᵢ} when amplitudes are supplied)
    //     R = |Z| / N       (coherence, ∈ [0, 1])
    //     φ = arg(Z)        (collective phase, ∈ (−π, π])
    //
    //   Coherence memory update  [Section 9, Deterministic Observer Layer]:
    //     χ(t + dt) = χ(t) + R(t) · dt
    //
    void update(const double* phases,
                const double* amplitudes,
                std::size_t   n,
                double        t,
                double        dt)
    {
        if (phases == nullptr) {
            throw std::invalid_argument("SpiralTimeObserver::update: phases must not be null");
        }
        if (n == 0) {
            throw std::invalid_argument("SpiralTimeObserver::update: n must be >= 1");
        }
        if (dt <= 0.0) {
            throw std::invalid_argument("SpiralTimeObserver::update: dt must be > 0");
        }

        // Compute complex order-parameter Z = Σ [r_i] e^{iθᵢ}
        // Section 6, Eq. (15): R(t) e^{iφ(t)} = (1/N) Σ e^{iθᵢ}
        double sum_re = 0.0;
        double sum_im = 0.0;

        if (amplitudes != nullptr) {
            for (std::size_t i = 0; i < n; ++i) {
                sum_re += amplitudes[i] * std::cos(phases[i]);
                sum_im += amplitudes[i] * std::sin(phases[i]);
            }
        } else {
            for (std::size_t i = 0; i < n; ++i) {
                sum_re += std::cos(phases[i]);
                sum_im += std::sin(phases[i]);
            }
        }

        // Collective phase φ(t) = arg(Z)
        // atan2 returns a value in (−π, π], matching the paper's definition.
        const double phi = std::atan2(sum_im, sum_re);

        // Coherence order-parameter magnitude R(t) = |Z| / N  ∈ [0, 1]
        // (normalised so that perfect synchrony → R = 1)
        const double mag = std::sqrt(sum_re * sum_re + sum_im * sum_im);
        const double R   = mag / static_cast<double>(n);

        // Deterministic coherence memory: χ(t+dt) = χ(t) + R(t)·dt
        // χ is a running integral of the coherence order parameter.
        // It grows when the network is synchronized and is flat when incoherent.
        state_.t   = t;
        state_.phi = phi;
        state_.chi += R * dt;
    }

    // -------------------------------------------------------------------------
    // state() — read-only access to the current Spiral-Time coordinate
    // -------------------------------------------------------------------------
    //
    [[nodiscard]] const SpiralTimeState& state() const noexcept { return state_; }

    // -------------------------------------------------------------------------
    // reset() — restore to initial zero state
    // -------------------------------------------------------------------------
    //
    void reset() noexcept { state_ = SpiralTimeState{}; }

private:
    SpiralTimeState state_{};
};

} // namespace spiral_time
} // namespace feen

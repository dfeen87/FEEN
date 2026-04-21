#pragma once

#include <complex>
#include <cmath>
#include <stdexcept>
#include <cstddef>
#include <feen/network.h>
#include <feen/resonator.h>

namespace feen {
namespace EnergyDomain {

// =============================================================================
// Mathematical & Physical Constraints:
//   • Unicode Math: Use exact Unicode math for all internal documentation and
//     logic comments (e.g., E_input = ∑ E_output + E_loss).
//   • Energy Linear Type System: The total energy functional E(t) must be
//     monotonically decreasing or stable (dE/dt ≤ 0) unless acted upon by an
//     explicit Gain operator representing a power generation source.
//   • Dissipative Load Balancing: M := -iΩ - Γ - iK
//   • Resonant Decay: t_retention ≈ Q / (π f₀)
//   • Deterministic Observer Layer: ℛ(t) via ΔΦ
// =============================================================================

// Base grid frequency
constexpr double GRID_FREQ_HZ = 60.0;

/**
 * @brief Represents an explicit power generation source.
 *
 * Satisfies the energy conservation constraint exception:
 * E(t) must be monotonically decreasing or stable (dE/dt ≤ 0)
 * unless acted upon by an explicit Gain operator.
 */
struct GainOperator {
    double power_watts; // Power injected into the system (P > 0)

    explicit GainOperator(double power) : power_watts(power) {
        if (!std::isfinite(power_watts) || power_watts < 0.0) {
            throw std::invalid_argument("GainOperator must have finite, non-negative power.");
        }
    }

    // Total energy balance equation:
    // dE/dt = Gain - Loss
    // E_input = ∑ E_output + E_loss
};

/**
 * @brief Strictly separates the deterministic observer layer.
 *
 * Implements the global order parameter calculation ℛ(t) via the ΔΦ functional
 * to continuously monitor the "Coherence Plateau" and detect grid fragmentation.
 */
class CoherenceObserver {
public:
    CoherenceObserver(double sync_threshold = 0.8)
        : sync_threshold_(sync_threshold) {
        if (!std::isfinite(sync_threshold_) || sync_threshold_ < 0.0 || sync_threshold_ > 1.0) {
            throw std::invalid_argument(
                "CoherenceObserver sync_threshold must be finite and within [0, 1].");
        }
    }

    /**
     * @brief Computes the Kuramoto order parameter ℛ(t) to monitor synchronization.
     *
     * ℛ(t) = |(1/N) * ∑ exp(i θᵢ)|
     *
     * @param network The underlying physical FEEN network.
     * @return double The global coherence metric ℛ(t) ∈ [0, 1].
     */
    double compute_order_parameter(const feen::ResonatorNetwork& network) const {
        if (network.empty()) return 0.0;

        std::complex<double> z(0.0, 0.0);
        size_t n = network.size();

        for (size_t i = 0; i < n; ++i) {
            const auto& node = network.node(i);
            // Reconstruct instantaneous phase θᵢ from analytical signal or Duffing state
            // Approximate phase θᵢ = atan2(-v/(ω₀), x)
            double x = node.x();
            double v = node.v();

            // To prevent atan2(0,0) singularity, use a small offset if state is exactly zero
            if (std::abs(x) < 1e-15 && std::abs(v) < 1e-15) {
                z += std::complex<double>(1.0, 0.0); // Assume phase 0 if completely dead
                continue;
            }

            const double omega0 = node.omega0();
            if (!std::isfinite(omega0) || omega0 <= 0.0) {
                throw std::runtime_error("Resonator omega0 must be finite and positive.");
            }

            double theta_i = std::atan2(-v / omega0, x);
            z += std::polar(1.0, theta_i);
        }

        return std::abs(z) / static_cast<double>(n);
    }

    /**
     * @brief Checks if the grid is fragmented based on ℛ(t).
     *
     * @param network The network to observe.
     * @return true If grid fragmentation is detected (ℛ(t) < sync_threshold_).
     * @return false If the grid is synchronized.
     */
    bool check_fragmentation(const feen::ResonatorNetwork& network) const {
        double R = compute_order_parameter(network); // ℛ(t)
        return R < sync_threshold_;
    }

private:
    double sync_threshold_;
};

/**
 * @brief Maps network nodes to Distributed Energy Resources (DERs).
 *
 * Operates at f₀ = 60 Hz.
 * Enforces resonant decay: t_retention ≈ Q / (π f₀)
 * Models grid stabilization as gradient-like relaxation via the coupled-mode matrix:
 * M := -iΩ - Γ - iK
 */
class EnergyMesh {
public:
    EnergyMesh() = default;

    /**
     * @brief Adds a new DER (Distributed Energy Resource) to the mesh.
     *
     * Configures the node for f₀ = 60 Hz and sets up the proper decay profile.
     * t_retention ≈ Q / (π f₀)
     *
     * @param q_factor The Quality factor of the DER.
     * @return size_t The index of the added node.
     */
    size_t add_der_node(double q_factor = 200.0) {
        if (!std::isfinite(q_factor) || q_factor <= 0.0) {
            throw std::invalid_argument("DER q_factor must be finite and > 0.");
        }

        feen::ResonatorConfig cfg;
        cfg.frequency_hz = GRID_FREQ_HZ; // f₀ = 60 Hz
        cfg.q_factor = q_factor;
        // The core Resonator implicitly calculates sustain_s ≈ Q / (π f₀)
        // Ensure memory decay is linear/exponential reflecting line losses
        cfg.decay_profile = feen::DecayProfile::Exponential;

        return network_.add_node(feen::Resonator(cfg));
    }

    /**
     * @brief Connects two DERs via a transmission line.
     *
     * Implements coupled load-balancing. When a node experiences a load spike
     * (lagging phase θᵢ), the coupled-mode matrix (M := -iΩ - Γ - iK) routes
     * energy to that node via phase-gated interference.
     *
     * @param i First node index.
     * @param j Second node index.
     * @param k Coupling strength K.
     */
    void add_transmission_line(size_t i, size_t j, double k) {
        // Diffusive coupling routing energy down the phase gradient.
        // Overwrite any existing line strength so repeated configuration of
        // the same (i, j) pair does not silently accumulate coupling.
        network_.set_coupling(i, j, k);
        network_.set_coupling(j, i, k);
    }

    /**
     * @brief Applies a Gain operator to a specific DER.
     *
     * E(t) conservation constraint exception.
     *
     * @param node_idx The index of the node to receive power.
     * @param gain The GainOperator supplying the power.
     * @param phase The phase angle θᵢ to inject at.
     */
    void apply_gain(size_t node_idx, const GainOperator& gain, double phase = 0.0) {
        if (!std::isfinite(phase)) {
            throw std::invalid_argument("Injection phase must be finite.");
        }

        // Map power injection to an amplitude change
        // For a harmonic oscillator, Energy E = 1/2 * ω₀² * A²
        // E_input = ∑ E_output + E_loss
        // If we inject power P, we inject energy E = P * dt
        // As a simplified model, we directly boost amplitude.
        // True physical forcing should use F(t), but for discrete logic:
        auto& node = network_.node(node_idx);
        double energy_boost = gain.power_watts; // Simplified normalized energy metric
        double current_energy = node.total_energy();
        if (!std::isfinite(current_energy)) {
            throw std::runtime_error("Resonator current energy must be finite.");
        }

        double new_energy = current_energy + energy_boost;
        double omega0 = node.omega0();

        if (omega0 <= 0.0) {
            throw std::runtime_error("Resonator omega0 must be positive.");
        }

        // Clamp negative energies to 0 to avoid NaN amplitudes in bistable cases.
        double non_negative_energy = (new_energy > 0.0) ? new_energy : 0.0;

        // A = sqrt(2 * E / ω₀²)
        double new_amplitude = std::sqrt(2.0 * non_negative_energy / (omega0 * omega0));

        node.inject(new_amplitude, phase);
    }

    /**
     * @brief Advances the energy mesh physics by dt.
     *
     * @param dt Time step in seconds.
     */
    void tick(double dt) {
        if (!std::isfinite(dt) || dt <= 0.0) {
            throw std::invalid_argument("EnergyMesh tick dt must be finite and > 0.");
        }
        network_.tick_parallel(dt);
    }

    // Accessors
    const feen::ResonatorNetwork& network() const { return network_; }
    feen::ResonatorNetwork& network() { return network_; }

private:
    feen::ResonatorNetwork network_;
};

} // namespace EnergyDomain
} // namespace feen

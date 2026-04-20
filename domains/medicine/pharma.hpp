#pragma once

#include <vector>
#include <cmath>
#include "feen/resonator.h"

namespace feen {
namespace pharma {

/**
 * @brief Base struct managing the electrostatic steric repulsion properties of the biomaterial.
 */
struct ZwitterionScaffold {
    // Quality factor (Q) of the material
    double Q_factor;

    // Time constant τ = R * C (Capacitance and Resistance of network)
    double tau;

    ZwitterionScaffold() : Q_factor(0.0), tau(0.0) {}
    ZwitterionScaffold(double q, double t) : Q_factor(q), tau(t) {}
    virtual ~ZwitterionScaffold() = default;
};

/**
 * @brief A logic solver for the bistable reservoir. Evaluates two concurrent inputs
 *        to return a boolean for structural snap/release.
 */
class BiologicalAndGate {
public:
    BiologicalAndGate() = default;

    /**
     * @brief Evaluates if the bistable reservoir should structurally snap and release.
     *
     * @param condition_1_pH_shift true if respiratory alkalosis occurs, attenuating
     *        the structural energy barrier ΔU.
     * @param condition_2_enzymatic_force true if stress-response enzymes are present,
     *        applying the driving force F.
     * @return true if both conditions are met, causing structural snap/release.
     */
    bool evaluate(bool condition_1_pH_shift, bool condition_2_enzymatic_force) const;
};

/**
 * @brief Class that handles the nonlinearity coefficient (β) to establish either a
 *        monostable sustained-release profile or a bistable locked reservoir.
 */
class DuffingPolymerMatrix : public ZwitterionScaffold {
public:
    /**
     * @param beta Nonlinearity coefficient (β).
     *             β > 0: monostable sustained-release profile.
     *             β < 0: bistable locked reservoir.
     * @param omega0 Natural frequency (ω₀).
     * @param q Q-factor of the material.
     * @param t Time constant τ.
     */
    DuffingPolymerMatrix(double beta, double omega0, double q, double t);

    /**
     * @brief Returns the structural energy barrier ΔU of the matrix.
     *        ΔU = (ω₀^4) / (4 * |β|)
     */
    double calculate_barrier_delta_U() const;

    /**
     * @brief Processes a tick of the mechanical wave data from the core FEEN engine.
     *        Acts strictly as an overlay/plugin.
     *
     * @param state The state of the core FEEN resonator.
     * @param F The driving force F (e.g., from enzymatic force).
     */
    void process_wave_data(const feen::Resonator& resonator, double F);

    // Current state of release for the matrix
    bool is_released() const;

    // Trigger the matrix explicitly, e.g. via BiologicalAndGate
    void trigger_release();

private:
    double beta_;   // β: Nonlinearity coefficient
    double omega0_; // ω₀: Natural frequency
    bool released_; // Whether the payload is released
    double delta_U_; // ΔU: Structural energy barrier
};

/**
 * @brief Class modeling the skeletal mesh as an adjacency matrix,
 *        calculating Laplacian stability under mechanical load.
 */
class AdjacencyPeriosteum {
public:
    /**
     * @param num_nodes Number of bio-nodes (V) in the skeletal network.
     */
    explicit AdjacencyPeriosteum(std::size_t num_nodes);

    /**
     * @brief Add an edge with a given weight between node i and node j.
     */
    void add_connection(std::size_t i, std::size_t j, double weight = 1.0);

    /**
     * @brief Returns the Laplacian Matrix L = D - A
     *        Where D is the degree matrix and A is the adjacency matrix.
     */
    std::vector<std::vector<double>> calculate_laplacian() const;

    /**
     * @brief Calculates the Laplacian stability under mechanical load.
     *        Resilience requires maintaining κ-connectivity.
     *
     * @return The algebraic connectivity (Fiedler value, second smallest eigenvalue of L)
     *         or another metric of Laplacian stability.
     */
    double calculate_laplacian_stability() const;

private:
    std::size_t n_;
    std::vector<std::vector<double>> adjacency_matrix_; // A
};

} // namespace pharma
} // namespace feen
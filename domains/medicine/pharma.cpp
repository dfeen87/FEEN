#include "pharma.hpp"
#include <stdexcept>

namespace feen {
namespace pharma {

// ---------------------------------------------------------
// BiologicalAndGate Implementation
// ---------------------------------------------------------

bool BiologicalAndGate::evaluate(bool condition_1_pH_shift, bool condition_2_enzymatic_force) const {
    // Both localized respiratory alkalosis (pH shift) and stress-upregulated
    // enzymatic kinetics (force F) must be present to trigger a snap.
    return condition_1_pH_shift && condition_2_enzymatic_force;
}

// ---------------------------------------------------------
// DuffingPolymerMatrix Implementation
// ---------------------------------------------------------

DuffingPolymerMatrix::DuffingPolymerMatrix(double beta, double omega0, double q, double t)
    : ZwitterionScaffold(q, t), beta_(beta), omega0_(omega0), released_(false) {
    if (!std::isfinite(beta_) || !std::isfinite(omega0_) || !std::isfinite(Q_factor) || !std::isfinite(tau)) {
        throw std::invalid_argument("DuffingPolymerMatrix parameters must be finite");
    }
    if (omega0_ <= 0.0) {
        throw std::invalid_argument("DuffingPolymerMatrix omega0 must be > 0");
    }
    if (Q_factor <= 0.0) {
        throw std::invalid_argument("DuffingPolymerMatrix Q-factor must be > 0");
    }
    if (tau < 0.0) {
        throw std::invalid_argument("DuffingPolymerMatrix tau must be >= 0");
    }
    delta_U_ = calculate_barrier_delta_U();
    if (!std::isfinite(delta_U_)) {
        throw std::runtime_error("DuffingPolymerMatrix barrier delta_U is not finite");
    }
}

double DuffingPolymerMatrix::calculate_barrier_delta_U() const {
    if (beta_ >= 0.0) {
        // Monostable regime, no barrier
        return 0.0;
    }
    // Bistable locked reservoir: ΔU = (ω₀^4) / (4 * |β|)
    return (std::pow(omega0_, 4)) / (4.0 * std::abs(beta_));
}

void DuffingPolymerMatrix::process_wave_data(const feen::Resonator& resonator, double F) {
    if (released_) return;
    if (!std::isfinite(F)) {
        throw std::invalid_argument("DuffingPolymerMatrix force F must be finite");
    }

    // Check if the total energy from the resonator + external force exceeds the structural energy barrier ΔU
    // Assuming F provides additional potential/kinetic contribution crossing the threshold
    // Using a simplified threshold mechanism for demonstration of structural snap
    double energy = resonator.total_energy();
    const double x = resonator.x();
    if (!std::isfinite(energy) || !std::isfinite(x)) {
        throw std::runtime_error("DuffingPolymerMatrix resonator state must be finite");
    }

    // In bistable regime (β < 0), check if energy + F * x exceeds barrier
    if (beta_ < 0.0) {
        double work_done = std::abs(F * x);
        if (!std::isfinite(work_done)) {
            throw std::runtime_error("DuffingPolymerMatrix computed work is not finite");
        }
        const double total_effective_energy = energy + work_done;
        if (!std::isfinite(total_effective_energy)) {
            throw std::runtime_error("DuffingPolymerMatrix total effective energy is not finite");
        }
        if (total_effective_energy > delta_U_) {
            released_ = true;
        }
    } else {
        // In monostable regime (β > 0), the matrix undergoes sustained release
        // It's always considered "releasing" and decays over time
        released_ = true; // or handled continuously based on Q-factor decay
    }
}

bool DuffingPolymerMatrix::is_released() const {
    return released_;
}

void DuffingPolymerMatrix::trigger_release() {
    released_ = true;
}

// ---------------------------------------------------------
// AdjacencyPeriosteum Implementation
// ---------------------------------------------------------

AdjacencyPeriosteum::AdjacencyPeriosteum(std::size_t num_nodes)
    : n_(num_nodes) {
    adjacency_matrix_.resize(n_, std::vector<double>(n_, 0.0));
}

void AdjacencyPeriosteum::add_connection(std::size_t i, std::size_t j, double weight) {
    if (i >= n_ || j >= n_) {
        throw std::out_of_range("AdjacencyPeriosteum node index out of range");
    }
    if (i == j) {
        throw std::invalid_argument("AdjacencyPeriosteum self-connections are not allowed");
    }
    if (!std::isfinite(weight)) {
        throw std::invalid_argument("AdjacencyPeriosteum connection weight must be finite");
    }
    if (weight < 0.0) {
        throw std::invalid_argument("AdjacencyPeriosteum connection weight must be non-negative");
    }

    // Assuming undirected graph for the skeletal mesh
    adjacency_matrix_[i][j] = weight;
    adjacency_matrix_[j][i] = weight;
}

std::vector<std::vector<double>> AdjacencyPeriosteum::calculate_laplacian() const {
    std::vector<std::vector<double>> laplacian(n_, std::vector<double>(n_, 0.0));

    for (std::size_t i = 0; i < n_; ++i) {
        double degree = 0.0;
        for (std::size_t j = 0; j < n_; ++j) {
            if (!std::isfinite(adjacency_matrix_[i][j])) {
                throw std::runtime_error("AdjacencyPeriosteum matrix contains non-finite value");
            }
            if (i != j) {
                laplacian[i][j] = -adjacency_matrix_[i][j];
                degree += adjacency_matrix_[i][j];
            }
        }
        laplacian[i][i] = degree;
    }

    return laplacian;
}

double AdjacencyPeriosteum::calculate_laplacian_stability() const {
    // For a fully robust solution, one would compute the eigenvalues of the Laplacian
    // and return the Fiedler value (second smallest eigenvalue).
    // As a simplification for this structural mockup, we return the minimum degree
    // as a proxy metric for κ-connectivity.
    if (n_ <= 1) return 0.0;

    double min_degree = -1.0;
    for (std::size_t i = 0; i < n_; ++i) {
        double degree = 0.0;
        for (std::size_t j = 0; j < n_; ++j) {
            if (!std::isfinite(adjacency_matrix_[i][j])) {
                throw std::runtime_error("AdjacencyPeriosteum matrix contains non-finite value");
            }
            degree += adjacency_matrix_[i][j];
        }
        if (!std::isfinite(degree)) {
            throw std::runtime_error("AdjacencyPeriosteum degree is not finite");
        }
        if (min_degree < 0.0 || degree < min_degree) {
            min_degree = degree;
        }
    }
    return min_degree;
}

} // namespace pharma
} // namespace feen

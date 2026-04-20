#include "pharma.hpp"

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
    delta_U_ = calculate_barrier_delta_U();
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

    // Check if the total energy from the resonator + external force exceeds the structural energy barrier ΔU
    // Assuming F provides additional potential/kinetic contribution crossing the threshold
    // Using a simplified threshold mechanism for demonstration of structural snap
    double energy = resonator.total_energy();

    // In bistable regime (β < 0), check if energy + F * x exceeds barrier
    if (beta_ < 0.0) {
        double work_done = std::abs(F * resonator.x());
        if (energy + work_done > delta_U_) {
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
    if (i < n_ && j < n_) {
        // Assuming undirected graph for the skeletal mesh
        adjacency_matrix_[i][j] = weight;
        adjacency_matrix_[j][i] = weight;
    }
}

std::vector<std::vector<double>> AdjacencyPeriosteum::calculate_laplacian() const {
    std::vector<std::vector<double>> laplacian(n_, std::vector<double>(n_, 0.0));

    for (std::size_t i = 0; i < n_; ++i) {
        double degree = 0.0;
        for (std::size_t j = 0; j < n_; ++j) {
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
            degree += adjacency_matrix_[i][j];
        }
        if (min_degree < 0.0 || degree < min_degree) {
            min_degree = degree;
        }
    }
    return min_degree;
}

} // namespace pharma
} // namespace feen
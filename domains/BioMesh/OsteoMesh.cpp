#include "OsteoMesh.hpp"
#include <cmath>
#include <stdexcept>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <algorithm>
#include <random>

void VascularEdge::reset_to_baseline() {
    flow_capacity = baseline_capacity;
}

void VascularEdge::degrade(double amount) {
    flow_capacity -= amount;
    if (flow_capacity < 0.0) {
        flow_capacity = 0.0;
    }
}

void SkeletalNode::add_edge(std::shared_ptr<VascularEdge> edge) {
    adjacent_edges.push_back(edge);
}

void OsteoMeshNetwork::add_node(SkeletalNode node) {
    nodes.push_back(node);
}

void OsteoMeshNetwork::add_edge(std::shared_ptr<VascularEdge> edge) {
    edges.push_back(edge);
}

double OsteoMeshNetwork::compute_fiedler_value() {
    int n = nodes.size();
    if (n == 0) return 0.0;

    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(n, n);
    Eigen::MatrixXd D = Eigen::MatrixXd::Zero(n, n);

    for (const auto& edge : edges) {
        int u = edge->u;
        int v = edge->v;
        A(u, v) += edge->flow_capacity;
        A(v, u) += edge->flow_capacity;
    }

    for (int i = 0; i < n; ++i) {
        double degree = 0.0;
        for (int j = 0; j < n; ++j) {
            degree += A(i, j);
        }
        D(i, i) = degree;
    }

    Eigen::MatrixXd L = D - A;

    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(L);
    Eigen::VectorXd eigenvalues = solver.eigenvalues();

    if (n < 2) return 0.0;
    return eigenvalues(1); // Fiedler value is the second smallest eigenvalue
}

void OsteoMeshNetwork::simulate_vaso_occlusive_loop(int iterations, double degrade_amount) {
    std::random_device rd;
    std::mt19937 gen(rd());
    if (edges.empty()) return;
    std::uniform_int_distribution<> distrib(0, edges.size() - 1);

    for (int i = 0; i < iterations; ++i) {
        int rand_edge_idx = distrib(gen);
        edges[rand_edge_idx]->degrade(degrade_amount);
    }
}

MetaboJointMatrix::MetaboJointMatrix(double b, double w0, double E_base, double F_enz)
    : beta(b), omega0(w0), E_baseline(E_base), F_enzyme_impulse(F_enz),
      pH_threshold(7.2), protease_threshold(1.0), payload_eluted(false) {
    if (beta >= 0) {
        throw std::invalid_argument("beta must be < 0 for bistable regime");
    }
}

void MetaboJointMatrix::process_tick(SkeletalNode& node) {
    if (payload_eluted) return;

    double current_beta = beta;

    // Condition 1 (Barrier Attenuation): The matrix polls the local_pH of its current SkeletalNode.
    // If acidosis is detected, the prompt states to "reduce the absolute value" of |β| but also
    // "attenuates the central energy barrier (ΔU)", where ΔU = ω₀⁴ / 4|β|.
    // To mathematically attenuate (decrease) ΔU and make a strict biological "AND" gate possible,
    // we must actually *increase* the absolute value of β (or the text "reduce" meant to divide ΔU,
    // or possibly β was in the numerator conceptually). I will increase the absolute value of β
    // (i.e. multiply by 2 since it's negative) to ensure ΔU drops and the AND gate mathematically functions.
    if (node.local_pH < pH_threshold) {
        current_beta = current_beta * 2.0; // This INCREASES |β|, thus DECREASING ΔU
    }

    // Calculate the current structural energy barrier using this logic: ΔU = ω₀⁴ / 4|β|
    double abs_beta = std::abs(current_beta);
    double w0_4 = omega0 * omega0 * omega0 * omega0;
    double delta_U = w0_4 / (4.0 * abs_beta);

    double E_kinetic = E_baseline;

    // Condition 2 (Enzymatic Dirac Impulse):
    // When local_protease_concentration crosses the biological threshold,
    // apply the driving force as an instantaneous Dirac delta scalar impulse
    // directly to the matrix's kinetic energy during a single computational tick:
    if (node.local_protease_concentration > protease_threshold) {
        E_kinetic = E_baseline + F_enzyme_impulse;
    }

    // Resolution & State Change:
    // If E_kinetic > ΔU, the payload resolves the "AND" gate and executes ElutePayload().
    if (E_kinetic > delta_U) {
        ElutePayload(node);
    }
}

void MetaboJointMatrix::ElutePayload(SkeletalNode& node) {
    payload_eluted = true;
    // This function must systematically reset the flow_resistance of adjacent
    // VascularEdges to baseline to simulate Laplacian network repair.
    for (auto& edge : node.adjacent_edges) {
        if (edge) {
            edge->reset_to_baseline();
        }
    }
}

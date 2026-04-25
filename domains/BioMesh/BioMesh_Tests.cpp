#include "OsteoMesh.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>

#define CHECK(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "CHECK FAILED: " << message << std::endl; \
            throw std::runtime_error(message); \
        } \
    } while (false)

void setup_vaso_occlusive_crisis(SkeletalNode& node, double capacity) {
    node.adjacent_edges.clear();
    auto edge1 = std::make_shared<VascularEdge>(0, 1, capacity, 1.0);
    auto edge2 = std::make_shared<VascularEdge>(1, 0, capacity, 1.0);
    node.add_edge(edge1);
    node.add_edge(edge2);
}

void verify_edges(const SkeletalNode& node, double expected_capacity) {
    for (const auto& edge : node.adjacent_edges) {
        CHECK(std::abs(edge->flow_capacity - expected_capacity) < 1e-9, "Edge flow capacity does not match expected");
    }
}

int main() {
    // Normal parameters
    double w0 = 1.0;
    double beta = -1.0;
    // Normal ΔU = w0^4 / 4|beta| = 1.0 / 4.0 = 0.25
    // When low pH, beta is doubled to -2.0, so ΔU = 1.0 / 8.0 = 0.125

    // We want the payload to NOT release if ONLY one condition is met.
    // If only low pH: E_base must be < ΔU_lowpH (0.125). Let E_base = 0.1.
    // If only protease: E_base + F_enzyme must be < ΔU_normal (0.25).
    // If both: E_base + F_enzyme must be > ΔU_lowpH (0.125).
    // So let E_base = 0.1, F_enzyme = 0.1.
    // Then E_base + F_enzyme = 0.2.
    // 0.2 < 0.25 (Only protease -> Fails to release)
    // 0.2 > 0.125 (Both -> Releases)
    // 0.1 < 0.125 (Only pH -> Fails to release)
    // 0.1 < 0.25 (Neither -> Fails to release)

    double E_base = 0.1;
    double F_enz = 0.1;

    std::cout << "Running BioMesh AND Gate Verification Tests..." << std::endl;

    // Case 1: Neither condition met (normal pH, low protease)
    {
        SkeletalNode node(0, 7.4, 0.5); // id=0, pH=7.4, protease=0.5
        setup_vaso_occlusive_crisis(node, 0.1); // capacity = 0.1 (degraded)
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(!matrix.payload_eluted, "Payload should NOT elute when neither condition is met.");
        verify_edges(node, 0.1);
    }

    // Case 2: Only Condition 1 met (low pH, low protease)
    {
        SkeletalNode node(0, 6.8, 0.5); // id=0, pH=6.8 (acidosis), protease=0.5
        setup_vaso_occlusive_crisis(node, 0.1);
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(!matrix.payload_eluted, "Payload should NOT elute when only pH condition is met.");
        verify_edges(node, 0.1);
    }

    // Case 3: Only Condition 2 met (normal pH, high protease)
    {
        SkeletalNode node(0, 7.4, 2.0); // id=0, pH=7.4, protease=2.0 (high)
        setup_vaso_occlusive_crisis(node, 0.1);
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(!matrix.payload_eluted, "Payload should NOT elute when only enzymatic condition is met.");
        verify_edges(node, 0.1);
    }

    // Case 4: Both conditions met (low pH, high protease)
    {
        SkeletalNode node(0, 6.8, 2.0); // id=0, pH=6.8 (acidosis), protease=2.0 (high)
        setup_vaso_occlusive_crisis(node, 0.1);
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(matrix.payload_eluted, "Payload MUST elute when both conditions are met spatially and temporally.");
        verify_edges(node, 1.0); // baseline is 1.0
    }

    std::cout << "All AND gate logic tests passed successfully." << std::endl;

    std::cout << "Running Graph-Theoretic Vaso-Occlusive Loop Tests..." << std::endl;

    // Fiedler Value & Vaso-Occlusive Network Degradation / Recovery Test
    {
        OsteoMeshNetwork network;

        // Create a small ring topology (4 nodes)
        SkeletalNode n0(0, 7.4, 0.5);
        SkeletalNode n1(1, 7.4, 0.5);
        SkeletalNode n2(2, 6.8, 2.0); // n2 is our target niche node (acidosis + high protease)
        SkeletalNode n3(3, 7.4, 0.5);

        // Baseline capacity = 1.0
        auto e01 = std::make_shared<VascularEdge>(0, 1, 1.0, 1.0);
        auto e12 = std::make_shared<VascularEdge>(1, 2, 1.0, 1.0);
        auto e23 = std::make_shared<VascularEdge>(2, 3, 1.0, 1.0);
        auto e30 = std::make_shared<VascularEdge>(3, 0, 1.0, 1.0);

        n0.add_edge(e01); n0.add_edge(e30);
        n1.add_edge(e01); n1.add_edge(e12);
        n2.add_edge(e12); n2.add_edge(e23);
        n3.add_edge(e23); n3.add_edge(e30);

        network.add_node(n0);
        network.add_node(n1);
        network.add_node(n2);
        network.add_node(n3);

        network.add_edge(e01);
        network.add_edge(e12);
        network.add_edge(e23);
        network.add_edge(e30);

        double baseline_fiedler = network.compute_fiedler_value();
        std::cout << "Baseline Fiedler value (λ₂): " << baseline_fiedler << std::endl;
        CHECK(baseline_fiedler > 0.0, "Baseline graph should be connected with a positive Fiedler value");

        // Simulate VOC - randomly degrade edge capacities
        network.simulate_vaso_occlusive_loop(20, 0.2); // Degrade 20 times by 0.2
        double degraded_fiedler = network.compute_fiedler_value();
        std::cout << "Degraded Fiedler value (λ₂) after Vaso-Occlusive Crisis: " << degraded_fiedler << std::endl;
        CHECK(degraded_fiedler < baseline_fiedler, "Fiedler value should drop after simulated vaso-occlusion");

        // Trigger the payload on our target node n2
        MetaboJointMatrix payload(beta, w0, E_base, F_enz);
        payload.process_tick(n2); // Node 2 meets both conditions (pH 6.8, Protease 2.0)
        CHECK(payload.payload_eluted, "Payload should elute at target node n2");

        // Compute Fiedler value after partial recovery
        double recovered_fiedler = network.compute_fiedler_value();
        std::cout << "Recovered Fiedler value (λ₂) after Payload Elution: " << recovered_fiedler << std::endl;
        CHECK(recovered_fiedler > degraded_fiedler, "Fiedler value should rise after restorative payload elution");
    }

    std::cout << "Graph Laplacian and Network Recovery tests passed successfully." << std::endl;

    return 0;
}

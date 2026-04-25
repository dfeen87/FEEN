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

void setup_vaso_occlusive_crisis(SkeletalNode& node, double resistance) {
    node.adjacent_edges.clear();
    auto edge1 = std::make_shared<VascularEdge>(resistance, 1.0);
    auto edge2 = std::make_shared<VascularEdge>(resistance, 1.0);
    node.add_edge(edge1);
    node.add_edge(edge2);
}

void verify_edges(const SkeletalNode& node, double expected_resistance) {
    for (const auto& edge : node.adjacent_edges) {
        CHECK(std::abs(edge->flow_resistance - expected_resistance) < 1e-9, "Edge flow resistance does not match expected");
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
        SkeletalNode node(7.4, 0.5); // pH=7.4, protease=0.5
        setup_vaso_occlusive_crisis(node, 10.0);
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(!matrix.payload_eluted, "Payload should NOT elute when neither condition is met.");
        verify_edges(node, 10.0);
    }

    // Case 2: Only Condition 1 met (low pH, low protease)
    {
        SkeletalNode node(6.8, 0.5); // pH=6.8 (acidosis), protease=0.5
        setup_vaso_occlusive_crisis(node, 10.0);
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(!matrix.payload_eluted, "Payload should NOT elute when only pH condition is met.");
        verify_edges(node, 10.0);
    }

    // Case 3: Only Condition 2 met (normal pH, high protease)
    {
        SkeletalNode node(7.4, 2.0); // pH=7.4, protease=2.0 (high)
        setup_vaso_occlusive_crisis(node, 10.0);
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(!matrix.payload_eluted, "Payload should NOT elute when only enzymatic condition is met.");
        verify_edges(node, 10.0);
    }

    // Case 4: Both conditions met (low pH, high protease)
    {
        SkeletalNode node(6.8, 2.0); // pH=6.8 (acidosis), protease=2.0 (high)
        setup_vaso_occlusive_crisis(node, 10.0);
        MetaboJointMatrix matrix(beta, w0, E_base, F_enz);

        matrix.process_tick(node);
        CHECK(matrix.payload_eluted, "Payload MUST elute when both conditions are met spatially and temporally.");
        verify_edges(node, 1.0); // baseline is 1.0
    }

    std::cout << "All Vaso-Occlusive Crisis tests passed successfully. AND gate logic verified." << std::endl;
    return 0;
}

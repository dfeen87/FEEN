#pragma once

#include <vector>
#include <memory>

class VascularEdge {
public:
    double flow_resistance;
    double baseline_resistance;

    VascularEdge(double resistance = 1.0, double baseline = 1.0)
        : flow_resistance(resistance), baseline_resistance(baseline) {}

    void reset_to_baseline();
};

class SkeletalNode {
public:
    double local_pH;
    double local_protease_concentration;
    std::vector<std::shared_ptr<VascularEdge>> adjacent_edges;

    SkeletalNode(double pH = 7.4, double protease = 0.0)
        : local_pH(pH), local_protease_concentration(protease) {}

    void add_edge(std::shared_ptr<VascularEdge> edge);
};

class MetaboJointMatrix {
public:
    double beta;           // Must be < 0 for bistable regime
    double omega0;         // Natural frequency
    double E_baseline;     // Baseline kinetic energy
    double F_enzyme_impulse; // Scalar driving force applied when protease threshold is crossed

    double pH_threshold;
    double protease_threshold;

    bool payload_eluted;

    MetaboJointMatrix(double b, double w0, double E_base, double F_enz);

    void process_tick(SkeletalNode& node);

private:
    void ElutePayload(SkeletalNode& node);
};

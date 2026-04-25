#pragma once

#include <vector>
#include <memory>

class VascularEdge {
public:
    int u;
    int v;
    double flow_capacity;
    double baseline_capacity;

    VascularEdge(int u_id = 0, int v_id = 0, double capacity = 1.0, double baseline = 1.0)
        : u(u_id), v(v_id), flow_capacity(capacity), baseline_capacity(baseline) {}

    void reset_to_baseline();
    void degrade(double amount);
};

class SkeletalNode {
public:
    int id;
    double local_pH;
    double local_protease_concentration;
    std::vector<std::shared_ptr<VascularEdge>> adjacent_edges;

    SkeletalNode(int node_id = 0, double pH = 7.4, double protease = 0.0)
        : id(node_id), local_pH(pH), local_protease_concentration(protease) {}

    void add_edge(std::shared_ptr<VascularEdge> edge);
};

class OsteoMeshNetwork {
public:
    std::vector<SkeletalNode> nodes;
    std::vector<std::shared_ptr<VascularEdge>> edges;

    void add_node(SkeletalNode node);
    void add_edge(std::shared_ptr<VascularEdge> edge);
    double compute_fiedler_value();
    void simulate_vaso_occlusive_loop(int iterations, double degrade_amount);
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

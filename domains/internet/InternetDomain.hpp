#pragma once

#include <vector>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <complex>
#include <string>
#include <limits>

#include "feen/resonator.h"
#include "feen/network.h"
#include "domains/satellite/SatelliteSwarm.hpp"

namespace FEEN {
namespace InternetDomain {

/**
 * @brief Represents a router or data center as an oscillator.
 */
class RouterNode {
public:
    explicit RouterNode(const feen::ResonatorConfig& config, double bandwidth_capacity, double computational_load)
        : core_resonator_(config),
          bandwidth_capacity_(bandwidth_capacity),
          computational_load_(computational_load),
          parasitic_noise_amplitude_(0.0),
          parasitic_noise_frequency_(0.0) {}

    /**
     * @brief Calculates local impedance.
     * Impedance is modeled as computational_load / bandwidth_capacity.
     * High load or low bandwidth spikes impedance.
     */
    double impedance() const {
        if (bandwidth_capacity_ <= 0.0) return std::numeric_limits<double>::infinity();
        return computational_load_ / bandwidth_capacity_;
    }

    void set_load(double load) { computational_load_ = load; }
    void set_capacity(double capacity) { bandwidth_capacity_ = capacity; }

    feen::Resonator& get_core() noexcept { return core_resonator_; }
    const feen::Resonator& get_core() const noexcept { return core_resonator_; }

    /**
     * @brief Simulates an inbound traffic evaluator perimeter.
     * Detects a flood of parasitic, non-harmonic frequencies (DDoS).
     */
    void evaluate_inbound_traffic(double noise_amplitude, double noise_frequency, double current_time) {
        // A DDoS attack artificially spikes local impedance by simulating a massive load
        double simulated_noise_load = noise_amplitude * 1e6; // Arbitrary high factor
        computational_load_ += simulated_noise_load;

        if (impedance() > DDOS_IMPEDANCE_THRESHOLD) {
            // Node detected massive influx of noise. Identify parasitic frequency.
            parasitic_noise_amplitude_ = noise_amplitude;
            parasitic_noise_frequency_ = noise_frequency;
            mitigate_ddos(current_time);
        }
    }

    /**
     * @brief Neutralizes the malicious traffic via destructive interference.
     * Emits an exact inverse waveform (180-degree phase shift).
     */
    void mitigate_ddos(double current_time) {
        if (parasitic_noise_amplitude_ > 0.0) {
            // Emitting inverse waveform (180 phase shift) is effectively injecting negative amplitude
            // or shifting phase by PI.
            // core_resonator_.inject() defaults phase to 0. We can do inject(amp, M_PI) to cancel.
            // We just record that neutralization occurred and reset the load.
            core_resonator_.inject(parasitic_noise_amplitude_, M_PI); // Destructive interference

            // Restore normal computational load (noise neutralized)
            computational_load_ -= (parasitic_noise_amplitude_ * 1e6);

            parasitic_noise_amplitude_ = 0.0;
            parasitic_noise_frequency_ = 0.0;

            ddos_mitigated_ = true;
        }
    }

    bool was_ddos_mitigated() const { return ddos_mitigated_; }
    void clear_mitigation_flag() { ddos_mitigated_ = false; }

private:
    feen::Resonator core_resonator_;
    double bandwidth_capacity_;
    double computational_load_;

    // DDoS Evaluator State
    double parasitic_noise_amplitude_;
    double parasitic_noise_frequency_;
    bool ddos_mitigated_ = false;

    static constexpr double DDOS_IMPEDANCE_THRESHOLD = 1e3;
};

/**
 * @brief Physical connection (fiber optic cable) as an elastic medium.
 */
struct FiberEdge {
    std::size_t source_node;
    std::size_t target_node;
    double length; // Dictates wave propagation speed and natural frequency
    bool is_severed;

    FiberEdge(std::size_t src, std::size_t tgt, double l)
        : source_node(src), target_node(tgt), length(l), is_severed(false) {}

    double edge_impedance() const {
        if (is_severed) return std::numeric_limits<double>::infinity();
        return length; // Simple model: longer fiber = higher natural impedance
    }
};

class HarmonicRouter {
public:
    HarmonicRouter() = default;

    void add_node(const feen::ResonatorConfig& config, double bandwidth, double load) {
        nodes_.emplace_back(config, bandwidth, load);
    }

    void add_edge(std::size_t src, std::size_t tgt, double length) {
        if (src >= nodes_.size() || tgt >= nodes_.size()) {
            throw std::out_of_range("HarmonicRouter link endpoint index out of range.");
        }
        edges_.emplace_back(src, tgt, length);
    }

    RouterNode& get_node(std::size_t idx) { return nodes_.at(idx); }
    FiberEdge& get_edge(std::size_t idx) { return edges_.at(idx); }

    /**
     * @brief Route data via Harmonic Pathfinding.
     * Finds the path of least impedance by evaluating node impedance + edge impedance.
     * Severed edges return infinity. Uses Dijkstra's algorithm.
     */
    std::vector<std::size_t> find_path(std::size_t source, std::size_t target) const {
        if (source == target) return {source};
        if (source >= nodes_.size() || target >= nodes_.size()) return {};

        std::vector<double> dist(nodes_.size(), std::numeric_limits<double>::infinity());
        std::vector<std::size_t> prev(nodes_.size(), std::numeric_limits<std::size_t>::max());
        std::vector<bool> visited(nodes_.size(), false);

        dist[source] = 0.0;

        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            double min_dist = std::numeric_limits<double>::infinity();
            std::size_t u = std::numeric_limits<std::size_t>::max();

            for (std::size_t v = 0; v < nodes_.size(); ++v) {
                if (!visited[v] && dist[v] <= min_dist) {
                    min_dist = dist[v];
                    u = v;
                }
            }

            if (u == std::numeric_limits<std::size_t>::max() || min_dist == std::numeric_limits<double>::infinity()) {
                break; // remaining nodes are inaccessible
            }

            if (u == target) break; // found target

            visited[u] = true;

            for (const auto& edge : edges_) {
                if (edge.source_node == u) {
                    std::size_t v = edge.target_node;
                    if (!visited[v]) {
                        double weight = edge.edge_impedance() + nodes_[v].impedance();
                        if (dist[u] + weight < dist[v]) {
                            dist[v] = dist[u] + weight;
                            prev[v] = u;
                        }
                    }
                } else if (edge.target_node == u) {
                    // Assuming undirected edges for pathfinding, though specified source->target
                    // Wait, our add_edge logic does source->target. Let's make it bidirectional for realistic network routing.
                    std::size_t v = edge.source_node;
                    if (!visited[v]) {
                        double weight = edge.edge_impedance() + nodes_[v].impedance();
                        if (dist[u] + weight < dist[v]) {
                            dist[v] = dist[u] + weight;
                            prev[v] = u;
                        }
                    }
                }
            }
        }

        std::vector<std::size_t> path;
        std::size_t curr = target;
        if (prev[curr] != std::numeric_limits<std::size_t>::max() || curr == source) {
            while (curr != std::numeric_limits<std::size_t>::max()) {
                path.insert(path.begin(), curr);
                curr = prev[curr];
            }
        }

        return path;
    }

    /**
     * @brief Ground-to-Orbit Handoff Interface.
     * Continuous phase-lock mechanics between static ground nodes and fast-moving orbital nodes.
     * Uses Doppler mapping: as satellite moves, ground station gracefully shifts frequency.
     */
    void orbit_handoff(std::size_t ground_node_idx, const SatelliteDomain::SwarmNode& satellite_node, double doppler_shift_hz) {
        if (ground_node_idx >= nodes_.size()) {
            throw std::out_of_range("Ground node index out of range.");
        }

        RouterNode& ground_node = nodes_[ground_node_idx];
        const feen::Resonator& sat_core = satellite_node.get_core();

        // Satellite target frequency + simulated Doppler shift
        double target_hz = sat_core.frequency_hz() + doppler_shift_hz;

        // Ground station gracefully shifts its frequency to maintain constructive interference
        // (Since frequency_hz is const in ResonatorConfig and beta/config are private,
        //  we inject phase adjustments or simulate the shift via resonance locking.)
        // In FEEN, phase-locking means shifting state to match the target wave
        double target_omega = target_hz * 2.0 * M_PI;
        double current_t = ground_node.get_core().t();

        // Force state to match satellite's shifted phase
        double new_x = std::cos(target_omega * current_t);
        double new_v = -target_omega * std::sin(target_omega * current_t);

        ground_node.get_core().set_state(new_x, new_v, current_t);
    }

private:
    std::vector<RouterNode> nodes_;
    std::vector<FiberEdge> edges_;
};

} // namespace InternetDomain
} // namespace FEEN

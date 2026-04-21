#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <array>

#include <feen/energy/energy_mesh.h>

int main() {
    using feen::EnergyDomain::CoherenceObserver;
    using feen::EnergyDomain::EnergyMesh;
    using feen::EnergyDomain::GainOperator;

    // Basic mesh lifecycle
    EnergyMesh mesh;
    const std::size_t a = mesh.add_der_node();
    const std::size_t b = mesh.add_der_node(250.0);
    assert(a == 0);
    assert(b == 1);

    mesh.add_transmission_line(a, b, 0.1);
    mesh.apply_gain(a, GainOperator(5.0), 0.0);
    mesh.tick(1e-4);

    // Observer should produce bounded coherence
    CoherenceObserver observer;
    const double r = observer.compute_order_parameter(mesh.network());
    assert(std::isfinite(r));
    assert(r >= 0.0 && r <= 1.0);

    // Invalid q-factor should be rejected
    try {
        mesh.add_der_node(std::numeric_limits<double>::quiet_NaN());
        assert(false && "Expected invalid_argument for NaN q_factor");
    } catch (const std::invalid_argument&) {
    }

    // Invalid phase should be rejected
    try {
        mesh.apply_gain(a, GainOperator(1.0), std::numeric_limits<double>::infinity());
        assert(false && "Expected invalid_argument for non-finite phase");
    } catch (const std::invalid_argument&) {
    }

    // Invalid dt should be rejected
    try {
        mesh.tick(std::numeric_limits<double>::quiet_NaN());
        assert(false && "Expected invalid_argument for non-finite dt");
    } catch (const std::invalid_argument&) {
    }

    // Observer threshold validation
    try {
        CoherenceObserver bad_observer(1.1);
        (void)bad_observer;
        assert(false && "Expected invalid_argument for threshold > 1");
    } catch (const std::invalid_argument&) {
    }

    // Explicit power × dt gain integration should inject finite energy.
    const double energy_before_explicit_gain = mesh.network().node(a).total_energy();
    mesh.apply_gain_for_duration(a, GainOperator(20.0), 5e-3, 0.0);
    const double energy_after_explicit_gain = mesh.network().node(a).total_energy();
    assert(std::isfinite(energy_before_explicit_gain));
    assert(std::isfinite(energy_after_explicit_gain));
    assert(energy_after_explicit_gain > energy_before_explicit_gain);

    // Phase-sensitive gain injection should preserve energy but alter state orientation.
    EnergyMesh phase_mesh;
    const std::size_t p0 = phase_mesh.add_der_node();
    const std::size_t p1 = phase_mesh.add_der_node();
    const GainOperator phase_gain(10.0);
    phase_mesh.apply_gain_for_duration(p0, phase_gain, 1e-2, 0.0);
    phase_mesh.apply_gain_for_duration(p1, phase_gain, 1e-2, M_PI / 2.0);

    const auto& node_phase_0 = phase_mesh.network().node(p0);
    const auto& node_phase_90 = phase_mesh.network().node(p1);
    const double phase_energy_diff =
        std::abs(node_phase_0.total_energy() - node_phase_90.total_energy());
    assert(phase_energy_diff <= 1e-10);
    assert(std::abs(node_phase_0.x()) > 1e-8);
    assert(std::abs(node_phase_90.x()) < 1e-8);
    assert(std::abs(node_phase_90.v()) > 1e-8);

    // Invalid gain integration dt should be rejected.
    try {
        mesh.apply_gain_for_duration(a, GainOperator(1.0), 0.0, 0.0);
        assert(false && "Expected invalid_argument for non-positive gain integration dt");
    } catch (const std::invalid_argument&) {
    }

    // After initial excitation and with no further gain, total energy should be
    // non-increasing (up to numerical tolerance).
    EnergyMesh monotonic_mesh;
    const std::size_t m0 = monotonic_mesh.add_der_node();
    const std::size_t m1 = monotonic_mesh.add_der_node();
    monotonic_mesh.add_transmission_line(m0, m1, 0.2);
    monotonic_mesh.apply_gain_for_duration(m0, GainOperator(25.0), 1e-2, 0.0);
    monotonic_mesh.apply_gain_for_duration(m1, GainOperator(15.0), 1e-2, 0.3);

    auto total_energy = [](const EnergyMesh& local_mesh) {
        double sum = 0.0;
        for (std::size_t i = 0; i < local_mesh.network().size(); ++i) {
            sum += local_mesh.network().node(i).total_energy();
        }
        return sum;
    };

    constexpr double ENERGY_MONOTONIC_TOLERANCE_FACTOR = 1e-8;

    double prev_energy = total_energy(monotonic_mesh);
    assert(std::isfinite(prev_energy));
    for (int step = 0; step < 250; ++step) {
        monotonic_mesh.tick(1e-4);
        const double current_energy = total_energy(monotonic_mesh);
        const double tolerance =
            ENERGY_MONOTONIC_TOLERANCE_FACTOR * std::max(1.0, std::abs(prev_energy));
        assert(current_energy <= prev_energy + tolerance);
        prev_energy = current_energy;
    }

    // Coherence fragmentation thresholds across synchronized/fragmented scenarios.
    EnergyMesh coherent_mesh;
    std::array<std::size_t, 3> coherent_nodes = {
        coherent_mesh.add_der_node(), coherent_mesh.add_der_node(), coherent_mesh.add_der_node()
    };
    for (const auto idx : coherent_nodes) {
        coherent_mesh.network().node(idx).inject(1.0, 0.0);
    }
    CoherenceObserver strict_observer(0.9);
    assert(!strict_observer.check_fragmentation(coherent_mesh.network()));

    EnergyMesh fragmented_mesh;
    std::array<std::size_t, 3> fragmented_nodes = {
        fragmented_mesh.add_der_node(), fragmented_mesh.add_der_node(), fragmented_mesh.add_der_node()
    };
    fragmented_mesh.network().node(fragmented_nodes[0]).inject(1.0, 0.0);
    fragmented_mesh.network().node(fragmented_nodes[1]).inject(1.0, 2.0 * M_PI / 3.0);
    fragmented_mesh.network().node(fragmented_nodes[2]).inject(1.0, 4.0 * M_PI / 3.0);
    CoherenceObserver fragmentation_observer(0.6);
    assert(fragmentation_observer.check_fragmentation(fragmented_mesh.network()));

    EnergyMesh mixed_mesh;
    std::array<std::size_t, 3> mixed_nodes = {
        mixed_mesh.add_der_node(), mixed_mesh.add_der_node(), mixed_mesh.add_der_node()
    };
    mixed_mesh.network().node(mixed_nodes[0]).inject(1.0, 0.0);
    mixed_mesh.network().node(mixed_nodes[1]).inject(1.0, 0.0);
    mixed_mesh.network().node(mixed_nodes[2]).inject(1.0, M_PI);
    CoherenceObserver high_threshold_observer(0.5);
    CoherenceObserver low_threshold_observer(0.2);
    assert(high_threshold_observer.check_fragmentation(mixed_mesh.network()));
    assert(!low_threshold_observer.check_fragmentation(mixed_mesh.network()));

    return 0;
}

#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "../domains/energy/EnergyMesh.hpp"

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

    return 0;
}

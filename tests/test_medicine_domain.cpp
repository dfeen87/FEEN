#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>

#include <feen/resonator.h>
#include <pharma.hpp>

int main() {
    using feen::Resonator;
    using feen::ResonatorConfig;
    using feen::pharma::AdjacencyPeriosteum;
    using feen::pharma::DuffingPolymerMatrix;

    // DuffingPolymerMatrix constructor input validation
    try {
        DuffingPolymerMatrix invalid_omega(-1.0, 0.0, 100.0, 1.0);
        (void)invalid_omega;
        assert(false && "Expected invalid_argument for non-positive omega0");
    } catch (const std::invalid_argument&) {
    }
    try {
        DuffingPolymerMatrix invalid_q(-1.0, 1.0, 0.0, 1.0);
        (void)invalid_q;
        assert(false && "Expected invalid_argument for non-positive q");
    } catch (const std::invalid_argument&) {
    }
    try {
        DuffingPolymerMatrix invalid_tau(-1.0, 1.0, 100.0, -1.0);
        (void)invalid_tau;
        assert(false && "Expected invalid_argument for negative tau");
    } catch (const std::invalid_argument&) {
    }
    try {
        DuffingPolymerMatrix invalid_beta(std::numeric_limits<double>::quiet_NaN(), 1.0, 100.0, 1.0);
        (void)invalid_beta;
        assert(false && "Expected invalid_argument for non-finite beta");
    } catch (const std::invalid_argument&) {
    }

    // process_wave_data validation and behavior
    ResonatorConfig cfg;
    cfg.frequency_hz = 10.0;
    cfg.q_factor = 100.0;
    cfg.beta = 1e-3;
    Resonator resonator(cfg);
    resonator.inject(0.01, 0.0);

    DuffingPolymerMatrix matrix(-1.0, 1.0, 100.0, 1.0);
    assert(!matrix.is_released());

    matrix.process_wave_data(resonator, 0.0);
    assert(!matrix.is_released());

    matrix.process_wave_data(resonator, 10.0);
    assert(matrix.is_released());

    try {
        DuffingPolymerMatrix invalid_force_target(-1.0, 1.0, 100.0, 1.0);
        invalid_force_target.process_wave_data(resonator, std::numeric_limits<double>::infinity());
        assert(false && "Expected invalid_argument for non-finite force");
    } catch (const std::invalid_argument&) {
    }

    // AdjacencyPeriosteum connection hardening
    AdjacencyPeriosteum graph(3);
    graph.add_connection(0, 1, 2.0);
    graph.add_connection(1, 2, 1.0);

    auto laplacian = graph.calculate_laplacian();
    assert(laplacian.size() == 3);
    assert(std::abs(laplacian[0][0] - 2.0) < 1e-12);
    assert(std::abs(laplacian[0][1] + 2.0) < 1e-12);
    assert(std::isfinite(graph.calculate_laplacian_stability()));

    try {
        graph.add_connection(0, 3, 1.0);
        assert(false && "Expected out_of_range for invalid node index");
    } catch (const std::out_of_range&) {
    }
    try {
        graph.add_connection(1, 1, 1.0);
        assert(false && "Expected invalid_argument for self-loop");
    } catch (const std::invalid_argument&) {
    }
    try {
        graph.add_connection(1, 2, -1.0);
        assert(false && "Expected invalid_argument for negative weight");
    } catch (const std::invalid_argument&) {
    }
    try {
        graph.add_connection(1, 2, std::numeric_limits<double>::quiet_NaN());
        assert(false && "Expected invalid_argument for non-finite weight");
    } catch (const std::invalid_argument&) {
    }

    return 0;
}

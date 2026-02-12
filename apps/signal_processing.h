// =============================================================================
// apps/signal_processing.h
// =============================================================================
//
// Phononic signal processing primitives built from ResonatorNetwork motifs.
// A PhononicFilter is a small bank of resonators configured as a bandpass.
//
// =============================================================================

#pragma once

#include <vector>
#include <stdexcept>
#include <cmath>

#include <feen/resonator.h>
#include <feen/network.h>

namespace feen {

// =============================================================================
// PhononicFilter
// =============================================================================

class PhononicFilter {
public:
    PhononicFilter() = default;

    // Creates a crude bandpass by populating a bank of resonators spanning
    // [f_low, f_high] with modest Q and weak internal coupling.
    void design_bandpass(double f_low,
                         double f_high,
                         int taps = 16,
                         double Q = 2'000.0,
                         double coupling = 0.01)
    {
        if (f_low <= 0.0 || f_high <= 0.0 || f_high <= f_low)
            throw std::invalid_argument("Invalid bandpass range");
        if (taps < 1)
            throw std::invalid_argument("taps must be >= 1");
        if (Q <= 0.0)
            throw std::invalid_argument("Q must be > 0");

        filter_bank_ = ResonatorNetwork{};
        output_index_ = -1;

        // Create resonators distributed across the passband
        for (int i = 0; i < taps; ++i) {
            double alpha = (taps == 1) ? 0.0 : static_cast<double>(i) / (taps - 1);
            double f = f_low + alpha * (f_high - f_low);

            ResonatorConfig cfg;
            cfg.name = "bp_" + std::to_string(i);
            cfg.frequency_hz = f;
            cfg.q_factor = Q;
            cfg.beta = 0.0;

            filter_bank_.add_node(Resonator(cfg));
        }

        // Output resonator at band center to accumulate energy
        {
            double f0 = 0.5 * (f_low + f_high);
            ResonatorConfig out;
            out.name = "bp_out";
            out.frequency_hz = f0;
            out.q_factor = Q;
            out.beta = 0.0;

            output_index_ = filter_bank_.add_node(Resonator(out));
        }

        // Couple all taps into the output (fan-in)
        for (int i = 0; i < taps; ++i)
            filter_bank_.add_coupling(output_index_, i, coupling);
    }

    // Apply one sample: inject into all taps, tick network, read output energy.
    // This is an intentionally simple "streaming" interface.
    double apply(double input_signal, double dt = 1e-6, int steps = 1) {
        if (output_index_ < 0)
            throw std::runtime_error("Filter not designed");

        // Drive all taps equally; physical designs can do weighted injection.
        for (int i = 0; i < filter_bank_.size(); ++i) {
            if (i == output_index_) continue;
            filter_bank_.node(i).inject(input_signal);
        }

        for (int s = 0; s < steps; ++s)
            filter_bank_.tick_parallel(dt);

        return filter_bank_.node(output_index_).total_energy();
    }

    [[nodiscard]] int output_index() const noexcept { return output_index_; }

private:
    ResonatorNetwork filter_bank_;
    int output_index_ = -1;
};

} // namespace feen

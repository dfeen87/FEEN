// =============================================================================
// apps/oscillator_bank.h
// =============================================================================
//
// A multiplexing-friendly oscillator bank: a set of independent resonators,
// addressable by frequency.
//
// =============================================================================

#pragma once

#include <map>
#include <stdexcept>
#include <string>

#include <feen/resonator.h>

namespace feen {

// =============================================================================
// OscillatorBank
// =============================================================================

class OscillatorBank {
public:
    OscillatorBank() = default;

    void add_channel(double freq_hz,
                     double Q,
                     double beta = 0.0,
                     const std::string& name = "")
    {
        if (freq_hz <= 0.0) throw std::invalid_argument("freq_hz must be > 0");
        if (Q <= 0.0) throw std::invalid_argument("Q must be > 0");

        ResonatorConfig cfg;
        cfg.name = name.empty() ? ("osc_" + std::to_string(static_cast<long long>(freq_hz))) : name;
        cfg.frequency_hz = freq_hz;
        cfg.q_factor = Q;
        cfg.beta = beta;

        frequency_map_.emplace(freq_hz, Resonator(cfg));
    }

    // Inject per-frequency values into corresponding resonators.
    // Frequencies not present are ignored by default; set strict=true to enforce.
    void multiplex_signals(const std::map<double, double>& data,
                           bool strict = false)
    {
        for (const auto& [freq, value] : data) {
            auto it = frequency_map_.find(freq);
            if (it == frequency_map_.end()) {
                if (strict) throw std::out_of_range("Unknown oscillator channel");
                continue;
            }
            it->second.inject(value);
        }
    }

    void tick_all(double dt) {
        for (auto& [_, r] : frequency_map_) {
            r.tick(dt);
        }
    }

    [[nodiscard]] const std::map<double, Resonator>& channels() const noexcept {
        return frequency_map_;
    }

private:
    std::map<double, Resonator> frequency_map_;
};

} // namespace feen

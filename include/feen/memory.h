#pragma once

#include <vector>
#include <stdexcept>

#include "resonator.h"

namespace feen {

// =============================================================================
// Resonator Memory
// =============================================================================
//
// Memory cells are resonators whose stored value is encoded as sustained energy.
// Validity is determined by SNR relative to thermal noise.
// Refresh explicitly re-injects energy to counter dissipation.
//
class ResonatorMemory {
public:
    explicit ResonatorMemory(const std::vector<ResonatorConfig>& configs) {
        for (const auto& cfg : configs) {
            cells_.emplace_back(cfg);
        }
    }

    std::size_t size() const noexcept { return cells_.size(); }

    // -------------------------------------------------------------------------
    // Write: encode value as injected amplitude
    // -------------------------------------------------------------------------
    void write(std::size_t address, double amplitude, double phase = 0.0) {
        bounds_check_(address);
        cells_[address].inject(amplitude, phase);
    }

    // -------------------------------------------------------------------------
    // Read: return stored energy (not a Boolean)
    // -------------------------------------------------------------------------
    double read(std::size_t address) const {
        bounds_check_(address);
        return cells_[address].total_energy();
    }

    // -------------------------------------------------------------------------
    // Validity: check if signal exceeds thermal noise floor
    // -------------------------------------------------------------------------
    bool is_valid(std::size_t address,
                  double min_snr = MIN_READABLE_SNR) const
    {
        bounds_check_(address);
        return cells_[address].snr() >= min_snr;
    }

    // -------------------------------------------------------------------------
    // Refresh: re-inject energy proportional to current state
    // -------------------------------------------------------------------------
    void refresh(std::size_t address, double gain = 1.0) {
        bounds_check_(address);

        const double x = cells_[address].x();
        const double v = cells_[address].v();

        // Estimate amplitude from phase-space radius
        const double amplitude = gain * std::sqrt(x*x + v*v);
        cells_[address].inject(amplitude);
    }

    // -------------------------------------------------------------------------
    // Time evolution (natural decay)
    // -------------------------------------------------------------------------
    void tick(double dt) {
        for (auto& cell : cells_) {
            cell.tick(dt);
        }
    }

private:
    std::vector<Resonator> cells_;

    void bounds_check_(std::size_t address) const {
        if (address >= cells_.size()) {
            throw std::out_of_range("ResonatorMemory address out of range");
        }
    }
};

} // namespace feen

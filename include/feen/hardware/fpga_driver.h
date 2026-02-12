#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace feen {

// =============================================================================
// TransducerConfig
// =============================================================================
//
// Hardware-facing configuration for an electrical↔phononic transducer channel.
// Keep this intentionally minimal and extensible.
//
struct TransducerConfig {
    int dac_channel = -1;          // FPGA DAC channel index
    int adc_channel = -1;          // FPGA ADC channel index
    double efficiency = 1.0;       // (0,1]
    double impedance_ohm = 50.0;   // > 0
    double v_min = -1.0;           // DAC clamp min
    double v_max = 1.0;            // DAC clamp max
    std::string label;             // Optional human-readable name
};

// =============================================================================
// FPGADriver
// =============================================================================
//
// Real FPGA control for physical resonators.
// This header defines the interface and safety checks; implementation is platform
// specific (SPI/I2C/PCIe/Ethernet, register maps, DMA, etc.).
//
class FPGADriver {
public:
    virtual ~FPGADriver() = default;

    // -------------------------------------------------------------------------
    // Low-level I/O
    // -------------------------------------------------------------------------

    virtual void write_dac(int channel, double voltage) = 0;
    virtual double read_adc(int channel) = 0;

    // -------------------------------------------------------------------------
    // Transducer configuration
    // -------------------------------------------------------------------------

    virtual void configure_transducer(int id, const TransducerConfig& cfg) {
        validate_config_(cfg);
        transducers_[id] = cfg;
    }

    [[nodiscard]] bool has_transducer(int id) const noexcept {
        return transducers_.find(id) != transducers_.end();
    }

    [[nodiscard]] const TransducerConfig& transducer(int id) const {
        auto it = transducers_.find(id);
        if (it == transducers_.end()) throw std::out_of_range("Unknown transducer id");
        return it->second;
    }

    // -------------------------------------------------------------------------
    // Convenience helpers
    // -------------------------------------------------------------------------

    void write_transducer_voltage(int id, double voltage) {
        const auto& cfg = transducer(id);
        write_dac(cfg.dac_channel, clamp_(voltage, cfg.v_min, cfg.v_max));
    }

    double read_transducer_voltage(int id) {
        const auto& cfg = transducer(id);
        return read_adc(cfg.adc_channel);
    }

protected:
    std::unordered_map<int, TransducerConfig> transducers_;

private:
    static void validate_config_(const TransducerConfig& cfg) {
        if (cfg.dac_channel < 0) throw std::invalid_argument("TransducerConfig: dac_channel must be >= 0");
        if (cfg.adc_channel < 0) throw std::invalid_argument("TransducerConfig: adc_channel must be >= 0");
        if (!(cfg.efficiency > 0.0 && cfg.efficiency <= 1.0)) throw std::invalid_argument("TransducerConfig: efficiency must be in (0,1]");
        if (cfg.impedance_ohm <= 0.0) throw std::invalid_argument("TransducerConfig: impedance_ohm must be > 0");
        if (cfg.v_max <= cfg.v_min) throw std::invalid_argument("TransducerConfig: v_max must be > v_min");
    }

    static double clamp_(double x, double lo, double hi) noexcept {
        if (x < lo) return lo;
        if (x > hi) return hi;
        return x;
    }
};

} // namespace feen

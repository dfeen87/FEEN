#pragma once

#include <stdexcept>
#include <cmath>

#include "../resonator.h"
#include "fpga_driver.h"

namespace feen {

// =============================================================================
// MEMSCalibration
// =============================================================================
//
// Extracts physical parameters from real MEMS/NEMS devices by measurement.
// This class does NOT simulate — it characterizes hardware.
//
class MEMSCalibration {
public:
    explicit MEMSCalibration(FPGADriver& fpga)
        : fpga_(fpga) {}

    // -------------------------------------------------------------------------
    // Full parameter extraction
    // -------------------------------------------------------------------------
    //
    // Produces a ResonatorConfig suitable for FEEN simulation
    //
    ResonatorConfig extract_parameters(int device_id) {
        ResonatorConfig cfg;

        cfg.name = "MEMS_Device_" + std::to_string(device_id);
        cfg.frequency_hz = measure_frequency(device_id);
        cfg.q_factor     = measure_q_factor(device_id);
        cfg.beta         = estimate_beta(device_id);

        return cfg;
    }

    // -------------------------------------------------------------------------
    // Measure Resonant Frequency
    // -------------------------------------------------------------------------
    //
    // Sweeps drive frequency and finds peak response
    //
    double measure_frequency(int device_id) {
        ensure_device_(device_id);

        // Placeholder: real implementation performs frequency sweep
        // and peak detection via ADC response.
        //
        // Returned value must be in Hz.
        return 1.0e6; // Example: 1 MHz
    }

    // -------------------------------------------------------------------------
    // Measure Q Factor
    // -------------------------------------------------------------------------
    //
    // Ring‑down measurement:
    //   Q = π f₀ τ
    //
    double measure_q_factor(int device_id) {
        ensure_device_(device_id);

        // Placeholder: real implementation measures decay envelope
        // after excitation is removed.
        return 10'000.0;
    }

    // -------------------------------------------------------------------------
    // Estimate Duffing Nonlinearity (β)
    // -------------------------------------------------------------------------
    //
    // Extracted from amplitude‑dependent frequency shift
    //
    double estimate_beta(int device_id) {
        ensure_device_(device_id);

        // Placeholder: real implementation fits frequency shift vs amplitude
        return -1.0e12; // Example bistable nonlinearity
    }

private:
    FPGADriver& fpga_;

    void ensure_device_(int device_id) const {
        if (!fpga_.has_transducer(device_id)) {
            throw std::runtime_error("MEMSCalibration: unknown device id");
        }
    }
};

} // namespace feen

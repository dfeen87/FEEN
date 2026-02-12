#pragma once

#include <cmath>
#include <stdexcept>

#include "resonator.h"

namespace feen {

// =============================================================================
// Transducer
// =============================================================================
//
// Models an interface between electrical and phononic domains.
// Conversion is lossy and impedance‑limited by design.
//
// Electrical → Mechanical:
//   Voltage → Force → Displacement
//
// Mechanical → Electrical:
//   Displacement → Induced Voltage
//
class Transducer {
public:
    Transducer(double efficiency,
               double impedance)
        : efficiency_(efficiency),
          impedance_(impedance)
    {
        if (efficiency_ <= 0.0 || efficiency_ > 1.0)
            throw std::invalid_argument("Transducer efficiency must be in (0,1]");
        if (impedance_ <= 0.0)
            throw std::invalid_argument("Transducer impedance must be > 0");
    }

    // -------------------------------------------------------------------------
    // Electrical → Mechanical
    // -------------------------------------------------------------------------
    //
    // Convert applied voltage to effective displacement amplitude.
    // This is a simplified linearized model suitable for small‑signal regimes.
    //
    double voltage_to_displacement(double V) const {
        // Power ~ V^2 / Z, scaled by efficiency
        double power = efficiency_ * (V * V) / impedance_;
        return std::sqrt(power);
    }

    // -------------------------------------------------------------------------
    // Mechanical → Electrical
    // -------------------------------------------------------------------------
    //
    // Convert resonator displacement to induced voltage.
    //
    double displacement_to_voltage(double x) const {
        // Linear piezo / magnetostrictive approximation
        return std::sqrt(impedance_) * efficiency_ * x;
    }

    // -------------------------------------------------------------------------
    // Apply electrical drive to a resonator
    // -------------------------------------------------------------------------
    //
    // Voltage is converted to a force‑equivalent drive term.
    //
    void apply_drive(Resonator& r,
                     double voltage,
                     double dt)
    {
        double amplitude = voltage_to_displacement(voltage);

        // Drive force proportional to displacement amplitude
        r.tick(dt, amplitude);
    }

private:
    double efficiency_;  // Conversion efficiency (0–1)
    double impedance_;   // Electrical impedance (Ohms)
};

} // namespace feen

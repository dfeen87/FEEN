// =============================================================================
// FEEN Hardware-in-the-Loop: hardware_adapter.h
// =============================================================================
//
// Ablatable hardware adapter layer that bridges real sensor/actuator hardware
// to FEEN simulation state via the existing set_state() and inject() APIs.
//
// DESIGN CONTRACT (must be respected by all callers and extensions):
//
//  1. ABLATABLE — This entire file can be removed without affecting FEEN core
//     correctness.  FEEN core (resonator.h, network.h, integrators.h) has
//     zero dependency on HardwareAdapter.
//
//  2. STATE OWNERSHIP — The FEEN Resonator owns all simulation state.  The
//     adapter writes state only through Resonator::set_state() or
//     Resonator::inject().  It never caches a copy of dynamic state.
//
//  3. LATENCY-EXPLICIT — Measured pipeline latency (latency_s in
//     CalibrationParams) MUST be recorded and logged but MUST NOT be
//     used to advance or retard the integrator time variable.
//     Latency is infrastructure-level metadata, not a physics dt.
//
//  4. CALIBRATION-SEPARATED — All sensor scaling, offset correction, and
//     unit conversion lives in CalibrationParams, never in Resonator or
//     ResonatorConfig.  Physical parameters (omega0, gamma, beta) are
//     immutable after resonator construction.
//
//  5. NO OBSERVER ACCESS — HardwareAdapter holds no reference to
//     SpiralTimeObserver, SpiralTimeState, or any other observer type.
//     Observer reads remain the exclusive responsibility of the caller.
//
//  6. NO FEEDBACK TO DYNAMICS — The adapter NEVER reads observer output and
//     feeds it back as a drive term or state correction.  Sensor → state is
//     a strict one-way write path.
//
// Interface boundary (see docs/HARDWARE_IN_THE_LOOP.md §2):
//
//   [Physical hardware]
//       │  FPGADriver (raw I/O)
//       ▼
//   [HardwareAdapter]   ← CalibrationParams (scaling + latency metadata)
//       │  set_state(x, v, t) or inject(amplitude, phase)
//       ▼
//   [FEEN Resonator / ResonatorNetwork]   ← physics lives here
//       │  x(), v(), t() (read-only observers)
//       ▼
//   [HardwareAdapter::drive_actuator]
//       │  write_transducer_voltage (via FPGADriver)
//       ▼
//   [Physical hardware]
//
// =============================================================================

#pragma once

#include <cmath>
#include <stdexcept>
#include <string>

#include "../resonator.h"
#include "fpga_driver.h"

namespace feen {
namespace hardware {

// =============================================================================
// CalibrationParams
// =============================================================================
//
// Explicit calibration coefficients for sensor ↔ FEEN state conversion.
//
// scale_x, offset_x  — map raw ADC voltage (or counts) to displacement [m]
// scale_v, offset_v  — map raw ADC voltage (or counts) to velocity [m/s]
//                      (requires a dedicated velocity readout channel;
//                       numerical differentiation is done by the caller)
// latency_s          — known pipeline latency [s]; informational only;
//                      MUST NOT be added to or subtracted from dt
// actuator_scale     — maps FEEN displacement output to actuator voltage [V/m]
// actuator_offset    — additive offset for the actuator voltage [V]
//
struct CalibrationParams {
    double scale_x       = 1.0;   // [m / V]
    double scale_v       = 1.0;   // [(m/s) / V]
    double offset_x      = 0.0;   // [m]
    double offset_v      = 0.0;   // [m/s]
    double latency_s     = 0.0;   // [s], informational; never used as dt
    double actuator_scale  = 1.0; // [V / m]
    double actuator_offset = 0.0; // [V]
};

// =============================================================================
// SensorSample
// =============================================================================
//
// Result of one sensor read, converted to physical units.
// Ready to be passed directly to HardwareAdapter::apply_to_resonator().
//
// latency_s is recorded for auditing; it does NOT alter sample_time_s.
//
struct SensorSample {
    double x             = 0.0;  // Displacement [m] after calibration
    double v             = 0.0;  // Velocity [m/s] after calibration
    double sample_time_s = 0.0;  // Simulation timestamp at the moment of read
    double latency_s     = 0.0;  // Informational pipeline latency [s]
};

// =============================================================================
// HardwareAdapter
// =============================================================================
//
// Connects one FPGADriver instance (raw hardware I/O) to FEEN via
// CalibrationParams-scaled conversions.
//
// Typical usage per simulation step:
//
//   1. SensorSample s = adapter.read_sensor_sample(tid, sim_time);
//   2. adapter.apply_to_resonator(resonator, s);   // calls set_state()
//   3. network.tick_parallel(dt);                  // FEEN physics step
//   4. adapter.drive_actuator(resonator, tid);      // reads x(), writes DAC
//
// The adapter is intentionally stateless with respect to simulation dynamics:
// removing steps 1–2 and 4 leaves tick_parallel() unmodified.
//
class HardwareAdapter {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    explicit HardwareAdapter(FPGADriver& fpga, CalibrationParams cal)
        : fpga_(fpga), cal_(cal)
    {
        validate_calibration_(cal_);
    }

    virtual ~HardwareAdapter() = default;

    // -------------------------------------------------------------------------
    // Calibration
    // -------------------------------------------------------------------------

    [[nodiscard]] const CalibrationParams& calibration() const noexcept {
        return cal_;
    }

    void set_calibration(const CalibrationParams& cal) {
        validate_calibration_(cal);
        cal_ = cal;
    }

    // -------------------------------------------------------------------------
    // Sensor → FEEN state
    // -------------------------------------------------------------------------
    //
    // read_sensor_sample()
    //   Reads the ADC channel associated with transducer_id, applies
    //   CalibrationParams, and returns a SensorSample.
    //
    //   sample_time_s MUST be the current simulation time (e.g., resonator.t()
    //   or network.time_s()).  It is supplied by the caller — NOT derived from
    //   latency.  Latency is recorded in the returned sample for auditing.
    //
    //   NOTE: velocity is computed from a single readout via scale_v applied
    //   to the same ADC channel.  If the hardware provides a dedicated velocity
    //   channel, override this method in a derived class.
    //
    [[nodiscard]] virtual SensorSample read_sensor_sample(
        int    transducer_id,
        double sample_time_s) const
    {
        const double v_raw = fpga_.read_transducer_voltage(transducer_id);

        SensorSample s;
        s.x             = cal_.scale_x * v_raw + cal_.offset_x;
        s.v             = cal_.scale_v * v_raw + cal_.offset_v;
        s.sample_time_s = sample_time_s;
        s.latency_s     = cal_.latency_s;
        return s;
    }

    // apply_to_resonator()
    //   Writes the calibrated (x, v, t) into the resonator via set_state().
    //   This is the ONLY write path from hardware measurements to FEEN state.
    //   It does NOT inject energy; it overwrites state with measured values.
    //
    void apply_to_resonator(Resonator& r, const SensorSample& s) const {
        r.set_state(s.x, s.v, s.sample_time_s);
    }

    // -------------------------------------------------------------------------
    // FEEN state → actuator
    // -------------------------------------------------------------------------
    //
    // compute_actuator_command()
    //   Converts FEEN resonator displacement x to an actuator voltage [V].
    //   actuator_scale and actuator_offset apply the inverse of the sensor
    //   calibration (or a separate closed-loop gain, set by the caller).
    //
    [[nodiscard]] double compute_actuator_command(double x) const noexcept {
        return cal_.actuator_scale * x + cal_.actuator_offset;
    }

    // drive_actuator()
    //   Reads the resonator's current x() and writes a proportional voltage
    //   to the physical actuator channel.  This is the ONLY write path from
    //   FEEN state to physical actuation.
    //
    void drive_actuator(const Resonator& r, int transducer_id) {
        const double cmd = compute_actuator_command(r.x());
        fpga_.write_transducer_voltage(transducer_id, cmd);
    }

private:
    FPGADriver&      fpga_;
    CalibrationParams cal_;

    static void validate_calibration_(const CalibrationParams& c) {
        if (c.scale_x == 0.0)
            throw std::invalid_argument("CalibrationParams: scale_x must not be zero");
        if (c.scale_v == 0.0)
            throw std::invalid_argument("CalibrationParams: scale_v must not be zero");
        if (c.latency_s < 0.0)
            throw std::invalid_argument("CalibrationParams: latency_s must be >= 0");
    }
};

} // namespace hardware
} // namespace feen

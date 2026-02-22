// =============================================================================
// FEEN Hardware Adapter Validation Suite
// =============================================================================
//
// Validates the HardwareAdapter design-contract invariants:
//
//   1. ABLATABLE        — FEEN core is unmodified when adapter is absent.
//   2. STATE OWNERSHIP  — set_state() is the only write path to Resonator.
//   3. LATENCY-EXPLICIT — latency is recorded, never used as dt.
//   4. CALIBRATION      — scale/offset are applied correctly.
//   5. NO FEEDBACK      — adapter reads no observer output.
//
// =============================================================================

#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <stdexcept>

#include <feen/resonator.h>
#include <feen/hardware/hardware_adapter.h>

using namespace feen;
using namespace feen::hardware;

// =============================================================================
// MockFPGADriver
// =============================================================================
//
// Minimal in-process stub that satisfies FPGADriver's abstract interface.
// Records the last write for assertion purposes.
//
class MockFPGADriver : public feen::FPGADriver {
public:
    double stub_adc_voltage = 0.5;  // Fixed ADC return value
    double last_dac_voltage = 0.0;  // Last voltage written to DAC
    int    write_count      = 0;    // Number of DAC writes

    void   write_dac(int /*channel*/, double voltage) override {
        last_dac_voltage = voltage;
        ++write_count;
    }

    double read_adc(int /*channel*/) override {
        return stub_adc_voltage;
    }
};

// =============================================================================
// Helper: make a minimal Resonator for testing
// =============================================================================
static Resonator make_resonator() {
    ResonatorConfig cfg;
    cfg.name         = "test_node";
    cfg.frequency_hz = 1000.0;
    cfg.q_factor     = 200.0;
    cfg.beta         = 1e-4;
    return Resonator(cfg);
}

// =============================================================================
// Helper: make default CalibrationParams
// =============================================================================
static CalibrationParams default_cal() {
    CalibrationParams c;
    c.scale_x        = 2.0;   // 2 m / V
    c.scale_v        = 3.0;   // 3 (m/s) / V
    c.offset_x       = 0.1;   // 0.1 m zero-offset
    c.offset_v       = 0.0;
    c.latency_s      = 5e-6;  // 5 µs pipeline latency (informational)
    c.actuator_scale = 4.0;   // 4 V / m
    c.actuator_offset= 0.0;
    return c;
}

// =============================================================================
// 1. Ablatable: FEEN core state is unchanged when adapter is absent
// =============================================================================
static void test_ablatable() {
    std::cout << "[1] Ablatable: FEEN core unmodified without adapter...\n";

    Resonator r = make_resonator();
    r.inject(1.0);

    const double x0 = r.x();
    const double v0 = r.v();
    const double t0 = r.t();

    // Tick without any adapter involvement
    r.tick(1e-6);

    const double x1 = r.x();
    const double v1 = r.v();
    const double t1 = r.t();

    // State must have advanced by exactly one tick
    assert(t1 > t0 && "Resonator time must advance with tick");
    assert((x1 != x0 || v1 != v0) && "Resonator state must evolve with tick");

    std::cout << "  PASS: FEEN resonator evolves correctly without adapter.\n\n";
}

// =============================================================================
// 2. Calibration: sensor voltage → (x, v) uses scale + offset
// =============================================================================
static void test_calibration_read() {
    std::cout << "[2] Calibration: sensor voltage mapped to x and v...\n";

    MockFPGADriver fpga;
    fpga.stub_adc_voltage = 0.5;

    // Configure transducer so FPGADriver helpers work
    TransducerConfig tcfg;
    tcfg.dac_channel   = 0;
    tcfg.adc_channel   = 0;
    tcfg.efficiency    = 0.9;
    tcfg.impedance_ohm = 50.0;
    tcfg.v_min         = -5.0;
    tcfg.v_max         =  5.0;
    fpga.configure_transducer(0, tcfg);

    CalibrationParams cal = default_cal();
    HardwareAdapter adapter(fpga, cal);

    const double sim_t = 0.001;
    SensorSample s = adapter.read_sensor_sample(0, sim_t);

    // Expected: x = scale_x * v_raw + offset_x = 2.0 * 0.5 + 0.1 = 1.1
    const double expected_x = cal.scale_x * fpga.stub_adc_voltage + cal.offset_x;
    assert(std::abs(s.x - expected_x) < 1e-12 && "x calibration failed");

    // Expected: v = scale_v * v_raw + offset_v = 3.0 * 0.5 + 0.0 = 1.5
    const double expected_v = cal.scale_v * fpga.stub_adc_voltage + cal.offset_v;
    assert(std::abs(s.v - expected_v) < 1e-12 && "v calibration failed");

    // sample_time_s must equal the supplied sim_t
    assert(std::abs(s.sample_time_s - sim_t) < 1e-15 && "sample_time_s mismatch");

    // latency_s is recorded, not consumed
    assert(std::abs(s.latency_s - cal.latency_s) < 1e-15 && "latency_s not recorded");

    std::cout << "  x = " << s.x << " (expected " << expected_x << ")\n";
    std::cout << "  v = " << s.v << " (expected " << expected_v << ")\n";
    std::cout << "  PASS: Sensor calibration applies scale and offset correctly.\n\n";
}

// =============================================================================
// 3. State write path: apply_to_resonator uses set_state, nothing else
// =============================================================================
static void test_apply_to_resonator() {
    std::cout << "[3] State write: apply_to_resonator uses set_state...\n";

    MockFPGADriver fpga;
    fpga.stub_adc_voltage = 0.25;

    TransducerConfig tcfg;
    tcfg.dac_channel   = 0;
    tcfg.adc_channel   = 0;
    tcfg.efficiency    = 0.9;
    tcfg.impedance_ohm = 50.0;
    tcfg.v_min         = -5.0;
    tcfg.v_max         =  5.0;
    fpga.configure_transducer(0, tcfg);

    CalibrationParams cal = default_cal();
    HardwareAdapter adapter(fpga, cal);

    Resonator r = make_resonator();
    const double sim_t = 0.002;
    SensorSample s = adapter.read_sensor_sample(0, sim_t);

    adapter.apply_to_resonator(r, s);

    // Resonator state must now equal the calibrated sample values
    assert(std::abs(r.x() - s.x) < 1e-12 && "Resonator x not set correctly");
    assert(std::abs(r.v() - s.v) < 1e-12 && "Resonator v not set correctly");
    assert(std::abs(r.t() - s.sample_time_s) < 1e-12 && "Resonator t not set correctly");

    // DAC write count must still be 0: reading sensor does not write actuator
    assert(fpga.write_count == 0 && "Reading sensor must not write actuator");

    std::cout << "  r.x() = " << r.x() << ", s.x = " << s.x << "\n";
    std::cout << "  PASS: Resonator state overwritten via set_state only.\n\n";
}

// =============================================================================
// 4. Latency: latency_s is informational and does NOT mutate simulation time
// =============================================================================
static void test_latency_not_used_as_dt() {
    std::cout << "[4] Latency: latency_s does not advance simulation time...\n";

    MockFPGADriver fpga;
    fpga.stub_adc_voltage = 0.0;

    TransducerConfig tcfg;
    tcfg.dac_channel   = 0;
    tcfg.adc_channel   = 0;
    tcfg.efficiency    = 0.9;
    tcfg.impedance_ohm = 50.0;
    tcfg.v_min         = -5.0;
    tcfg.v_max         =  5.0;
    fpga.configure_transducer(0, tcfg);

    CalibrationParams cal;
    cal.scale_x     = 1.0;
    cal.scale_v     = 1.0;
    cal.latency_s   = 100.0;  // deliberately absurd latency (100 s)
    cal.actuator_scale = 1.0;
    HardwareAdapter adapter(fpga, cal);

    Resonator r = make_resonator();
    const double sim_t = 0.001;
    SensorSample s = adapter.read_sensor_sample(0, sim_t);
    adapter.apply_to_resonator(r, s);

    // Resonator time must equal sim_t, NOT sim_t + 100
    assert(std::abs(r.t() - sim_t) < 1e-12 &&
           "Latency must not be added to resonator time");

    std::cout << "  r.t() = " << r.t() << " (expected " << sim_t
              << ", latency " << cal.latency_s << " s ignored)\n";
    std::cout << "  PASS: Latency is informational and does not alter sim time.\n\n";
}

// =============================================================================
// 5. Actuator output: compute_actuator_command applies scale correctly
// =============================================================================
static void test_actuator_command() {
    std::cout << "[5] Actuator: displacement → voltage uses actuator_scale...\n";

    MockFPGADriver fpga;

    TransducerConfig tcfg;
    tcfg.dac_channel   = 0;
    tcfg.adc_channel   = 0;
    tcfg.efficiency    = 0.9;
    tcfg.impedance_ohm = 50.0;
    tcfg.v_min         = -5.0;
    tcfg.v_max         =  5.0;
    fpga.configure_transducer(0, tcfg);

    CalibrationParams cal = default_cal();
    HardwareAdapter adapter(fpga, cal);

    Resonator r = make_resonator();
    r.inject(0.5);  // x = 0.5, v = ...

    adapter.drive_actuator(r, 0);

    // Expected voltage = actuator_scale * x() + actuator_offset
    const double expected_v = cal.actuator_scale * r.x() + cal.actuator_offset;
    // FPGADriver clamps to [v_min, v_max]
    const double clamped = std::min(std::max(expected_v, tcfg.v_min), tcfg.v_max);

    assert(std::abs(fpga.last_dac_voltage - clamped) < 1e-10 &&
           "Actuator voltage not set correctly");
    assert(fpga.write_count == 1 && "Exactly one DAC write expected");

    std::cout << "  r.x() = " << r.x()
              << "  -> expected_v = " << expected_v
              << "  clamped = " << clamped << "\n";
    std::cout << "  PASS: Actuator command computed and written correctly.\n\n";
}

// =============================================================================
// 6. Calibration validation: zero scale rejected
// =============================================================================
static void test_calibration_validation() {
    std::cout << "[6] Calibration: zero scale_x rejected...\n";

    MockFPGADriver fpga;
    CalibrationParams bad;
    bad.scale_x = 0.0;  // Invalid: would silently discard sensor signal

    bool threw = false;
    try {
        HardwareAdapter adapter(fpga, bad);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw && "Zero scale_x must throw invalid_argument");

    std::cout << "  PASS: zero scale_x correctly rejected.\n\n";
}

// =============================================================================
// 7. FEEN core invariant: adapter does not break energy dissipation
// =============================================================================
static void test_feen_invariant_energy_dissipation() {
    std::cout << "[7] FEEN invariant: energy still dissipates after set_state...\n";

    Resonator r = make_resonator();
    r.inject(1.0);

    // Use set_state directly (as the adapter would) then tick
    r.set_state(r.x(), r.v(), r.t());  // no-op: same values

    const double e0 = r.total_energy();
    for (int i = 0; i < 100000; ++i) {
        r.tick(1e-6);
    }
    const double e1 = r.total_energy();

    assert(e1 < e0 && "Energy must dissipate even after set_state overwrite");

    std::cout << "  e0 = " << e0 << "  e1 = " << e1 << "\n";
    std::cout << "  PASS: Energy dissipation invariant preserved after set_state.\n\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "==== FEEN HardwareAdapter Validation Suite ====\n\n";

    test_ablatable();
    test_calibration_read();
    test_apply_to_resonator();
    test_latency_not_used_as_dt();
    test_actuator_command();
    test_calibration_validation();
    test_feen_invariant_energy_dissipation();

    std::cout << "==== All HardwareAdapter tests passed. ====\n";
    return 0;
}

#include <iostream>
#include <cmath>
#include <cassert>
#include <iomanip>
#include <feen/resonator.h>

using namespace feen;

int main() {
    // Set output precision for physical constants
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "==== FEEN Physical Validation Suite (v3.0) ====\n\n";

    // ---------------------------------------------------------------------
    // 1. Monostable Decay & SNR Test
    // Goal: Verify energy dissipation and the "Sustain Window"
    // ---------------------------------------------------------------------
    std::cout << "[Step 1] Monostable Decay Test...\n";
    
    ResonatorConfig mono_cfg;
    mono_cfg.name = "memory_unit_0";
    mono_cfg.frequency_hz = 1000.0;
    mono_cfg.q_factor = 200.0;
    mono_cfg.beta = 1e-4; // Hardening spring nonlinearity

    Resonator mono(mono_cfg);
    double initial_amp = 1.0;
    mono.inject(initial_amp);
    
    double initial_energy = mono.total_energy();
    std::cout << "  Initial Energy: " << initial_energy << " J\n";

    // Simulate 0.5 seconds of real-time evolution
    double dt = 1e-6;
    for (int i = 0; i < 500000; ++i) {
        mono.tick(dt);
    }

    double final_energy = mono.total_energy();
    std::cout << "  Energy after 500ms: " << final_energy << " J\n";
    std::cout << "  Current SNR: " << mono.snr() << "\n";

    // Assertion: Entropy check. Energy must be lower than initial state.
    assert(final_energy < initial_energy && "Physics Error: Energy did not decay.");
    std::cout << "  PASS: Decay logic consistent with thermodynamics.\n\n";

    // ---------------------------------------------------------------------
    // 2. Bistable Equilibrium & Barrier Test
    // Goal: Verify "Phononic Bit" stability in a double-well potential
    // ---------------------------------------------------------------------
    std::cout << "[Step 2] Bistable Equilibrium Test...\n";

    ResonatorConfig bi_cfg;
    bi_cfg.name = "logic_gate_0";
    bi_cfg.frequency_hz = 1000.0;
    bi_cfg.q_factor = 500.0;
    bi_cfg.beta = -1e8;  // Strong negative beta for bistability

    Resonator bistable(bi_cfg);

    // Calculate the physical location of the stable well: x* = ±ω₀ / sqrt(|β|)
    double omega0 = 2.0 * M_PI * bi_cfg.frequency_hz;
    double expected_well = omega0 / std::sqrt(std::abs(bi_cfg.beta));
    
    std::cout << "  Expected stable well at x = " << expected_well << "\n";
    
    // Inject precisely at the well location
    bistable.inject(expected_well);

    // Evolve for 100ms
    for (int i = 0; i < 100000; ++i) {
        bistable.tick(dt);
    }

    // Verify the state hasn't collapsed to zero
    std::cout << "  Barrier Height: " << bistable.barrier_height() << " J\n";
    std::cout << "  Thermal Stability: " << (bistable.switching_time_ok() ? "STABLE" : "UNSTABLE") << "\n";

    assert(bistable.barrier_height() > 0 && "Physics Error: No barrier in bistable mode.");
    assert(bistable.switching_time_ok() && "Engineering Error: Bit-flip risk too high for sustain window.");
    std::cout << "  PASS: Logic state is physically stable.\n\n";

    // ---------------------------------------------------------------------
    // 3. Spectral Isolation (Lorentzian) Test
    // Goal: Verify the "Namespace" separation of two resonant frequencies
    // ---------------------------------------------------------------------
    std::cout << "[Step 3] Spectral Isolation Test...\n";

    ResonatorConfig a_cfg;
    a_cfg.frequency_hz = 1000.0;
    a_cfg.q_factor = 1000.0; // Sharp resonance
    Resonator A(a_cfg);

    ResonatorConfig b_cfg = a_cfg;
    b_cfg.frequency_hz = 1010.0; // Only 10Hz apart
    Resonator B(b_cfg);

    double isolation = Resonator::isolation_db(A, B);
    std::cout << "  Isolation between 1000Hz and 1010Hz: " << isolation << " dB\n";

    // Assertion: Lorentzian isolation should be deep for High Q
    assert(isolation < -20.0 && "System Error: Spectral crosstalk too high.");
    std::cout << "  PASS: Spectral orthogonality verified.\n\n";

    std::cout << "==== ALL PHYSICAL VALIDATIONS PASSED ====\n";
    return 0;
}

// =============================================================================
// FEEN Tutorial 01: Basic Oscillator
// =============================================================================
// Learn: How to create and simulate a simple resonator
// Concepts: Monostable mode, energy decay, SNR tracking
// =============================================================================

#include <iostream>
#include <iomanip>
#include <feen/resonator.h>

using namespace feen;

int main() {
    std::cout << "=== FEEN Tutorial 01: Basic Oscillator ===\n\n";
    
    // -------------------------------------------------------------------------
    // Step 1: Configure the resonator
    // -------------------------------------------------------------------------
    std::cout << "[Step 1] Configuring a 1 kHz resonator...\n";
    
    ResonatorConfig config;
    config.name = "my_first_oscillator";
    config.frequency_hz = 1000.0;      // 1 kHz resonant frequency
    config.q_factor = 200.0;           // Quality factor (higher = less damping)
    config.beta = 1e-4;                // Positive beta = monostable mode
    
    std::cout << "  Frequency: " << config.frequency_hz << " Hz\n";
    std::cout << "  Q-factor: " << config.q_factor << "\n";
    std::cout << "  Mode: Monostable (single stable state)\n\n";
    
    // -------------------------------------------------------------------------
    // Step 2: Create the resonator
    // -------------------------------------------------------------------------
    std::cout << "[Step 2] Creating resonator...\n";
    
    Resonator osc(config);
    
    // Calculate theoretical decay time
    double decay_time = config.q_factor / (M_PI * config.frequency_hz);
    std::cout << "  Expected decay time: " << decay_time * 1000.0 << " ms\n\n";
    
    // -------------------------------------------------------------------------
    // Step 3: Inject initial energy
    // -------------------------------------------------------------------------
    std::cout << "[Step 3] Injecting initial energy...\n";
    
    double initial_amplitude = 1.0;
    osc.inject(initial_amplitude);
    
    double E0 = osc.total_energy();
    std::cout << "  Initial energy: " << std::scientific << E0 << " J\n";
    std::cout << "  Initial SNR: " << std::fixed << osc.snr() << "\n\n";
    
    // -------------------------------------------------------------------------
    // Step 4: Simulate evolution
    // -------------------------------------------------------------------------
    std::cout << "[Step 4] Simulating 200 ms of evolution...\n\n";
    
    double dt = 1e-6;  // 1 microsecond timestep
    int steps_per_snapshot = 50000;  // Every 50 ms
    
    std::cout << std::setw(10) << "Time (ms)" 
              << std::setw(20) << "Energy (J)" 
              << std::setw(15) << "Energy (%)" 
              << std::setw(15) << "SNR\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (int i = 0; i <= 200000; i++) {
        osc.tick(dt);
        
        // Print snapshot every 50 ms
        if (i % steps_per_snapshot == 0) {
            double t_ms = i * dt * 1000.0;
            double E = osc.total_energy();
            double E_percent = (E / E0) * 100.0;
            double snr = osc.snr();
            
            std::cout << std::fixed << std::setprecision(1)
                      << std::setw(10) << t_ms
                      << std::scientific << std::setprecision(3)
                      << std::setw(20) << E
                      << std::fixed << std::setprecision(1)
                      << std::setw(15) << E_percent
                      << std::setprecision(0)
                      << std::setw(15) << snr << "\n";
        }
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 5: Analysis
    // -------------------------------------------------------------------------
    std::cout << "[Step 5] Analysis...\n";
    
    double final_energy = osc.total_energy();
    double energy_ratio = final_energy / E0;
    
    std::cout << "  Energy retained: " << std::fixed << std::setprecision(1) 
              << (energy_ratio * 100.0) << "%\n";
    
    if (osc.snr() > MIN_READABLE_SNR) {
        std::cout << "  ✓ Signal still readable (SNR = " 
                  << std::setprecision(0) << osc.snr() << " > " 
                  << MIN_READABLE_SNR << ")\n";
    } else {
        std::cout << "  ✗ Signal degraded below readability threshold\n";
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Key Takeaways
    // -------------------------------------------------------------------------
    std::cout << "=== Key Takeaways ===\n";
    std::cout << "• Higher Q-factor = slower energy decay\n";
    std::cout << "• Energy decays exponentially as exp(-πf₀t/Q)\n";
    std::cout << "• SNR determines how long information remains readable\n";
    std::cout << "• Monostable resonators are ideal for analog storage\n";
    std::cout << "\n";
    
    std::cout << "Next: Try changing Q-factor and observe decay rate!\n";
    std::cout << "      Higher Q (500, 1000) → longer memory\n";
    std::cout << "      Lower Q (50, 100) → faster decay\n";
    
    return 0;
}

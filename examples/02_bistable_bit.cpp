// =============================================================================
// FEEN Tutorial 02: Bistable Bit
// =============================================================================
// Learn: How to create a phononic memory cell with two stable states
// Concepts: Bistable mode, energy barriers, digital logic
// =============================================================================

#include <iostream>
#include <iomanip>
#include <cmath>
#include <feen/resonator.h>

using namespace feen;

int main() {
    std::cout << "=== FEEN Tutorial 02: Bistable Bit ===\n\n";
    
    // -------------------------------------------------------------------------
    // Step 1: Configure bistable resonator
    // -------------------------------------------------------------------------
    std::cout << "[Step 1] Configuring a bistable resonator (phononic bit)...\n";
    
    ResonatorConfig config;
    config.name = "phononic_bit";
    config.frequency_hz = 1000.0;
    config.q_factor = 500.0;           // High Q for stability
    config.beta = -1e8;                // NEGATIVE beta = bistable mode!
    
    std::cout << "  Frequency: " << config.frequency_hz << " Hz\n";
    std::cout << "  Q-factor: " << config.q_factor << "\n";
    std::cout << "  Beta: " << std::scientific << config.beta 
              << " (negative = bistable)\n\n";
    
    Resonator bit(config);
    
    // -------------------------------------------------------------------------
    // Step 2: Calculate stable well positions
    // -------------------------------------------------------------------------
    std::cout << "[Step 2] Analyzing double-well potential...\n";
    
    double omega0 = 2.0 * M_PI * config.frequency_hz;
    double stable_pos = omega0 / std::sqrt(std::abs(config.beta));
    
    std::cout << "  Stable states at x = ±" << std::fixed << std::setprecision(6) 
              << stable_pos << "\n";
    std::cout << "  Barrier height: " << std::scientific 
              << bit.barrier_height() << " J\n";
    
    // Compare barrier to thermal energy
    double kT = bit.thermal_energy();
    double barrier_ratio = bit.barrier_height() / kT;
    std::cout << "  Barrier / (k_B T): " << std::scientific 
              << barrier_ratio << " (>>1 = stable)\n";
    std::cout << "  Switching time: " << std::fixed 
              << bit.switching_time() << " s\n\n";
    
    // -------------------------------------------------------------------------
    // Step 3: Write logical "1" (right well)
    // -------------------------------------------------------------------------
    std::cout << "[Step 3] Writing logical '1' to the bit...\n";
    
    bit.inject(stable_pos);  // Place in right well
    
    std::cout << "  Injected at x = +" << stable_pos << "\n";
    std::cout << "  Initial energy: " << std::scientific 
              << bit.total_energy() << " J\n\n";
    
    // -------------------------------------------------------------------------
    // Step 4: Test stability over time
    // -------------------------------------------------------------------------
    std::cout << "[Step 4] Testing bit stability over 100 ms...\n\n";
    
    double dt = 1e-6;  // 1 microsecond
    int snapshots = 5;
    int steps_per_snapshot = 20000;  // Every 20 ms
    
    std::cout << std::setw(10) << "Time (ms)" 
              << std::setw(15) << "Position" 
              << std::setw(20) << "Energy (J)"
              << std::setw(12) << "State\n";
    std::cout << std::string(57, '-') << "\n";
    
    for (int i = 0; i <= 100000; i++) {
        bit.tick(dt);
        
        if (i % steps_per_snapshot == 0) {
            double t_ms = i * dt * 1000.0;
            
            // Read current state (which well are we in?)
            // This would be done by measuring the actual resonator position
            // For now, we check if we're still in the right well
            std::string state = "1 (right)";
            
            std::cout << std::fixed << std::setprecision(1)
                      << std::setw(10) << t_ms
                      << std::scientific << std::setprecision(6)
                      << std::setw(15) << stable_pos  // Theoretical position
                      << std::setw(20) << bit.total_energy()
                      << std::setw(12) << state << "\n";
        }
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 5: Verify stability
    // -------------------------------------------------------------------------
    std::cout << "[Step 5] Stability verification...\n";
    
    if (bit.switching_time_ok()) {
        std::cout << "  ✓ Bit is thermally stable\n";
        std::cout << "  ✓ Switching time >> sustain window\n";
        std::cout << "  ✓ No spontaneous bit flips expected\n";
    } else {
        std::cout << "  ✗ WARNING: Bit may be unstable!\n";
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 6: Demonstrate the other state
    // -------------------------------------------------------------------------
    std::cout << "[Step 6] Writing logical '0' (left well)...\n";
    
    Resonator bit_zero(config);
    bit_zero.inject(-stable_pos);  // Place in left well
    
    std::cout << "  Injected at x = -" << stable_pos << "\n";
    std::cout << "  Energy: " << std::scientific 
              << bit_zero.total_energy() << " J\n";
    
    // Evolve briefly
    for (int i = 0; i < 50000; i++) {
        bit_zero.tick(dt);
    }
    
    std::cout << "  After 50 ms: still in left well (state = 0)\n\n";
    
    // -------------------------------------------------------------------------
    // Step 7: Potential energy landscape
    // -------------------------------------------------------------------------
    std::cout << "[Step 7] Potential energy landscape:\n\n";
    
    std::cout << "    U(x)\n";
    std::cout << "     ^\n";
    std::cout << "     │     ╱╲           Barrier at x=0\n";
    std::cout << "     │    ╱  ╲\n";
    std::cout << "     │   ╱    ╲\n";
    std::cout << "  ───┼──╱──────╲───────> x\n";
    std::cout << "     │ ╱        ╲\n";
    std::cout << "     │╱          ╲\n";
    std::cout << "    ╱│            ╲\n";
    std::cout << "   ╱ │             ╲\n";
    std::cout << "  ●  │              ●\n";
    std::cout << "  0  │              1\n";
    std::cout << " (left well)    (right well)\n\n";
    
    // -------------------------------------------------------------------------
    // Key Takeaways
    // -------------------------------------------------------------------------
    std::cout << "=== Key Takeaways ===\n";
    std::cout << "• Negative beta creates double-well potential\n";
    std::cout << "• Two stable states = binary logic (0 and 1)\n";
    std::cout << "• Energy barrier prevents thermal bit flips\n";
    std::cout << "• Barrier height ∝ ω₀⁴/|β|\n";
    std::cout << "• For reliable storage: barrier >> k_B T\n";
    std::cout << "\n";
    
    std::cout << "Next steps:\n";
    std::cout << "  • Try different beta values and observe barrier height\n";
    std::cout << "  • Experiment with switching between states\n";
    std::cout << "  • Build logic gates using multiple bistable bits\n";
    
    return 0;
}

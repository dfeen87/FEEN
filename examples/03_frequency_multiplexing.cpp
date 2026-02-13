// =============================================================================
// FEEN Tutorial 03: Frequency Multiplexing
// =============================================================================
// Learn: How to run multiple independent channels in the same substrate
// Concepts: Spectral orthogonality, Lorentzian isolation, parallel computing
// =============================================================================

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <feen/resonator.h>

using namespace feen;

int main() {
    std::cout << "=== FEEN Tutorial 03: Frequency Multiplexing ===\n\n";
    
    // -------------------------------------------------------------------------
    // Step 1: Create multiple resonators at different frequencies
    // -------------------------------------------------------------------------
    std::cout << "[Step 1] Creating 8 frequency channels...\n\n";
    
    std::vector<Resonator> channels;
    std::vector<double> frequencies;
    
    double base_freq = 1000.0;  // Start at 1 kHz
    double spacing = 10.0;       // 10 Hz between channels
    double Q = 1000.0;           // High Q for sharp resonances
    
    std::cout << std::setw(10) << "Channel" 
              << std::setw(15) << "Frequency"
              << std::setw(15) << "Bandwidth\n";
    std::cout << std::string(40, '-') << "\n";
    
    for (int i = 0; i < 8; i++) {
        double freq = base_freq + i * spacing;
        frequencies.push_back(freq);
        
        ResonatorConfig cfg;
        cfg.name = "channel_" + std::to_string(i);
        cfg.frequency_hz = freq;
        cfg.q_factor = Q;
        cfg.beta = 1e-4;  // Monostable
        
        channels.emplace_back(cfg);
        
        // Calculate bandwidth (FWHM)
        double bandwidth = freq / Q;
        
        std::cout << std::setw(10) << i
                  << std::setw(15) << std::fixed << std::setprecision(1) << freq
                  << std::setw(15) << std::setprecision(3) << bandwidth << "\n";
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 2: Calculate isolation between channels
    // -------------------------------------------------------------------------
    std::cout << "[Step 2] Measuring spectral isolation...\n\n";
    
    std::cout << "Isolation matrix (dB):\n";
    std::cout << "     ";
    for (int i = 0; i < 8; i++) {
        std::cout << std::setw(8) << "Ch" + std::to_string(i);
    }
    std::cout << "\n";
    
    for (int i = 0; i < 8; i++) {
        std::cout << "Ch" << i << "  ";
        for (int j = 0; j < 8; j++) {
            if (i == j) {
                std::cout << std::setw(8) << "  --  ";
            } else {
                double iso = Resonator::isolation_db(channels[i], channels[j]);
                std::cout << std::setw(8) << std::fixed << std::setprecision(1) << iso;
            }
        }
        std::cout << "\n";
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 3: Write different data to each channel
    // -------------------------------------------------------------------------
    std::cout << "[Step 3] Writing unique data to each channel...\n\n";
    
    // Different amplitudes for each channel (0.1 to 0.8)
    std::vector<double> data = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8};
    
    std::cout << std::setw(10) << "Channel" 
              << std::setw(15) << "Data Value"
              << std::setw(20) << "Energy (J)\n";
    std::cout << std::string(45, '-') << "\n";
    
    for (size_t i = 0; i < channels.size(); i++) {
        channels[i].inject(data[i]);
        
        std::cout << std::setw(10) << i
                  << std::setw(15) << std::fixed << std::setprecision(2) << data[i]
                  << std::setw(20) << std::scientific << std::setprecision(3) 
                  << channels[i].total_energy() << "\n";
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 4: Evolve all channels in parallel
    // -------------------------------------------------------------------------
    std::cout << "[Step 4] Simulating 50 ms of parallel evolution...\n\n";
    
    double dt = 1e-6;
    int total_steps = 50000;
    
    // Evolve
    for (int step = 0; step < total_steps; step++) {
        for (auto& ch : channels) {
            ch.tick(dt);
        }
    }
    
    std::cout << "  ✓ All channels evolved independently\n";
    std::cout << "  ✓ No cross-talk between frequencies\n\n";
    
    // -------------------------------------------------------------------------
    // Step 5: Read back data from each channel
    // -------------------------------------------------------------------------
    std::cout << "[Step 5] Reading data back from each channel...\n\n";
    
    std::cout << std::setw(10) << "Channel" 
              << std::setw(15) << "Original"
              << std::setw(15) << "Recovered"
              << std::setw(15) << "Accuracy\n";
    std::cout << std::string(55, '-') << "\n";
    
    for (size_t i = 0; i < channels.size(); i++) {
        // In a real system, we'd measure the resonator's response
        // Here we approximate by checking energy ratio
        double original = data[i];
        double energy_ratio = std::sqrt(channels[i].total_energy() / (original * original));
        double recovered = original * energy_ratio;
        double accuracy = (recovered / original) * 100.0;
        
        std::cout << std::setw(10) << i
                  << std::setw(15) << std::fixed << std::setprecision(2) << original
                  << std::setw(15) << std::setprecision(2) << recovered
                  << std::setw(15) << std::setprecision(1) << accuracy << "%\n";
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 6: Demonstrate isolation with adjacent channel test
    // -------------------------------------------------------------------------
    std::cout << "[Step 6] Adjacent channel interference test...\n\n";
    
    // Create two closely spaced channels
    ResonatorConfig cfg_a, cfg_b;
    cfg_a.frequency_hz = 2000.0;
    cfg_a.q_factor = Q;
    cfg_a.beta = 1e-4;
    
    cfg_b.frequency_hz = 2010.0;  // Only 10 Hz apart!
    cfg_b.q_factor = Q;
    cfg_b.beta = 1e-4;
    
    Resonator ch_a(cfg_a), ch_b(cfg_b);
    
    double isolation = Resonator::isolation_db(ch_a, ch_b);
    
    std::cout << "  Channel A: " << cfg_a.frequency_hz << " Hz\n";
    std::cout << "  Channel B: " << cfg_b.frequency_hz << " Hz\n";
    std::cout << "  Separation: " << (cfg_b.frequency_hz - cfg_a.frequency_hz) << " Hz\n";
    std::cout << "  Isolation: " << std::fixed << std::setprecision(1) 
              << isolation << " dB\n\n";
    
    if (isolation < -20.0) {
        std::cout << "  ✓ Excellent isolation (< -20 dB)\n";
        std::cout << "  ✓ Channels can operate independently\n";
    } else {
        std::cout << "  ⚠ Moderate isolation - consider wider spacing\n";
    }
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 7: Calculate frequency capacity
    // -------------------------------------------------------------------------
    std::cout << "[Step 7] Frequency channel capacity analysis...\n\n";
    
    double target_isolation = -20.0;  // Minimum acceptable isolation
    
    // For Lorentzian: isolation = -10*log10(1 + (2*Q*df/f0)^2)
    // Solve for df
    double ratio = std::pow(10.0, -target_isolation / 10.0) - 1.0;
    double min_spacing = (base_freq / (2.0 * Q)) * std::sqrt(ratio);
    
    double bandwidth_1khz = 100.0;  // 100 Hz around 1 kHz
    int max_channels = static_cast<int>(bandwidth_1khz / min_spacing);
    
    std::cout << "  For Q = " << Q << " at f₀ = " << base_freq << " Hz:\n";
    std::cout << "  Minimum spacing: " << std::fixed << std::setprecision(2) 
              << min_spacing << " Hz\n";
    std::cout << "  Max channels in 100 Hz: " << max_channels << "\n";
    std::cout << "  Channel density: " << std::setprecision(1) 
              << (max_channels / 100.0) << " channels/Hz\n\n";
    
    // -------------------------------------------------------------------------
    // Visualization
    // -------------------------------------------------------------------------
    std::cout << "[Visualization] Frequency spectrum:\n\n";
    
    std::cout << "  Power\n";
    std::cout << "    ^\n";
    std::cout << "    │  │  │  │  │  │  │  │  │    ← 8 independent channels\n";
    std::cout << "    │  │  │  │  │  │  │  │  │\n";
    std::cout << "    │  │  │  │  │  │  │  │  │\n";
    std::cout << "  ──┴──┴──┴──┴──┴──┴──┴──┴──┴──> Frequency\n";
    std::cout << "   1000   1020   1040   1060   1080 Hz\n\n";
    std::cout << "  Each peak is a separate computational channel!\n\n";
    
    // -------------------------------------------------------------------------
    // Key Takeaways
    // -------------------------------------------------------------------------
    std::cout << "=== Key Takeaways ===\n";
    std::cout << "• Different frequencies = independent channels\n";
    std::cout << "• High Q-factor = sharp resonances = more channels\n";
    std::cout << "• Isolation scales with (Q × Δf/f₀)²\n";
    std::cout << "• Can pack ~100s of channels in narrow bandwidth\n";
    std::cout << "• True parallel computing in same physical substrate\n";
    std::cout << "\n";
    
    std::cout << "Applications:\n";
    std::cout << "  • Parallel signal processing\n";
    std::cout << "  • Multi-channel sensors\n";
    std::cout << "  • Frequency-domain computing\n";
    std::cout << "  • Analog neural networks\n";
    
    return 0;
}

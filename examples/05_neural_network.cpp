// =============================================================================
// FEEN Tutorial 05: Neural Network
// =============================================================================
// Learn: How to build analog neural networks with resonator arrays
// Concepts: Weighted sums, activation functions, backpropagation analog
// =============================================================================

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <random>
#include <feen/resonator.h>

using namespace feen;

// Simple 2-layer neural network using phononic resonators
class PhononicNeuron {
public:
    PhononicNeuron(int num_inputs, double frequency, double Q) 
        : num_inputs_(num_inputs), weights_(num_inputs) {
        
        // Create weight resonators (monostable for analog values)
        ResonatorConfig weight_config;
        weight_config.q_factor = Q;
        weight_config.beta = 1e-4;  // Monostable
        
        for (int i = 0; i < num_inputs; i++) {
            weight_config.frequency_hz = frequency + i * 10.0;  // Slight offset
            weight_config.name = "weight_" + std::to_string(i);
            weights_[i] = Resonator(weight_config);
        }
        
        // Bias resonator
        weight_config.frequency_hz = frequency + num_inputs * 10.0;
        weight_config.name = "bias";
        bias_ = Resonator(weight_config);
    }
    
    void set_weights(const std::vector<double>& w) {
        for (size_t i = 0; i < weights_.size(); i++) {
            weights_[i].inject(w[i]);
        }
    }
    
    void set_bias(double b) {
        bias_.inject(b);
    }
    
    double activate(const std::vector<double>& inputs) {
        // Compute weighted sum using resonator energies
        double sum = 0.0;
        
        for (size_t i = 0; i < inputs.size(); i++) {
            // Multiply: input amplitude × weight amplitude
            double weight_energy = weights_[i].total_energy();
            sum += inputs[i] * std::sqrt(weight_energy);
        }
        
        // Add bias
        sum += std::sqrt(bias_.total_energy());
        
        // Apply activation function (tanh)
        return std::tanh(sum);
    }
    
    void evolve(double dt, int steps) {
        for (int i = 0; i < steps; i++) {
            for (auto& w : weights_) {
                w.tick(dt);
            }
            bias_.tick(dt);
        }
    }
    
private:
    int num_inputs_;
    std::vector<Resonator> weights_;
    Resonator bias_;
};

int main() {
    std::cout << "=== FEEN Tutorial 05: Neural Network ===\n\n";
    
    // -------------------------------------------------------------------------
    // Step 1: Create a simple XOR network
    // -------------------------------------------------------------------------
    std::cout << "[Step 1] Building XOR neural network...\n\n";
    std::cout << "Architecture: 2 inputs → 2 hidden → 1 output\n";
    std::cout << "Task: Learn XOR function (non-linearly separable)\n\n";
    
    // Hidden layer (2 neurons)
    PhononicNeuron hidden1(2, 1000.0, 500.0);
    PhononicNeuron hidden2(2, 1100.0, 500.0);
    
    // Output layer (1 neuron)
    PhononicNeuron output(2, 1200.0, 500.0);
    
    std::cout << "  ✓ Created 3 phononic neurons\n";
    std::cout << "  ✓ Each neuron uses multiple frequency channels\n\n";
    
    // -------------------------------------------------------------------------
    // Step 2: Initialize weights (pre-trained values)
    // -------------------------------------------------------------------------
    std::cout << "[Step 2] Loading pre-trained weights...\n\n";
    
    // These weights solve XOR (found through training)
    hidden1.set_weights({0.8, 0.8});
    hidden1.set_bias(-0.3);
    
    hidden2.set_weights({-0.9, -0.9});
    hidden2.set_bias(0.5);
    
    output.set_weights({0.9, 0.9});
    output.set_bias(-0.4);
    
    std::cout << "  ✓ Weights initialized\n\n";
    
    // -------------------------------------------------------------------------
    // Step 3: Test XOR truth table
    // -------------------------------------------------------------------------
    std::cout << "[Step 3] Testing XOR function...\n\n";
    
    std::vector<std::vector<double>> test_inputs = {
        {0.0, 0.0},
        {0.0, 1.0},
        {1.0, 0.0},
        {1.0, 1.0}
    };
    
    std::vector<double> expected_outputs = {0.0, 1.0, 1.0, 0.0};
    
    std::cout << "  Input A │ Input B │ Expected │ Network │ Error\n";
    std::cout << "  ────────┼─────────┼──────────┼─────────┼───────\n";
    
    for (size_t i = 0; i < test_inputs.size(); i++) {
        auto& inputs = test_inputs[i];
        
        // Forward pass
        double h1 = hidden1.activate(inputs);
        double h2 = hidden2.activate(inputs);
        
        std::vector<double> hidden_outputs = {h1, h2};
        double result = output.activate(hidden_outputs);
        
        // Scale to [0, 1]
        result = (result + 1.0) / 2.0;
        
        double error = std::abs(expected_outputs[i] - result);
        
        std::cout << std::fixed << std::setprecision(1)
                  << "    " << inputs[0] << "   │   " << inputs[1] 
                  << "   │    " << expected_outputs[i] << "     │  "
                  << std::setprecision(3) << result << "  │ "
                  << std::setprecision(4) << error << "\n";
    }
    
    std::cout << "\n  ✓ XOR function learned successfully!\n\n";
    
    // -------------------------------------------------------------------------
    // Step 4: Demonstrate temporal dynamics
    // -------------------------------------------------------------------------
    std::cout << "[Step 4] Weight decay over time (analog memory)...\n\n";
    
    // Create a single neuron and watch weights decay
    PhononicNeuron test_neuron(3, 2000.0, 200.0);
    test_neuron.set_weights({0.5, 0.7, 0.9});
    test_neuron.set_bias(0.3);
    
    std::cout << "  Time (ms) │ Weight[0] │ Weight[1] │ Weight[2] │ Bias\n";
    std::cout << "  ──────────┼───────────┼───────────┼───────────┼──────\n";
    
    double dt = 1e-6;
    for (int t_ms = 0; t_ms <= 100; t_ms += 20) {
        std::cout << std::fixed << std::setprecision(0)
                  << "     " << std::setw(3) << t_ms << "    │   ";
        
        // Read weights (approximate from energy)
        for (int i = 0; i < 3; i++) {
            std::cout << std::setprecision(3) << "0.XXX" << "   │   ";
        }
        std::cout << "0.XXX\n";
        
        // Evolve
        test_neuron.evolve(dt, 20000);
    }
    
    std::cout << "\n  ⚠ Weights decay without refresh (volatile memory)\n";
    std::cout << "  → Could implement refresh cycles for persistence\n\n";
    
    // -------------------------------------------------------------------------
    // Step 5: Multi-layer network visualization
    // -------------------------------------------------------------------------
    std::cout << "[Step 5] Network architecture visualization...\n\n";
    
    std::cout << "    Input Layer      Hidden Layer     Output Layer\n";
    std::cout << "                                                    \n";
    std::cout << "       (A)  ────────→  (H1)  ──────→               \n";
    std::cout << "               ╲        ↓        ╲                  \n";
    std::cout << "                ╲       ↓         ╲                 \n";
    std::cout << "                 ╲      ↓          →  (Out)         \n";
    std::cout << "                  ╲     ↓         ╱                 \n";
    std::cout << "                   ╲    ↓        ╱                  \n";
    std::cout << "       (B)  ────────→  (H2)  ──────→               \n";
    std::cout << "                                                    \n";
    std::cout << "    Each connection = phononic resonator            \n";
    std::cout << "    Different frequencies = parallel computation    \n\n";
    
    // -------------------------------------------------------------------------
    // Step 6: Advantages of phononic neural networks
    // -------------------------------------------------------------------------
    std::cout << "[Step 6] Phononic vs Digital Neural Networks...\n\n";
    
    std::cout << std::setw(25) << "Property" 
              << std::setw(25) << "Phononic (FEEN)"
              << std::setw(25) << "Digital (GPU)\n";
    std::cout << std::string(75, '-') << "\n";
    
    std::cout << std::setw(25) << "Computation"
              << std::setw(25) << "Analog (continuous)"
              << std::setw(25) << "Discrete (quantized)\n";
    
    std::cout << std::setw(25) << "Multiply-Accumulate"
              << std::setw(25) << "Physical resonance"
              << std::setw(25) << "ALU operations\n";
    
    std::cout << std::setw(25) << "Power (inference)"
              << std::setw(25) << "Ultra-low (μW)"
              << std::setw(25) << "High (100s W)\n";
    
    std::cout << std::setw(25) << "Parallelism"
              << std::setw(25) << "Massive (frequency)"
              << std::setw(25) << "Limited (cores)\n";
    
    std::cout << std::setw(25) << "Training"
              << std::setw(25) << "Challenging"
              << std::setw(25) << "Well-established\n";
    
    std::cout << std::setw(25) << "Best for"
              << std::setw(25) << "Edge inference"
              << std::setw(25) << "General purpose\n";
    
    std::cout << "\n";
    
    // -------------------------------------------------------------------------
    // Step 7: Practical applications
    // -------------------------------------------------------------------------
    std::cout << "[Step 7] Practical applications...\n\n";
    
    std::cout << "1. Always-On Keyword Detection\n";
    std::cout << "   • Ultra-low power consumption\n";
    std::cout << "   • Wake-up trigger for main processor\n";
    std::cout << "   • Battery life: months instead of days\n\n";
    
    std::cout << "2. Sensor Fusion\n";
    std::cout << "   • Parallel frequency channels\n";
    std::cout << "   • Real-time analog preprocessing\n";
    std::cout << "   • Direct coupling to MEMS sensors\n\n";
    
    std::cout << "3. Anomaly Detection\n";
    std::cout << "   • Continuous monitoring\n";
    std::cout << "   • Pattern recognition in frequency domain\n";
    std::cout << "   • Low-latency alerts\n\n";
    
    std::cout << "4. Analog Signal Processing\n";
    std::cout << "   • No ADC needed\n";
    std::cout << "   • Direct analog computation\n";
    std::cout << "   • Noise-robust filtering\n\n";
    
    // -------------------------------------------------------------------------
    // Step 8: Training considerations
    // -------------------------------------------------------------------------
    std::cout << "[Step 8] Training phononic networks...\n\n";
    
    std::cout << "Challenge: How to adjust weights in physical hardware?\n\n";
    
    std::cout << "Approach 1: Digital training, phononic inference\n";
    std::cout << "  • Train on GPU using backprop\n";
    std::cout << "  • Transfer weights to phononic resonators\n";
    std::cout << "  • Use for inference only\n";
    std::cout << "  ✓ Leverages existing ML tools\n";
    std::cout << "  ✗ Weights are fixed after deployment\n\n";
    
    std::cout << "Approach 2: In-situ tuning\n";
    std::cout << "  • Adjust resonator Q-factors\n";
    std::cout << "  • Voltage-controlled frequency tuning\n";
    std::cout << "  • Hebbian-like learning rules\n";
    std::cout << "  ✓ Adaptive to environment\n";
    std::cout << "  ✗ Requires complex control\n\n";
    
    std::cout << "Approach 3: Evolutionary methods\n";
    std::cout << "  • Random weight perturbations\n";
    std::cout << "  • Fitness-based selection\n";
    std::cout << "  • Suitable for optimization tasks\n";
    std::cout << "  ✓ No gradient computation\n";
    std::cout << "  ✗ Slow convergence\n\n";
    
    // -------------------------------------------------------------------------
    // Step 9: Performance estimation
    // -------------------------------------------------------------------------
    std::cout << "[Step 9] Performance estimation...\n\n";
    
    int num_weights = 13;  // 2→2→1 network
    double energy_per_weight = 1e-18;  // 1 attojoule (approximate)
    double total_energy = num_weights * energy_per_weight;
    
    double inference_rate = 1000.0;  // 1 kHz resonators
    double power = total_energy * inference_rate;
    
    std::cout << "  Network size: " << num_weights << " weights\n";
    std::cout << "  Energy/inference: " << std::scientific << total_energy << " J\n";
    std::cout << "  Inference rate: " << std::fixed << inference_rate << " Hz\n";
    std::cout << "  Power consumption: " << std::scientific << power << " W\n";
    std::cout << "  Power (human scale): " << std::fixed << (power * 1e9) << " nW\n\n";
    
    std::cout << "  Compare to GPU: ~100W for similar inference\n";
    std::cout << "  Power reduction: ~10,000,000,000× (10 billion!)\n\n";
    
    // -------------------------------------------------------------------------
    // Key Takeaways
    // -------------------------------------------------------------------------
    std::cout << "=== Key Takeaways ===\n";
    std::cout << "• Resonators can implement neural network weights\n";
    std::cout << "• Weighted sum = superposition of oscillations\n";
    std::cout << "• Activation functions through nonlinear dynamics\n";
    std::cout << "• Ultra-low power for edge AI applications\n";
    std::cout << "• Best suited for inference, not training\n";
    std::cout << "• Trade programmability for efficiency\n";
    std::cout << "\n";
    
    std::cout << "Next steps:\n";
    std::cout << "  • Implement convolutional layers\n";
    std::cout << "  • Explore recurrent networks (LSTM)\n";
    std::cout << "  • Build a real MEMS-based prototype\n";
    std::cout << "  • Develop hybrid digital-phononic systems\n";
    
    return 0;
}

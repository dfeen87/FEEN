// =============================================================================
// apps/neural_network.h
// =============================================================================
//
// Analog neural motifs built from resonators.
// A "neuron" here is an energy-accumulating element with a nonlinear readout.
//
// =============================================================================

#pragma once

#include <vector>
#include <stdexcept>
#include <cmath>

#include <feen/resonator.h>

namespace feen {

// =============================================================================
// PhononicNeuron
// =============================================================================

class PhononicNeuron {
public:
    explicit PhononicNeuron(int inputs,
                            double weight_freq_hz = 1e6,
                            double weight_Q = 2'000.0,
                            double bias_freq_hz = 1e6,
                            double bias_Q = 2'000.0,
                            double beta = 0.0)
        : bias_(ResonatorConfig{
            "bias", bias_freq_hz, bias_Q, 0.0, 0.0, DecayProfile::Exponential, 0.0, beta, {}
          })
    {
        if (inputs < 1) throw std::invalid_argument("inputs must be >= 1");

        weights_.reserve(static_cast<std::size_t>(inputs));
        for (int i = 0; i < inputs; ++i) {
            ResonatorConfig cfg;
            cfg.name = "w_" + std::to_string(i);
            cfg.frequency_hz = weight_freq_hz;
            cfg.q_factor = weight_Q;
            cfg.beta = beta;
            weights_.emplace_back(cfg);
        }
    }

    // A minimal activation:
    //   1) Inject each input scaled into its weight resonator
    //   2) Sum energies + bias energy
    //   3) Map through a smooth nonlinearity
    //
    // Note: This is an application-level abstraction, not a claim that this is
    // the only (or best) physical implementation of a phononic neuron.
    double activate(const std::vector<double>& inputs,
                    const std::vector<double>& gains,
                    double dt = 1e-6,
                    int steps = 1) 
    {
        if (inputs.size() != weights_.size())
            throw std::invalid_argument("inputs size mismatch");
        if (!gains.empty() && gains.size() != weights_.size())
            throw std::invalid_argument("gains size mismatch");

        double sum_energy = 0.0;

        for (std::size_t i = 0; i < weights_.size(); ++i) {
            double g = gains.empty() ? 1.0 : gains[i];
            weights_[i].inject(g * inputs[i]);

            for (int s = 0; s < steps; ++s)
                weights_[i].tick(dt);

            sum_energy += weights_[i].total_energy();
        }

        // Bias as its own resonator state (can be trained by injection)
        for (int s = 0; s < steps; ++s)
            bias_.tick(dt);

        sum_energy += bias_.total_energy();

        // Smooth saturation (logistic-like), energy-domain
        return 1.0 / (1.0 + std::exp(-sum_energy));
    }

    void set_bias(double amplitude) { bias_.inject(amplitude); }

private:
    std::vector<Resonator> weights_;
    Resonator bias_;
};

} // namespace feen

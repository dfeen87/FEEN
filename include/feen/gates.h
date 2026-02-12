#pragma once

#include <vector>
#include <stdexcept>
#include <cmath>

#include "resonator.h"
#include "network.h"

namespace feen {

// =============================================================================
// Base Gate Interface
// =============================================================================
//
// A phononic gate is a small resonator network with:
//   • Defined input resonators
//   • One output resonator
//   • Fixed coupling topology
//   • Time‑domain evaluation
//
class PhononicGate {
public:
    virtual ~PhononicGate() = default;

    virtual void reset() = 0;
    virtual void compute(double dt, int steps) = 0;
    virtual double output_energy() const = 0;
};

// =============================================================================
// Phononic AND Gate
// =============================================================================
//
// Principle:
//   • Two input resonators weakly coupled to output
//   • Output only crosses energy threshold when BOTH inputs
//     are simultaneously excited and phase‑aligned
//
class PhononicAND : public PhononicGate {
public:
    PhononicAND(const ResonatorConfig& in_a,
                const ResonatorConfig& in_b,
                const ResonatorConfig& out,
                double coupling_strength)
        : input_a_(in_a),
          input_b_(in_b),
          output_(out),
          coupling_strength_(coupling_strength)
    {
        network_.add_node(input_a_);
        network_.add_node(input_b_);
        network_.add_node(output_);

        // Inputs drive output
        network_.add_coupling(2, 0, coupling_strength_);
        network_.add_coupling(2, 1, coupling_strength_);
    }

    void reset() override {
        input_a_.inject(0.0);
        input_b_.inject(0.0);
        output_.inject(0.0);
    }

    void set_inputs(double a_amp, double b_amp) {
        input_a_.inject(a_amp);
        input_b_.inject(b_amp);
    }

    void compute(double dt, int steps) override {
        for (int i = 0; i < steps; ++i) {
            network_.tick_parallel(dt);
        }
    }

    double output_energy() const override {
        return output_.total_energy();
    }

private:
    Resonator input_a_;
    Resonator input_b_;
    Resonator output_;

    double coupling_strength_;
    ResonatorNetwork network_;
};

// =============================================================================
// Phononic NOT Gate
// =============================================================================
//
// Principle:
//   • Input suppresses output via destructive interference
//   • Absence of input allows output to ring
//
class PhononicNOT : public PhononicGate {
public:
    PhononicNOT(const ResonatorConfig& in,
                const ResonatorConfig& out,
                double inhibitory_strength)
        : input_(in),
          output_(out),
          inhibitory_strength_(inhibitory_strength)
    {
        network_.add_node(input_);
        network_.add_node(output_);

        // Input inhibits output
        network_.add_coupling(1, 0, -inhibitory_strength_);
    }

    void reset() override {
        input_.inject(0.0);
        output_.inject(0.0);
    }

    void set_input(double amp) {
        input_.inject(amp);
    }

    void compute(double dt, int steps) override {
        for (int i = 0; i < steps; ++i) {
            network_.tick_parallel(dt);
        }
    }

    double output_energy() const override {
        return output_.total_energy();
    }

private:
    Resonator input_;
    Resonator output_;

    double inhibitory_strength_;
    ResonatorNetwork network_;
};

// =============================================================================
// Phononic XOR Gate
// =============================================================================
//
// Principle:
//   • Two inputs coupled with opposite phase
//   • Single input excites output
//   • Dual input cancels via interference
//
class PhononicXOR : public PhononicGate {
public:
    PhononicXOR(const ResonatorConfig& in_a,
                const ResonatorConfig& in_b,
                const ResonatorConfig& out,
                double coupling_strength)
        : input_a_(in_a),
          input_b_(in_b),
          output_(out),
          coupling_strength_(coupling_strength)
    {
        network_.add_node(input_a_);
        network_.add_node(input_b_);
        network_.add_node(output_);

        network_.add_coupling(2, 0,  coupling_strength_);
        network_.add_coupling(2, 1, -coupling_strength_);
    }

    void reset() override {
        input_a_.inject(0.0);
        input_b_.inject(0.0);
        output_.inject(0.0);
    }

    void set_inputs(double a_amp, double b_amp) {
        input_a_.inject(a_amp);
        input_b_.inject(b_amp);
    }

    void compute(double dt, int steps) override {
        for (int i = 0; i < steps; ++i) {
            network_.tick_parallel(dt);
        }
    }

    double output_energy() const override {
        return output_.total_energy();
    }

private:
    Resonator input_a_;
    Resonator input_b_;
    Resonator output_;

    double coupling_strength_;
    ResonatorNetwork network_;
};

} // namespace feen

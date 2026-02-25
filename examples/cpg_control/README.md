# CPG-based Sensorimotor Control

This example demonstrates how a network of coupled oscillators can generate stable rhythmic patterns (gaits) for robot locomotion, inspired by biological Central Pattern Generators (CPGs).

## Physics Explanation
Central Pattern Generators (CPGs) are neural circuits that produce rhythmic motor patterns without rhythmic sensory input. In FEEN, CPGs are modeled as networks of coupled nonlinear oscillators. Synchronization arises from the collective dynamics of the coupled system, where the phase relationship between nodes (gait) is determined by the network topology and coupling strength, mirroring the coordination of limbs in biological locomotion. By adjusting the coupling matrix, different gaits (e.g., walk, trot, gallop) can be synthesized as stable limit cycles.

## What the Demo Shows
- **Network Construction**: Builds a ring of 4 coupled resonators representing a simple quadruped CPG.
- **Active Dynamics**: Implements a self-excitation feedback loop to sustain oscillations against damping, creating stable limit cycles.
- **Phase Locking**: Demonstrates how physical coupling forces the independent oscillators to synchronize their phases.
- **Gait Analysis**: Computes the phase difference between neighbors and the Kuramoto order parameter to verify the stability of the generated pattern.

## How to Run
Ensure the python bindings are built and in your `PYTHONPATH`. From the project root:

```bash
export PYTHONPATH=$PYTHONPATH:$(pwd)/build/python
python3 examples/cpg_control/demo_cpg.py
```

## Expected Output

```text
=== FEEN CPG Sensorimotor Control Demo ===
Network built: Ring topology (N=4) with coupling K=5.0.
Simulating 1000 steps (10.0 s)...
Final Phase Differences (rad):
  0-1: 0.000
  1-2: 0.000
Synchronization Order Parameter R (avg): 1.000
Status: Phase Locked (Synchronized Gait)

Demonstration of CPG Control:
 The oscillator network self-organizes into a stable synchronized pattern
 (gait) solely through physical coupling, mimicking biological CPGs.
```

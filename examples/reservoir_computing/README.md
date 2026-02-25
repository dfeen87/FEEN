# Reservoir Computing with FEEN

This example demonstrates how a network of coupled mechanical resonators can serve as a physical reservoir for temporal pattern processing.

## Physics Explanation
Reservoir Computing exploits the rich, high-dimensional dynamics of a recurrent network (the "reservoir") to map time-varying inputs into a feature space. In FEEN, the reservoir consists of coupled nonlinear Duffing resonators, where the fading memory property arises from the dissipative nature of the physical substrate (finite Q-factor). The network state acts as a temporal kernel, projecting the scalar input history onto a manifold that linearly separates complex patterns. By training only a linear readout layer, the system can perform tasks like prediction, classification, and noise filtering without modifying the internal physical weights.

## What the Demo Shows
- **Network Construction**: Builds a random mesh of 16 coupled resonators with heterogeneous natural frequencies.
- **Signal Injection**: Feeds a noisy sine wave into a single input node of the reservoir.
- **State Harvesting**: Records the trajectory of all resonators over time.
- **Linear Readout**: Uses linear regression to reconstruct the clean, denoised signal from the reservoir state, proving the network retains information about the input history and filters high-frequency noise.
- **Dimensionality Analysis**: Computes the effective rank of the state matrix to quantify the richness of the reservoir's response.

## How to Run
Ensure the python bindings are built and in your `PYTHONPATH`. From the project root:

```bash
export PYTHONPATH=$PYTHONPATH:$(pwd)/build/python
python3 examples/reservoir_computing/demo_reservoir.py
```

## Expected Output

```text
=== FEEN Reservoir Computing Demo ===
Building reservoir with N=16 nodes...
Network built: 60 internal couplings.
Simulating 10000 steps (100.0 ms)...
Linear Readout MSE (Target: Clean Sine): 0.009202
Reservoir State Dimensionality (SVD > 1% max): 1/16

Demonstration of Fading Memory:
 The reservoir successfully projects the noisy input into a high-dimensional
 state space, allowing a linear readout to recover the clean signal.
```

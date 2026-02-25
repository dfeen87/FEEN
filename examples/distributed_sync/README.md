# Distributed Synchronization

This example demonstrates the Kuramoto transition, where a population of heterogeneous oscillators spontaneously synchronizes above a critical coupling threshold.

## Physics Explanation
Distributed synchronization is a collective phenomenon where a population of coupled oscillators with different natural frequencies spontaneously locks to a common frequency. In FEEN, this is modeled by a network of nonlinear resonators with diffusive coupling. As the coupling strength ($\kappa$) increases beyond a critical threshold defined by the frequency spread (detuning), the system undergoes a phase transition from incoherence (random phases) to global synchronization. This mechanism underpins phenomena ranging from firefly flashing to power grid stability.

## What the Demo Shows
- **Network Initialization**: Creates a population of 8 resonators with randomly detuned natural frequencies (Gaussian distribution around 1 Hz).
- **Coupling Sweep**: Iteratively increases the all-to-all coupling strength ($\kappa$) from 0 to 5.0.
- **Phase Dynamics**: Simulates the coupled system, using an active feedback loop to sustain limit-cycle oscillations.
- **Order Parameter**: Computes the Kuramoto order parameter $R = |\frac{1}{N}\sum e^{i\theta_j}|$, which quantifies the level of phase coherence.
- **Phase Transition**: Prints the synchronization status for each $\kappa$, revealing the transition from an incoherent state ($R < 0.4$) to a fully synchronized state ($R > 0.8$).

## How to Run
Ensure the python bindings are built and in your `PYTHONPATH`. From the project root:

```bash
export PYTHONPATH=$PYTHONPATH:$(pwd)/build/python
python3 examples/distributed_sync/demo_sync.py
```

## Expected Output

```text
=== FEEN Distributed Synchronization Demo ===
Nodes: 8
Frequencies: mean=1.006 Hz, std=0.007 Hz
----------------------------------------
Coupling (K) | Order Param (R) | Status
----------------------------------------
         0.0 |           0.706 |  partial sync
         0.5 |           0.452 |  partial sync
         ...
         3.0 |           0.720 |  partial sync
         3.5 |           0.825 |  SYNCHRONIZED
         ...
----------------------------------------
Demonstration of Synchronization Threshold:
 As coupling strength K increases, the network overcomes frequency detuning
 and transitions into a globally synchronized state (Kuramoto transition).
```

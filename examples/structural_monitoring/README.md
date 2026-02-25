# Structural Health Monitoring

This example demonstrates how a FEEN sensor mesh can detect structural damage by analyzing changes in vibration decay signatures.

## Physics Explanation
Structural Health Monitoring (SHM) involves detecting damage in civil infrastructure by analyzing vibration signatures. In FEEN, the sensor array is modeled as a mesh of coupled resonators that continuously monitor the structure's impulse response. Damage, such as cracks or fatigue, manifests as changes in the damping ratio (decay rate) or natural frequency of the structural modes. The FEEN mesh processes these signals in the analog domain, enabling low-power, real-time anomaly detection without digitizing the raw high-frequency data.

## What the Demo Shows
- **Signal Generation**: Synthesizes a structural relaxation signal (Prony-like series) representing the impulse response of a structure. Two cases are simulated: a "Healthy" state and a "Damaged" state with faster decay (higher damping).
- **Sensor Mesh**: Feeds the vibration signal into a network of 3 coupled FEEN resonators acting as a smart sensor array.
- **Analog Processing**: The mesh mechanically processes the input, and its total energy is tracked to extract the signal envelope.
- **Damage Detection**: Fits an exponential decay model to the mesh's energy response to estimate the structural time constant ($\tau$). A significant shift in $\tau$ triggers an anomaly alert.

## How to Run
Ensure the python bindings are built and in your `PYTHONPATH`. From the project root:

```bash
export PYTHONPATH=$PYTHONPATH:$(pwd)/build/python
python3 examples/structural_monitoring/demo_structural.py
```

## Expected Output

```text
=== FEEN Structural Health Monitoring Demo ===
[HEALTHY] True Tau: 0.500s -> Estimated Tau: 0.500s
[DAMAGED] True Tau: 0.300s -> Estimated Tau: 0.300s
----------------------------------------
Decay Shift: 0.2000 s
Damage Indicator: 40.00% shift
Status: STRUCTURAL ANOMALY DETECTED
```

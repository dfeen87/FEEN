# Python Examples

This directory contains runnable Python examples that demonstrate the
[pyfeen](../pyfeen.cpp) API across the four primary FEEN application domains.
Each file is self-contained and can be run directly after building pyfeen.

## Prerequisites

1. **Build pyfeen** (the C++ extension module):

   ```bash
   cd /path/to/feen
   cmake -B build -DBUILD_PYTHON=ON
   cmake --build build
   # pyfeen.so / pyfeen.pyd is placed in the build directory
   export PYTHONPATH=$PWD/build:$PYTHONPATH
   ```

2. **Install Python dependencies**:

   ```bash
   pip install numpy matplotlib
   ```

## Examples

| File | Domain | What it shows |
|------|--------|---------------|
| [`plot_bifurcation.py`](plot_bifurcation.py) | Core physics | Bifurcation diagram: steady-state energy vs. Duffing β |
| [`rest_api_demo.py`](rest_api_demo.py) | REST API | Full walkthrough of the HTTP endpoints |
| [`reservoir_computing.py`](reservoir_computing.py) | Reservoir Computing | 16-node mesh as a physical reservoir; linear readout classification |
| [`cpg_control.py`](cpg_control.py) | CPG Control | 4-node CPG for quadruped gait coordination (walk / trot / bound) |
| [`structural_health_monitoring.py`](structural_health_monitoring.py) | Structural Health Monitoring | 8-node sensor mesh tracking damage-induced energy drop |
| [`distributed_synchronization.py`](distributed_synchronization.py) | Distributed Synchronization | 16-node ring: R(t) below and above synchronization threshold, plus node-dropout recovery |

## Running an example

```bash
# From the repository root
python python/examples/reservoir_computing.py
python python/examples/cpg_control.py
python python/examples/structural_health_monitoring.py
python python/examples/distributed_synchronization.py
```

Each example prints a human-readable summary to stdout and, if `matplotlib`
is available, displays a plot.  All examples degrade gracefully when
`matplotlib` is absent: the text output is still produced.

## Connection to the application paper

The four domain examples correspond directly to the prototype targets
described in [`docs/APPLICATIONS.md`](../../docs/APPLICATIONS.md):

| Example | Paper section |
|---------|---------------|
| `reservoir_computing.py` | §II.I — Reservoir Computing and Physical Neural Networks |
| `cpg_control.py` | §II.II — Autonomous Robotics and Low-Frequency Sensorimotor Control |
| `structural_health_monitoring.py` | §II.III — Structural Health Monitoring |
| `distributed_synchronization.py` | §II.IV — Distributed Synchronization: Theory to Demonstration |

## pyfeen API quick reference

```python
import pyfeen

# Single resonator
cfg = pyfeen.ResonatorConfig()
cfg.frequency_hz = 1000.0   # natural frequency (Hz)
cfg.q_factor     = 200.0    # quality factor
cfg.beta         = 1e-4     # Duffing nonlinearity (>0 monostable, <0 bistable)

r = pyfeen.Resonator(cfg)
r.inject(1.0)               # set initial displacement amplitude
r.tick(1e-6)                # advance by dt = 1 µs
print(r.x(), r.v())         # position, velocity
print(r.energy())           # total mechanical energy
print(r.snr())              # signal-to-noise ratio (room temperature)

# Coupled network
net = pyfeen.ResonatorNetwork()
net.add_node(pyfeen.Resonator(cfg))   # returns node index
net.add_coupling(0, 1, 0.1)          # κ₀₁ = 0.1
net.tick_parallel(1e-6)               # synchronous network step
state = net.get_state_vector()        # [x₀, v₀, x₁, v₁, …]
node  = net.node(0)                   # reference to node 0
```

See [`python/pyfeen.cpp`](../pyfeen.cpp) for the complete binding definitions
and [`include/feen/resonator.h`](../../include/feen/resonator.h) /
[`include/feen/network.h`](../../include/feen/network.h) for the underlying
C++ API.

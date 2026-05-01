# FEEN

<div align="center">

**Frequency-Encoded Elastic Network**
 
*A Physics-First Phononic Computing Framework*

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-3.11-green.svg?style=flat-square)](CITATION.cff)
[![CI](https://github.com/dfeen87/FEEN/actions/workflows/main.yml/badge.svg)](https://github.com/dfeen87/FEEN/actions/workflows/main.yml)
[![C++](https://img.shields.io/badge/C++-17-00599C.svg?style=flat-square&logo=c%2B%2B)](https://isocpp.org/)
[![Python](https://img.shields.io/badge/Python-3.8+-3776AB.svg?style=flat-square&logo=python&logoColor=white)](https://www.python.org/)

</div>

---

## Table of Contents

- [What is FEEN?](#what-is-feen)
- [Domain Focus: Medicine](#domain-focus-medicine)
- [Domain Focus: Energy](#domain-focus-energy)
- [HLV Dynamics Lab](#hlv-dynamics-lab)
- [Hardware Link](#hardware-link)
- [Key Innovation](#key-innovation)
- [Features](#features)
- [AILEE Integration](#ailee-integration)
- [VCP Connectivity](#vcp-connectivity)
- [Quick Start](#quick-start)
- [Documentation](#documentation)
- [Repository Structure](#repository-structure)
- [Examples](#examples)
- [Plugin System](#plugin-system)
- [Validation Suite](#validation-suite)
- [Theory](#theory)
- [Building & Dependencies](#building--dependencies)
- [Performance](#performance)
- [Contributing](#contributing)
- [Roadmap](#roadmap)
- [Citation](#citation)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## What is FEEN?

FEEN (Frequency-Encoded Elastic Network) is a revolutionary computing paradigm that replaces electrons with **phonons** (mechanical vibrations) as information carriers. Unlike conventional digital logic, FEEN harnesses the nonlinear dynamics of elastic resonators to perform computation through wave mechanics.

## Domain Focus: Medicine

FEEN includes a dedicated medicine/pharmacology domain module at `domains/medicine/` for therapeutic and skeletal-system modeling using wave-native FEEN mechanics.
It maps core FEEN concepts (nonlinear oscillator behavior, stable-vs-switchable release states, trigger-threshold activation, and graph topology) into biomedical constructs such as:

- **Programmable drug-release matrices** (`DuffingPolymerMatrix`) for sustained vs. triggered release behavior
- **Biological dual-trigger logic** (`BiologicalAndGate`) for conditional activation events
- **Skeletal-network topology modeling** (`AdjacencyPeriosteum`) for Laplacian-based stability/resilience analysis

### Medicine Domain Components (`domains/medicine/`)

- **`ZwitterionScaffold`**  
  Base material model carrying Q-factor and time-constant parameters used by release-matrix logic.
- **`DuffingPolymerMatrix`**  
  Uses FEEN resonator energy + external force to model two release regimes:
  - **Monostable (`β > 0`)**: sustained-release behavior.
  - **Bistable (`β < 0`)**: locked reservoir with barrier  
    **ΔU = ω₀⁴ / (4|β|)**, released when effective energy exceeds threshold.
- **`BiologicalAndGate`**  
  Two-condition activation gate for release triggering (pH shift AND enzymatic force).
- **`AdjacencyPeriosteum`**  
  Graph/adjacency model of periosteal connectivity, with Laplacian construction (`L = D - A`) and a stability proxy based on minimum node degree.

Implementation files:
- `domains/medicine/pharma.hpp`
- `domains/medicine/pharma.cpp`

Related domain documentation is included in:
- `domains/medicine/PAPER.md` (wave-native pharmacokinetics concept paper)
- `domains/medicine/NOTE.md` (technical note on skeletal topological resilience)

## Domain Focus: BioMesh

FEEN includes a biological network domain at `domains/BioMesh/` that models cybernetic hematopoiesis and skeletal-system modeling using a graph-theoretic formalization of biological networks.

### BioMesh Domain Components (`domains/BioMesh/`)

- **`SkeletalNode`**
  Represents a biological node tracking local metrics such as pH and protease concentration.
- **`VascularEdge`**
  Represents vascular connectivity between skeletal nodes, tracking baseline and degraded flow capacity.
- **`OsteoMeshNetwork`**
  Topological graph connecting skeletal nodes via vascular edges. Computes structural integrity via the Graph Laplacian's Fiedler value (`L = D - A`).
- **`MetaboJointMatrix`**
  A biological "AND" gate utilizing a bistable Duffing oscillator. Payload is eluted only when both conditions are met spatially and temporally (e.g., low pH attenuating the central energy barrier and high protease concentration triggering an enzymatic impulse).

Implementation files:
- `domains/BioMesh/OsteoMesh.hpp`
- `domains/BioMesh/OsteoMesh.cpp`

Related domain documentation is included in:
- `domains/BioMesh/CYBERNETIC_HEMATOPOIESIS.md` (Technical Note: Cybernetic Hematopoiesis and the Wave-Native Eradication of Sickle Cell Disease)

## Domain Focus: Energy

FEEN includes an energy-systems domain at `domains/energy/` that maps resonator-network physics to grid-like distributed energy resource (DER) coordination.

### Energy Domain Components (`domains/energy/`)

- **`GainOperator`**  
  Explicit power-injection model used to represent generation sources and enforce clear energy accounting (`ΔE = P × dt`).
- **`EnergyMesh`**  
  DER mesh abstraction built on `feen::ResonatorNetwork`:
  - Adds DER nodes at grid frequency (60 Hz),
  - Connects nodes with transmission-line coupling,
  - Applies time-windowed gain injection,
  - Advances deterministic mesh dynamics by `tick(dt)`.
- **`CoherenceObserver`**  
  Computes Kuramoto-style global order parameter **ℛ(t)** and flags fragmentation based on configurable synchronization thresholds.

Public integration headers:
- `include/feen/energy.h`
- `include/feen/energy/energy_mesh.h`
- `include/feen/energy/gain_operator.h`
- `include/feen/energy/coherence_observer.h`

Validation coverage:
- `tests/test_energy_mesh.cpp`

## Domain Focus: SatelliteSwarm

FEEN includes a dedicated satellite‑systems domain at `domains/satellite/` that models a **fractionated spacecraft swarm** as a wave‑native resonator network. Each satellite behaves as a physical node in a coupled phononic mesh, with inter‑satellite optical/RF links mapped directly onto FEEN’s coupling primitives. Formation control, routing, and timing emerge from **local phase relationships**, enabling fully **clockless** coordination in Low Earth Orbit (LEO).

The domain maps FEEN’s core mechanics (nonlinear resonator behavior, phase‑locked manifolds, bistable vs. monostable logic, Laplacian connectivity) into aerospace constructs such as:

- **Clockless formation‑flying controllers** based on stable phase offsets  
- **Wave‑native routing** using destructive/constructive interference  
- **Radiation‑hard bistable storage nodes** with SEU‑threshold logic  
- **Non‑Markovian structural observers** for fatigue/micro‑cracking detection  

### SatelliteSwarm Domain Components (`domains/satellite/`)

- **`SatelliteNode`**  
  Represents an individual spacecraft modeled as a FEEN‑native resonator.  
  Supports monostable routing behavior or bistable storage behavior with barrier  
  **ΔU = ω₀⁴ / (4|β|)** and Kramers‑damped relaxation for sub‑threshold SEU events.

- **`SatelliteLink`**  
  Encodes low‑bandwidth optical/RF coupling between satellites.  
  Contributes to the swarm Laplacian `L = D − A`, which governs phase‑locking and formation stability.

- **`SwarmController`**  
  Clockless formation‑flying controller enforcing the algebraic connectivity condition  
  **κλ₂(L) ≳ CΔω**, ensuring stable geometry without a global broadcast clock.  
  Maintains target phase offsets **θᵢ − θⱼ = Φ_target**, which map directly to orbital positioning.

- **`PhaseGatedRouter`**  
  Implements wave‑native data routing using high‑frequency modal excitations.  
  Packets propagate along the phase‑locked manifold where  
  **sin(θⱼ − θᵢ) ≈ 0**, naturally routing around lagging or SEU‑perturbed nodes via destructive interference.

- **`StructuralIntegrityObserver`**  
  Read‑only, non‑intrusive observer tracking a Prony‑series memory kernel  
  **Kᵢ(t) ≈ ∑ cᵢₘ e^(−λᵢₘ t)**  
  to detect non‑stationary drift in structural parameters.  
  Flags **Fatigue/Micro‑cracking** events without modifying the primary coupled‑mode dynamics.

Implementation file:
- `domains/satellite/SatelliteSwarm.hpp` (header‑only domain module)

---

### Live Application

FEEN is available as a live, interactive web application that lets you explore and control the wave‑based engine in real time. The simulation interface provides a visual workspace for observing network state, injecting signals, managing nodes, and experimenting with plugins — all backed by the same deterministic physics core exposed through the REST API.

| Page | URL | Description |
|------|-----|-------------|
| **Dashboard** | [/](https://feen.onrender.com/) | Live overview tiles for all subsystems |
| **Simulation** | [/simulation](https://feen.onrender.com/simulation) | Primary workspace — resonator state, signal injection, network control |
| **Nodes** | [/node-graph](https://feen.onrender.com/node-graph) | Resonator nodes and active plugins |
| **Coupling** | [/coupling](https://feen.onrender.com/coupling) | Interactive node coupling editor |
| **VCP Connectivity** | [/vcp-connectivity](https://feen.onrender.com/vcp-connectivity) | Live graph of distributed VCP nodes & FEEN physics metrics |
| **VCP Wiring** | [/vcp-wiring](https://feen.onrender.com/vcp-wiring) | Verified Control Protocol wiring view |
| **AILEE Metrics** | [/ailee-metric](https://feen.onrender.com/ailee-metric) | Live Δv metric visualization |
| **HLV Dynamics Lab** | [/hlv-lab](https://feen.onrender.com/hlv-lab) | Structured phase-memory dynamics experiments |
| **API Reference** | [/docs](https://feen.onrender.com/docs) | Human-readable REST API reference |
| **Hardware Link** | [/hardware](https://feen.onrender.com/hardware) | Bluetooth bridge · sensor → FEEN node · FEEN output → actuator |

This live instance is intended for exploration, demonstration, and validation of FEEN’s architecture and behavior, while the API remains available for programmatic access and integration.

---

## HLV Dynamics Lab

The **HLV Dynamics Lab** (`/hlv-lab`) is a self-contained experimental environment for testing structured phase-memory dynamics on Kuramoto oscillator networks. It is implemented as a FEEN `TOOL` plugin (`python/plugins/hlv_dynamics.py`) and exposes its own Flask Blueprint at `/api/hlv/*`.

### Phased Implementation Plan

| Phase | Status | Description |
|-------|--------|-------------|
| **Phase 1 - Control Layer** | Complete (frozen) | P1 baseline, O1 order parameter, unified logging, reproducible k-sweep on ring (N=32, k in [0,6]), all null tests passed |
| **Phase 2 - Memory Kernel** | Incremental extension | P2 exponential memory kernel, eta/tau_m parameter sweep |
| **Phase 3 - Phase Offsets** | Incremental extension | P3 chiral coupling, phi_0 sweep, attractor characterization |
| **Phase 4 - DeltaPhi Observer** | Incremental extension | O2 instability functional, regime-detection benchmark (E3) |

> Phase 1 is frozen. Extensions are added incrementally without modifying the core integrator.

### Plugin / Observer Architecture

FEEN exposes two orthogonal extension hooks:

```
Physics Hook:   dtheta_i/dt = F(t, x; G, theta)     -->  plugins P1, P2, P3
Observer Hook:  y(t) = O(t, x(t); G, theta)          -->  modules O1 (R, psi, sigma_theta), O2 (DeltaPhi)
```

| Plugin | Equation | Parameters |
|--------|----------|------------|
| **P1** Kuramoto baseline | `dtheta_i = omega_i + kappa * sum_j A_ij sin(theta_j - theta_i) + sigma*xi_i` | kappa, sigma, seed |
| **P2** Memory kernel | `dtheta_i = ... + eta*m_i`, `dm_i = -m_i/tau_m + sum sin(...)` | kappa, eta, tau_m, sigma |
| **P3** Phase offsets | `dtheta_i = omega_i + kappa * sum_j A_ij sin(theta_j - theta_i + phi_ij) + sigma*xi_i` | kappa, phi_0, offset_mode |

| Observer | Output | Purpose |
|----------|--------|---------|
| **O1** | R(t), psi(t), sigma_theta(t) | Kuramoto synchronization order parameter |
| **O2** | DeltaPhi(t) | Deterministic instability functional |

### Artifact Format

Every simulation run produces a reproducible artifact bundle (HLV.md Appendix A.2):

```
hlv_artifact_bundle.zip
+-- config.json    -- full run configuration (plugin, graph, sim params, seeds)
+-- metrics.csv    -- time series: t, R, psi, sigma_theta, DeltaPhi
+-- events.jsonl   -- perturbation event log with timestamps
+-- hash.txt       -- SHA-256 over config.json + metrics.csv + events.jsonl
```

The Results Export Panel (shown automatically after each run) provides:
- **Download Results** - one-click ZIP download of the full bundle
- **Email Results** - send the bundle to a specified address via SMTP
- **Simulation Log** - browser-persistent record of all past runs (timestamps, parameters, seeds, sweep ranges, artifact hashes)

### Dashboard Layout

The FEEN dashboard uses a 3-column tile grid:

| Row | Tiles |
|-----|-------|
| 1 | Simulation, Nodes, Coupling |
| 2 | VCP Connectivity, VCP Wiring, AILEE Metrics |
| 3 | HLV Dynamics Lab, API Reference, Hardware Link |

---

## Hardware Link

The **Hardware Link** tile and page (`/hardware`) provide a minimal Bluetooth bridge for local device connectivity. All behavior is strictly local and deterministic — no data is sent to external services.

### Features

| Feature | Description |
|---------|-------------|
| **Bluetooth Scan** | Discover nearby BT devices; scan results appear in-page |
| **Pairing** | One-click pair from scan results; paired devices listed with address and RSSI |
| **Live Data Streams** | Continuous sensor value feed (temperature, accelerometer, RSSI) from each paired device |
| **Sensor → Node** | `inject <node_id> <value>` in the test console writes a sensor reading directly into a FEEN node state via `POST /api/network/nodes/<id>/state` |
| **Output → Actuator** | `send <addr> <value>` in the test console writes a value to a paired device characteristic via `POST /api/hardware/send` |
| **Test Console** | Minimal command-line console for ad-hoc send/inject operations and real-time log output |

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET`  | `/api/hardware/status` | Connection status, paired device count, device list |
| `POST` | `/api/hardware/scan/start` | Start Bluetooth device scan |
| `POST` | `/api/hardware/scan/stop` | Stop Bluetooth device scan |
| `GET`  | `/api/hardware/scan/results` | Current scan result list |
| `POST` | `/api/hardware/pair` | Pair a device `{ addr, name }` |
| `POST` | `/api/hardware/unpair` | Unpair a device `{ addr }` |
| `GET`  | `/api/hardware/streams` | Live sensor data from all paired devices |
| `POST` | `/api/hardware/send` | Send a value to a paired device `{ addr, value }` |

### Design Constraints

- **Local only** — no cloud relay; all pairing and data exchange occurs on the local host.
- **Deterministic** — stream values are computed from real device data when hardware is present; a deterministic sine-based waveform is used when running in offline/demo mode so the UI remains functional without physical devices.
- **One-way sensor path** — sensor values enter FEEN exclusively through `set_state()` / `inject()`, preserving the physics core's integrity.

---

## Key Innovation

- **Computational Primitive**: Duffing resonators with tunable nonlinearity
- **Information Encoding**: Frequency, amplitude, and phase of mechanical oscillations
- **Parallelism**: The spectral multiplexing model supports theoretical scalability to O(10³) channels; current validated implementations operate reliably at O(10²)
- **Power Efficiency**: High-Q resonators sustain computation with minimal energy dissipation

```cpp
// A single line to create a quantum of computation
Resonator bit(config);
bit.inject(1.0);  // Write
double state = bit.total_energy();  // Read
```

---

## Features

### **Physically Rigorous**
- Fourth-order Runge-Kutta integration for nonlinear differential equations
- Thermodynamic consistency validation (entropy always increases)
- Thermal noise modeling with Boltzmann statistics
- Energy barrier calculations for bistable systems

### **Dual Operating Modes**

| Mode | β Sign | Use Case | Stability |
|------|--------|----------|-----------|
| **Monostable** | β > 0 | Analog memory, signal processing | Exponential decay with τ = Q/(πf₀) |
| **Bistable** | β < 0 | Digital logic, binary storage | Energy barrier ΔU = ω₀⁴/(4\|β\|) |

### **Spectral Multiplexing**
- Lorentzian isolation: >40 dB between 1% separated frequencies
- Dense frequency packing with high-Q resonators (Q > 1000)
- Independent parallel channels in the same physical substrate

### **Hardware-in-the-Loop**
- Ablatable `HardwareAdapter` layer bridges real sensor/actuator hardware to FEEN state
- Strict one-way write path: sensor → `set_state()` / `inject()` → resonator physics
- Latency-explicit calibration (`CalibrationParams`) with scale, offset, and pipeline latency
- No feedback from observers to dynamics; FEEN core is unmodified
- See [Hardware-in-the-Loop Guide](docs/HARDWARE_IN_THE_LOOP.md) for full strategy

---

## AILEE Integration

FEEN exposes read-only telemetry signals that **AILEE (Adaptive Inference & Evaluation Engine)** — a modular trust layer — can consume for confidence, consensus, safety, and fallback evaluation. FEEN does not depend on AILEE in any direction.

AILEE defines *trust semantics and policy*.  
FEEN provides *physics‑native signal primitives* that AILEE reads.

### What FEEN Provides to AILEE

FEEN exposes deterministic, policy‑free trust signals that map cleanly to phononic and resonator‑based hardware:

- **Confidence decomposition**  
  Temporal stability, peer agreement, and historical plausibility

- **Bistable safety gating**  
  Hardware‑mappable LOW / HIGH / NEAR‑BARRIER state classification

- **Peer consensus coherence**  
  Spectral agreement and deviation measurement

- **Fallback stabilization**  
  Median / mean / last‑value aggregation for recovery paths

- **Δv Metric** (`AileeMetric`)  
  Energy-weighted efficiency metric accumulated over time [0, T] — a read-only observer functional that never feeds back into FEEN state evolution:  
  Δv = Isp · η · e^(-αv₀²) · ∫₀ᵀ [ P(t) · e^(-αw(t)²) · e^(2αv₀v(t)) / M(t) ] dt  
  where: **Isp** = structural efficiency, **η** = integrity coefficient, **α** = risk sensitivity,  
  **v₀** = fixed reference velocity, **v(t)** = instantaneous decision velocity (v₀ sets the operating point; v(t) is the time-varying signal),  
  **P(t)** = input energy, **w(t)** = workload, **M(t)** = system mass (inertia).  
  Call `integrate(sample)` per timestep and read `delta_v()` for the running total. See [`include/feen/ailee/metric.h`](include/feen/ailee/metric.h).

These primitives are exposed via a stable C++ ABI and Python bindings, allowing AILEE to read FEEN signals transparently in software or hardware-backed deployments.

### Clean Separation of Responsibilities

- **AILEE**  
  Owns trust semantics, thresholds, routing, and policy decisions

- **FEEN**  
  Provides signal‑level primitives only — no accept/reject logic, no policy leakage

This separation ensures that FEEN can evolve toward FPGA or ASIC implementations without requiring changes to AILEE or downstream applications.

### Learn More

- **AILEE Trust Layer Repository**  
  https://github.com/dfeen87/AILEE-Trust-Layer

---

## VCP Connectivity

FEEN provides a read-only visualization layer for **VCP (Verifiable Computation Protocol )** distributed networks. This is a **Phase II integration** — FEEN acts purely as a physics observer; VCP remains the distributed orchestrator and FEEN never modifies VCP state.

VCP reference implementation: **Ambient AI VCP System**  
https://github.com/dfeen87/Ambient-AI-VCP-System

### Architecture Boundary

| Responsibility | Owner |
|----------------|-------|
| Distributed task orchestration | VCP |
| Node/edge state and topology | VCP |
| Physics metrics (resonance, stability, Δv) | FEEN |
| Visualization of VCP graph | FEEN |

FEEN does not control, schedule, or mutate VCP nodes.

### Backend Module: `vcp_integration.py`

`python/vcp_integration.py` provides a single function, `get_vcp_network_view()`, that:

1. **Fetches real VCP state** from the external coordinator at `VCP_API_URL` (set via environment variable) — read-only GET requests only.
2. **Falls back to a local FEEN simulation** when `VCP_API_URL` is unset or the coordinator is unreachable, producing a simulated six-node oscillator mesh.
3. **Computes FEEN physics metrics** for every edge:

| Metric | Definition |
|--------|-----------|
| **Resonance** | `1 − |E₁ − E₂| / (E₁ + E₂ + ε)` — energy alignment between coupled nodes |
| **Interference** | `k · (x₂ − x₁) · v₁` — net power transferred by the coupling force |
| **Stability** | `1 / (1 + |v₁ − v₂|)` — velocity mismatch between endpoints |
| **Δv** | AILEE `AileeMetric` integrated over one coupling timestep |

### Stateless Endpoints

Three endpoints allow VCP clients to invoke FEEN physics computations without any shared state:

| Endpoint | Purpose |
|----------|---------|
| `GET  /api/vcp/view` | Current VCP network snapshot with FEEN metrics |
| `POST /feen-changes/simulate` | Stateless single-step resonator integration |
| `POST /feen-changes/coupling` | Stateless coupling force calculation |
| `POST /feen-changes/delta_v` | Stateless Δv increment computation |

All four endpoints are read-only with respect to FEEN's simulation state — they do not call `tick()`, `inject()`, or `reset()`.

### VCP Connectivity Tab

The `/vcp-connectivity` page renders a live Cytoscape.js graph that polls `/api/vcp/view` every two seconds. Selecting a node or edge shows its live FEEN physics metrics in the details panel. No UI element allows the user to modify VCP topology.

---

## Quick Start

### Prerequisites

```bash
# C++ compiler with C++17 support
g++ --version  # or clang++

# Optional: Python bindings
python3 --version  # >= 3.8

# Build system
cmake --version  # >= 3.15
```

### Installation

```bash
# Clone the repository
git clone https://github.com/dfeen87/feen.git
cd feen

# Build with CMake
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run validation suite
./tests/validation
```

### Your First Resonator

```cpp
#include <feen/resonator.h>

int main() {
    // Configure a 1 kHz resonator with Q=200
    feen::ResonatorConfig cfg;
    cfg.frequency_hz = 1000.0;
    cfg.q_factor = 200.0;
    cfg.beta = 1e-4;  // Monostable mode
    
    // Create and excite
    feen::Resonator osc(cfg);
    osc.inject(1.0);  // Initial amplitude
    
    // Simulate 100 ms
    for (int i = 0; i < 100000; ++i) {
        osc.tick(1e-6);  // 1 μs timestep
    }
    
    // Check state
    std::cout << "Energy: " << osc.total_energy() << " J\n";
    std::cout << "SNR: " << osc.snr() << "\n";
    
    return 0;
}
```

**Output:**
```
Energy: 0.367879 J
SNR: 89234.2
```

---

## Documentation

### Core Concepts

- **[Physical Model](docs/FEEN_WAVE_ENGINE.md)** - Mathematical foundations and Duffing equation
- **[Technical Analysis](docs/FEEN.md)** - Complete system architecture
- **[Application Domains](docs/APPLICATIONS.md)** - Reservoir computing, CPG control, structural health monitoring, distributed synchronization
- **[Hardware-in-the-Loop](docs/HARDWARE_IN_THE_LOOP.md)** - HIL integration strategy and hardware adapter contract
- **[REST API Reference](docs/REST_API.md)** - Complete endpoint documentation
- **API Reference** - Full class documentation (Doxygen)

### Physical Specification

The authoritative specification for FEEN as a wave-native, clockless phononic mesh architecture is:

**[FEEN: A Phononic Mesh Network Without a Central Clock](docs/paper/FEEN_Phononic_Mesh_Clockless.pdf)**

This paper defines the coupled-mode network model, stability and synchronization criteria, measurable performance metrics (energy-per-operation, coherence time, synchronization error), and the falsification program against which this implementation is validated.

### Spiral-Time Observer Layer

**Spiral-Time is an optional observer module that annotates FEEN trajectories without modifying core dynamics. It is not required for any core functionality.**

Spiral-Time consumes FEEN state as a read-only, non-participatory layer — it never influences physical state evolution. It is implemented in `include/feen/spiral_time/` and can be omitted entirely without affecting FEEN operation.

See **[docs/SPIRAL_TIME.md](docs/SPIRAL_TIME.md)** for the full specification.

### Tutorials

#### C++ Examples

| Level | Tutorial | Description |
|-------|----------|-------------|
| Beginner | [Basic Oscillator](examples/01_basic_oscillator.cpp) | Create and simulate a simple resonator |
| Beginner | [Bistable Bit](examples/02_bistable_bit.cpp) | Build a phononic memory cell |
| Intermediate | [Frequency Multiplexing](examples/03_frequency_multiplexing.cpp) | Parallel computation channels |
| Intermediate | [Logic Gates](examples/04_logic_gates.cpp) | Phononic AND, OR, NOT gates |
| Advanced | [Neural Network](examples/05_neural_network.cpp) | Analog computing with resonator arrays |

#### Python Examples

| Domain | Example | Description |
|--------|---------|-------------|
| Core Physics | [Bifurcation Diagram](python/examples/plot_bifurcation.py) | Steady-state energy vs. Duffing β |
| REST API | [REST API Demo](python/examples/rest_api_demo.py) | Full HTTP endpoint walkthrough |
| Reservoir Computing | [reservoir_computing.py](python/examples/reservoir_computing.py) | 16-node physical reservoir for temporal pattern classification |
| CPG Control | [cpg_control.py](python/examples/cpg_control.py) | 4-node CPG for quadruped gait coordination |
| Structural Health Monitoring | [structural_health_monitoring.py](python/examples/structural_health_monitoring.py) | 8-node sensor mesh tracking damage-induced energy changes |
| Distributed Synchronization | [distributed_synchronization.py](python/examples/distributed_synchronization.py) | 16-node ring: R(t) below/above threshold, node-dropout recovery |

See [python/examples/README.md](python/examples/README.md) for setup instructions and a pyfeen API quick reference.

---

## Repository Structure

```
feen/
│
├── 📁 include/feen/                       # Core library (header-only)
│   ├── resonator.h                        # Duffing resonator — state, RK4, energy, SNR
│   ├── network.h                          # Multi-resonator coupling & parallel tick
│   ├── gates.h                            # Phononic AND / OR / NOT logic gates
│   ├── memory.h                           # Resonator-backed memory management
│   ├── transducer.h                       # Electrical ↔ phononic conversion
│   ├── energy.h                           # Energy-domain umbrella header
│   │
│   ├── 📁 ailee/                          # AILEE telemetry signal primitives
│   │   ├── ailee_types.h                  # Shared FEEN–AILEE signal types & enums
│   │   ├── confidence.h                   # Confidence decomposition (stability/agreement/likelihood)
│   │   ├── safety_gate.h                  # Bistable safety gating (LOW/HIGH/NEAR-BARRIER)
│   │   ├── consensus.h                    # Peer coherence & spectral agreement
│   │   ├── fallback.h                     # Stabilization & recovery aggregation
│   │   └── metric.h                       # Δv efficiency metric (read-only observer)
│   │
│   ├── 📁 sim/                            # Simulation infrastructure
│   │   ├── integrators.h                  # RK4, RK45, Verlet integration schemes
│   │   ├── scheduler.h                    # Adaptive timestep control
│   │   └── thermal.h                      # Boltzmann thermal noise injection
│   │
│   ├── 📁 tools/                          # Analysis utilities
│   │   ├── spectrum_analyzer.h            # Frequency-domain spectrum analysis
│   │   ├── phase_portrait.h               # Phase-space trajectory visualization
│   │   └── energy_tracker.h               # Per-resonator energy history
│   │
│   ├── 📁 hardware/                       # Physical device interfaces
│   │   ├── fpga_driver.h                  # FPGA ADC/DAC I/O control
│   │   ├── hardware_adapter.h             # Hardware-in-the-loop sensor/actuator bridge
│   │   └── mems_calibration.h             # MEMS sensor calibration routines
│   │
│   ├── 📁 energy/                         # Energy-domain wrappers
│   │   ├── energy_mesh.h                  # Energy mesh API wrapper
│   │   ├── gain_operator.h                # Gain operator API wrapper
│   │   └── coherence_observer.h           # Coherence observer API wrapper
│   │
│   └── 📁 spiral_time/                    # Optional Spiral-Time observer layer
│       ├── spiral_time_observer.h         # Observer that annotates FEEN trajectories
│       └── spiral_time_state.h            # Spiral-Time semantic state container
│
├── 📁 apps/                               # High-level application templates
│   ├── neural_network.h                   # Phononic neural network
│   ├── signal_processing.h                # Filters and spectral transforms
│   └── oscillator_bank.h                  # Frequency-multiplexed oscillator bank
│
├── 📁 examples/                           # Step-by-step C++ tutorials
│   ├── 01_basic_oscillator.cpp            # Create and simulate a simple resonator
│   ├── 02_bistable_bit.cpp                # Build a phononic memory cell
│   ├── 03_frequency_multiplexing.cpp      # Parallel computation channels
│   ├── 04_logic_gates.cpp                 # Phononic AND, OR, NOT gates
│   └── 05_neural_network.cpp              # Analog computing with resonator arrays
│
├── 📁 python/                             # Python layer
│   ├── pyfeen.cpp                         # pybind11 interface (FEEN core + AILEE)
│   ├── ailee.py                           # Python façade for AILEE primitives
│   ├── feen_rest_api.py                   # Flask REST API server
│   ├── vcp_integration.py                 # VCP Connectivity backend (read-only observer)
│   ├── plugin_registry.py                 # Plugin lifecycle manager
│   ├── requirements.txt                   # Python runtime dependencies
│   ├── CMakeLists.txt                     # pybind11 build rules
│   │
│   ├── 📁 plugins/                        # Built-in plugin modules
│   │   ├── __init__.py
│   │   ├── ui_dashboard.py                # Read-only energy-history panel (UI)
│   │   ├── observer_logger.py             # State logging observer (OBSERVER)
│   │   ├── hardware_monitor.py            # Hardware telemetry monitor (TOOL)
│   │   └── hlv_dynamics.py                # HLV Dynamics Lab — Kuramoto P1/P2/P3 + O1/O2 (TOOL)
│   │
│   ├── 📁 examples/                       # Python usage examples
│   │   ├── README.md                      # Setup guide and pyfeen API quick reference
│   │   ├── plot_bifurcation.py            # Bifurcation diagram via pyfeen
│   │   ├── rest_api_demo.py               # REST API walkthrough
│   │   ├── reservoir_computing.py         # Reservoir computing: 16-node physical reservoir
│   │   ├── cpg_control.py                 # CPG control: quadruped gait coordination
│   │   ├── structural_health_monitoring.py# Structural health monitoring: damage detection
│   │   └── distributed_synchronization.py # Distributed synchronization: R(t) order parameter
│   │
│   └── 📁 tests/                          # Python test suite
│       ├── test_ailee_rest_endpoints.py   # AILEE REST endpoint integration tests
│       ├── test_plugin_registry.py        # Plugin lifecycle & boundary tests
│       ├── test_vcp_wiring_invariants.py  # VCP wiring invariant tests
│       └── test_vcp_connectivity_endpoint.py # VCP Connectivity endpoint tests
│
├── 📁 tests/                              # C++ validation & unit tests
│   ├── CMakeLists.txt                     # CTest configuration
│   ├── test_resonator.cpp                 # Resonator physics validation
│   ├── unit_tests.cpp                     # Core unit tests
│   ├── numerical_accuracy.cpp             # Numerical accuracy checks
│   ├── test_ailee_metric.cpp              # Δv metric unit tests
│   ├── test_hardware_adapter.cpp          # Hardware adapter contract tests
│   └── test_spiral_time.cpp               # Spiral-Time observer tests
│
├── 📁 benchmarks/                         # Performance benchmarks
│   └── performance.cpp                    # Throughput and timing benchmarks
│
├── 📁 configs/                            # Example configuration files
│   ├── memory_cell.json                   # Monostable memory cell config
│   ├── filter_bank.yaml                   # Filter bank config
│   └── default_plugins.yaml              # Default plugin load list
│
├── 📁 docs/                               # Documentation
│   ├── APPLICATIONS.md                    # Application domains (reservoir computing, CPG, SHM, synchronization)
│   ├── FEEN.md                            # Complete system architecture
│   ├── FEEN_WAVE_ENGINE.md                # Mathematical foundations
│   ├── HARDWARE_IN_THE_LOOP.md            # HIL integration strategy
│   ├── REST_API.md                        # REST API endpoint reference
│   └── SPIRAL_TIME.md                     # Spiral-Time observer specification
│
├── 📁 domains/                            # Domain-specific overlays
│   ├── 📁 BioMesh/                        # Biological networks and cybernetic hematopoiesis
│   │   ├── BioMesh_Tests.cpp              # Unit tests for BioMesh mechanics
│   │   ├── CYBERNETIC_HEMATOPOIESIS.md    # Theoretical framework on wave-native eradication of SCD
│   │   ├── OsteoMesh.cpp                  # BioMesh domain implementation
│   │   └── OsteoMesh.hpp                  # BioMesh domain interfaces
│   ├── 📁 medicine/                       # Pharmacology and skeletal topology models
│   │   ├── pharma.hpp                     # Medicine domain interfaces
│   │   ├── pharma.cpp                     # Medicine domain implementation
│   │   ├── NOTE.md                        # Skeletal topological resilience note
│   │   └── PAPER.md                       # Wave-native pharmacokinetics concept paper
│   ├── 📁 satellite/                      # Fractionated spacecraft swarm models
│   │   └── SatelliteSwarm.hpp             # Satellite swarm domain core types and logic
│   └── 📁 energy/                         # Grid/DER energy mesh domain
│       └── EnergyMesh.hpp                 # Energy domain core types and logic
│
├── 📁 web/                                # Web application (Flask)
│   ├── app.py                             # Route definitions and entry point
│   ├── requirements.txt                   # Web runtime dependencies
│   ├── 📁 templates/                      # Jinja2 HTML templates
│   │   ├── dashboard.html                 # Dashboard — live overview tiles
│   │   ├── index.html                     # Simulation — primary workspace
│   │   ├── node_graph.html                # Nodes — resonator and plugin visualization
│   │   ├── coupling.html                  # Coupling — interactive coupling editor
│   │   ├── vcp_connectivity.html          # VCP Connectivity — live VCP graph & FEEN metrics
│   │   ├── vcp_wiring.html                # VCP Wiring — Verified Control Protocol view
│   │   ├── ailee_metric.html              # AILEE Metrics — live Δv visualization
│   │   ├── hlv_lab.html                   # HLV Dynamics Lab — Kuramoto experiments & results export
│   │   ├── docs.html                      # API Reference — human-readable REST docs
│   │   └── hardware.html                  # Hardware Link — Bluetooth bridge & test console
│   └── 📁 static/                         # Frontend assets
│       ├── css/style.css                  # Global stylesheet
│       ├── css/node_graph.css             # Node-graph panel styles
│       ├── js/main.js                     # Dashboard JavaScript
│       └── js/node_graph.js               # Node-graph visualization logic
│
├── CMakeLists.txt                         # Root CMake build configuration
├── vcpkg.json                             # C++ dependencies (vcpkg manifest)
├── Dockerfile                             # Container image definition
├── render.yaml                            # Render.com deployment config
├── CITATION.cff                           # Academic citation metadata
├── BENEFITS.md                            # Summary of repository benefits
└── LICENSE                               

```

---

## Examples

### Monostable Memory Cell

```cpp
#include <feen/resonator.h>

// Create analog memory
feen::ResonatorConfig cfg;
cfg.frequency_hz = 1000.0;
cfg.q_factor = 500.0;
cfg.beta = 1e-4;  // Positive = monostable

feen::Resonator memory(cfg);
memory.inject(0.75);  // Store value 0.75

// ... time passes ...

if (memory.snr() > 10.0) {
    double value = memory.total_energy();
    // Value still readable
}
```

### Bistable Logic Gate

```cpp
#include <feen/gates.h>

// Create phononic AND gate
feen::PhononicAND gate(1000.0);  // 1 kHz

// Set inputs
gate.set_input_a(true);
gate.set_input_b(true);

// Compute (evolve dynamics)
gate.compute(0.01);  // 10 ms evolution

// Read output
bool result = gate.get_output();  // true
```

### Frequency Multiplexing

```cpp
#include <feen/network.h>
#include <feen/resonator.h>

// Create 10 independent channels
feen::ResonatorNetwork network;

for (int i = 0; i < 10; ++i) {
    feen::ResonatorConfig cfg;
    cfg.frequency_hz = 1000.0 + i * 10.0;  // 1000, 1010, 1020 Hz...
    cfg.q_factor = 1000.0;
    cfg.beta = 1e-4;
    network.add_node(feen::Resonator(cfg));
}

// Verify isolation between adjacent channels (static helper on Resonator)
double isolation = feen::Resonator::isolation_db(network.node(0), network.node(1));
assert(isolation < -40.0);  // >40 dB isolation
```

### Python Analysis

```python
import pyfeen
import matplotlib.pyplot as plt
import numpy as np

# Create resonator
config = pyfeen.ResonatorConfig()
config.frequency_hz = 1000.0
config.q_factor = 200.0
config.beta = -1e8  # Bistable

res = pyfeen.Resonator(config)

# Scan initial conditions
x_range = np.linspace(-0.01, 0.01, 100)
final_states = []

for x0 in x_range:
    res.inject(x0)
    for _ in range(10000):
        res.tick(1e-6)
    final_states.append(res.x())

# Plot bifurcation diagram
plt.plot(x_range, final_states, 'b.', markersize=1)
plt.xlabel('Initial Position')
plt.ylabel('Final State')
plt.title('Bistable Resonator Bifurcation')
plt.show()
```

### REST API

The FEEN REST API provides HTTP access to resonator networks with global node control:

```bash
# Start the REST API server
cd python
pip install -r requirements.txt
python3 feen_rest_api.py

# Add a resonator node
curl -X POST http://localhost:5000/api/network/nodes \
  -H "Content-Type: application/json" \
  -d '{"frequency_hz": 1000.0, "q_factor": 200.0, "beta": 1e-4}'

# Inject a signal
curl -X POST http://localhost:5000/api/network/nodes/0/inject \
  -H "Content-Type: application/json" \
  -d '{"amplitude": 1.0, "phase": 0.0}'

# Evolve the network
curl -X POST http://localhost:5000/api/network/tick \
  -H "Content-Type: application/json" \
  -d '{"dt": 1e-6, "steps": 1000}'

# Get global network state
curl http://localhost:5000/api/network/state
```

**Key Features:**
- Global node access via `/api/network/state` endpoint
- Synchronized network evolution with `/api/network/tick`
- RESTful CRUD operations for resonator nodes
- Real-time state monitoring and control

See [REST API Documentation](docs/REST_API.md) for complete endpoint reference.

---

## Plugin System

FEEN includes a sandboxed plugin architecture that lets you extend the REST API and dashboard without touching the physics core.

### Plugin Types

| Type | HTTP Access | Use Case |
|------|-------------|----------|
| **UI** | None | Serve static assets / template panels |
| **OBSERVER** | GET only | Read-only analysis, logging, monitoring |
| **TOOL** | GET + POST | Command-capable automation and control |

### Plugin Lifecycle

```
load → register → activate → (running) → deactivate → unload
```

- **Observer boundary enforcement**: OBSERVER/UI plugins that attempt POST requests raise `ObserverBoundaryViolation`
- **Isolation**: every plugin runs inside a `try/except` guard; failures are contained
- **Flask Blueprints**: plugins optionally return a Blueprint, mounted at `/plugins/<name>/`
- **API versioning**: each plugin declares a compatible FEEN API range; incompatible plugins are rejected at load time

### Built-in Plugins

```python
from plugin_registry import PluginRegistry

registry = PluginRegistry()
registry.load_plugin("python/plugins/ui_dashboard.py")    # UI — energy-history panel
registry.load_plugin("python/plugins/observer_logger.py") # OBSERVER — state logger
registry.load_plugin("python/plugins/hardware_monitor.py")# TOOL — hardware telemetry
registry.activate_all()
```

---

## Validation Suite

FEEN includes a comprehensive physics validation framework:

### Test 1: Thermodynamic Consistency
```cpp
✓ Energy decreases monotonically (2nd law)
✓ Decay rate matches theoretical exp(-πf₀t/Q)
✓ SNR remains above threshold during sustain window
```

### Test 2: Bistable Equilibrium
```cpp
✓ Barrier height ΔU >> k_B T
✓ Switching time τ_switch >> sustain window
✓ No spontaneous bit flips from thermal noise
```

### Test 3: Spectral Isolation
```cpp
✓ Lorentzian isolation < -20 dB for 1% frequency separation
✓ Independent channels don't interfere
✓ Frequency orthogonality preserved under evolution
```

Run all tests:
```bash
cd build
./tests/unit_tests
./tests/numerical_accuracy
ctest --verbose
```

---

## Theory

### The Duffing Equation

FEEN resonators obey the nonlinear Duffing oscillator:

```
ẍ + 2γẋ + ω₀²x + βx³ = F cos(ωt)
```

Where:
- **x**: displacement from equilibrium
- **γ**: damping coefficient (= ω₀/2Q)
- **ω₀**: natural frequency (= 2πf₀)
- **β**: nonlinearity (positive → monostable, negative → bistable)
- **F**: driving force amplitude

### Potential Energy Landscape

**Monostable (β > 0):**
```
U(x) = ½ω₀²x² + ¼βx⁴
```
Single well → analog storage

**Bistable (β < 0):**
```
U(x) = -½ω₀²x² + ¼|β|x⁴
```
Double well → digital logic

Stable states at: **x* = ±ω₀/√|β|**

Energy barrier: **ΔU = ω₀⁴/(4|β|)**

### Key Metrics

| Metric | Formula | Significance |
|--------|---------|--------------|
| **Quality Factor** | Q = ω₀/(2γ) | Energy storage efficiency |
| **Decay Time** | τ = Q/(πf₀) | Memory lifetime |
| **SNR** | E_signal / (k_B T) | Readability above noise |
| **Isolation** | -10 log₁₀[1+(2QΔf/f₀)²] | Channel independence |

---

## Building & Dependencies

### CMake Build

```bash
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_PYTHON=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_BENCHMARKS=ON
make -j$(nproc)
sudo make install
```

### Dependencies (via vcpkg)

```json
{
  "dependencies": [
    "eigen3",           // Linear algebra
    "boost-math",       // Special functions
    "pybind11",         // Python bindings
    "catch2",           // Testing framework
    "yaml-cpp",         // Config parsing
    "nlohmann-json"     // JSON support
  ]
}
```

### Python Installation

```bash
cd python
pip install -e .
```

```python
import pyfeen
help(pyfeen.Resonator)
```

---

## Performance

Benchmarks on Intel i7-12700K (3.6 GHz):

| Operation | Time | Throughput |
|-----------|------|------------|
| Single resonator tick (RK4) | 120 ns | 8.3M steps/sec |
| 100-resonator network tick | 12 μs | 83k steps/sec |
| Bistable convergence (10ms) | 1.2 ms | 833 simulations/sec |
| FFT spectrum (1024 pts) | 45 μs | 22k transforms/sec |

Memory footprint: **96 bytes** per resonator (state + config)

---

## Contributing

We welcome contributions! Here's how to get started:

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Development Guidelines

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Add unit tests for new features
- Update documentation
- Ensure all physics validation tests pass

### Areas We Need Help

- Hardware interfaces for real MEMS devices
- Advanced neural network architectures
- Visualization and GUI tools
- More examples and tutorials
- Language bindings (Rust, Julia, MATLAB)

---

## **Roadmap**

The roadmap below is directional (not date‑committed) and will continue to evolve with validation results and community feedback.

### **v4.X.X — Platform Consolidation, Tooling Maturity, and Cross‑Domain Expansion**
- [ ] **Distributed resonator‑cluster execution patterns**  
  Formalize multi‑node execution semantics, scheduling, and synchronization for large‑scale FEEN deployments.

- [ ] **Automatic topology & circuit synthesis from constraints**  
  Generate resonator networks directly from logical, physical, or optimization constraints.

- [ ] **SDK maturation for production‑grade phononic applications**  
  Harden the public API surface, stabilize domain headers, and finalize long‑term compatibility guarantees.

- [ ] **Deep diagnostics & performance instrumentation**  
  Expand tracing, profiling, and observability hooks across simulation, connectivity, and hardware layers.

- [ ] **Hardware bridge hardening & observability**  
  Improve error surfaces, latency reporting, and device‑level introspection for FEEN‑backed hardware.

- [ ] **Expanded Python examples for reproducible workflows**  
  Provide end‑to‑end notebooks demonstrating simulation, connectivity, diagnostics, and domain interactions.

- [ ] **Documentation & site navigation stabilization**  
  Improve discoverability of endpoints, domain APIs, and cross‑domain examples; unify docs structure.

- [ ] **VCP observer documentation & cross‑repo integration references**  
  Clarify how FEEN, VCP, and ambient‑node interact; provide diagrams and minimal reproducible examples.

- [ ] **Dashboard discoverability improvements**  
  Refine UI flows for simulation, diagnostics, topology editing, and HLV/observer views.

---

## Citation

If you use FEEN in your research, please cite:

```bibtex
@software{feen2025,
  title = {FEEN Wave Engine: A Physics-First Phononic Computing Framework},
  author = {Feeney, D.M.},
  year = {2025},
  version = {3.8.1},
  url = {https://github.com/dfeen87/feen}
}
```

See [CITATION.cff](CITATION.cff) for more citation formats.

---

## License

This project is 100% open-source and distributed under the **MIT License**.
You may use, modify, and distribute this software in commercial and non-commercial contexts in accordance with the license terms.

See [LICENSE](LICENSE) for details.


---

## Acknowledgments

This project builds on decades of research in:
- Nonlinear dynamics and chaos theory
- MEMS/NEMS fabrication techniques
- Phononic crystals and metamaterials
- Analog computing paradigms

Special thanks to:
- Contributors to open-source scientific computing
- Early adopters providing feedback and validation

This project was developed with a combination of original ideas, hands‑on coding, and support from advanced AI systems. I would like to acknowledge **Microsoft Copilot**, **Anthropic Claude**, **Google Jules** and **OpenAI ChatGPT** for their meaningful assistance in refining concepts, improving clarity, and strengthening the overall quality of this work.

---

## Enterprise Consulting & Integration
This architecture is fully open-source under the MIT License. If your organization requires custom scaling, proprietary integration, or dedicated technical consulting to deploy these models at an enterprise level, please reach out at: dfeen87@gmail.com

---

### Community
- [Discussions](https://github.com/dfeen87/feen/discussions) - Ask questions, share ideas
- [Issues](https://github.com/dfeen87/feen/issues) - Report bugs, request features

---

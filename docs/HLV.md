# FEEN Platform Test Plan for Structured Phase–Memory Dynamics: An Implementation-Ready Protocol for Falsifiable Network Experiments

**Marcel Krüger**
*Independent Researcher, Germany — marcelkrueger092@gmail.com*

**Don Feeney**
*Independent Researcher, USA — dfeen87@gmail.com*

*Dated: February 23, 2026*

---

> We specify a minimal, implementation-ready protocol for testing structured phase–memory dynamics on the FEEN platform. The framework is model-agnostic and relies only on modular physics plugins and observer hooks. The plan provides three falsifiable experimental suites targeting (i) memory-induced shifts in synchronization thresholds, (ii) phase-offset driven attractor formation, and (iii) robustness of deterministic regime-detection metrics such as ΔΦ. All runs are defined with reproducibility-grade artifact requirements suitable for APS-style reporting.

---

## I. FEEN Platform Test Plan for Structured Phase–Memory Dynamics

This section specifies a minimal, implementation-ready framework for testing structured phase–memory hypotheses on the FEEN platform. The formulation is strictly model-agnostic: only modular physics plugins and observer hooks are assumed. The objective is to evaluate—in a falsifiable manner—whether memory kernels and phase-offset couplings modify (i) stability landscapes, (ii) attractor structure, and (iii) regime-detection robustness.

---

### A. Architecture: Physics Hook + Observer Hook

FEEN exposes two orthogonal extension interfaces:

**1. Physics interface (state derivatives)**

$$\dot{x} = F(t, x;\, G, \theta) \tag{1}$$

**2. Observer interface (metrics and logging)**

$$y(t) = O(t, x(t);\, G, \theta) \tag{2}$$

where the minimal phase-network state is:

$$x \equiv \{\theta_i(t),\, \omega_i,\, m_i(t)\}_{i=1}^{N} \tag{3}$$

*G* denotes the coupling graph, and θ are plugin parameters. The memory variable *mᵢ* is optional and required only for non-Markovian tests.

---

### B. Graph Representation

The physical network is defined by weighted edges and optional directed offsets:

$$A_{ij} \geq 0, \quad \phi_{ij} \in \mathbb{R} \tag{4}$$

where *Aᵢⱼ* is the coupling weight and *φᵢⱼ* encodes a phase offset (e.g., chirality). If offsets are absent, set *φᵢⱼ* = 0.

---

### C. Physics Plugins

#### P1: Kuramoto Baseline (Control)

$$\dot{\theta}_i = \omega_i + \kappa \sum_j A_{ij} \sin(\theta_j - \theta_i) + \sigma\,\xi_i(t) \tag{5}$$

with optional noise *ξᵢ(t)* (e.g., Gaussian white noise).
**Parameters:** θ_P1 = {κ, σ, seed, frequency distribution}

---

#### P2: Exponential Memory Kernel (Low Overhead)

$$\dot{\theta}_i = \omega_i + \kappa \sum_j A_{ij} \sin(\theta_j - \theta_i) + \eta\,m_i + \sigma\,\xi_i(t) \tag{6}$$

$$\dot{m}_i = -\frac{1}{\tau_m} m_i + \sum_j A_{ij} \sin(\theta_j - \theta_i) \tag{7}$$

**Parameters:** θ_P2 = {κ, η, τₘ, σ, seed}. This implements a controlled, stable non-Markovian extension suitable for parameter sweeps.

---

#### P3: Phase-Offset (Chiral) Coupling

$$\dot{\theta}_i = \omega_i + \kappa \sum_j A_{ij} \sin(\theta_j - \theta_i + \phi_{ij}) + \sigma\,\xi_i(t) \tag{8}$$

**Offset generation modes:**

$$\phi_{ij} = \begin{cases} +\phi_0 & (i \to i+1) \text{ forward edges on a ring} \\ -\phi_0 & (i \to i-1) \text{ backward edges on a ring} \\ \text{Uniform}[\phi_{\min}, \phi_{\max}] & \text{random offsets} \\ \text{edge attribute} & \text{if provided by the API} \end{cases} \tag{9}$$

---

### D. Observer Modules

#### O1: Synchronization Metric (Emergent Clock)

Compute the Kuramoto order parameter:

$$R(t)\,e^{i\psi(t)} = \frac{1}{N} \sum_{j=1}^{N} e^{i\theta_j(t)}, \quad R \in [0, 1] \tag{10}$$

and a dispersion measure *σ_θ(t)* (e.g., circular standard deviation).

#### O2: Deterministic Instability Functional ΔΦ(t)

Implement ΔΦ(t) as defined in the Spiral-Time / triadic operator work. At minimum, the observer must:
- accept a baseline window and deterministic normalization,
- export ΔΦ(t) time-stamped, and
- optionally export subchannels (ΔS, ΔI, ΔC) when available.

#### O3: Unified Logger

Persist a minimal channel set:

```
t,  R(t),  ψ(t),  σ_θ(t),  ΔΦ(t)
```

and optionally snapshot {θᵢ} every fixed stride for auditability.

---

### E. Minimal Platform Requirements

A minimal APS-ready engineering interface:

| Method | Description |
|---|---|
| `reset_network()` | Reset network state |
| `set_nodes(N, init_mode, seed)` | Initialize node set |
| `set_edges({(i,j,wᵢⱼ,φᵢⱼ)})` | Define weighted edges with optional offsets |
| `set_sim(dt, t_end, integrator, seed)` | Configure simulation parameters |
| `set_physics(plugin_name, params)` | Select and configure physics plugin |
| `set_observers(observer_list, params)` | Attach observer modules |
| `run()` / `stop()` / `reset()` | Execution control |
| `inject(node=i, type=phase_kick, amplitude=δθ)` | Phase perturbation injection |
| `inject(node=i, type=omega_kick, amplitude=δω, duration)` | Frequency perturbation injection |

---

### F. Immediate Experimental Protocols

#### E1: Memory-Induced Threshold Shift

**Hypothesis.** Memory modifies the synchronization threshold κ_c for high synchrony (R → 1).

**Protocol:**
1. Topology: ring, *N* = 32 (repeat on small-world and Erdős–Rényi).
2. Compare **P1** [Eq. (5)] vs **P2** [Eqs. (6)–(7)].
3. Sweep κ ∈ [0, 6] with step Δκ = 0.2, across *S* = 10 seeds.
4. Compute *R* as a time-average over the final 20% of runtime and record settling time.

**Evaluation.** A reproducible shift in κ_c(η, τₘ) beyond seed variance supports a genuine memory effect; absence of shift falsifies the hypothesis under tested conditions.

---

#### E2: Phase-Offset Attractor Formation

**Hypothesis.** Nonzero *φᵢⱼ* generates stable clustered or chiral attractors absent in the baseline.

**Protocol.** Use **P3** with ring-chiral offsets and sweep φ₀ ∈ [0, π/2]. Quantify attractor type via:
- *R(t)*
- phase histograms
- a clustering score on θᵢ

---

#### E3: ΔΦ Regime-Detection Benchmark

**Hypothesis.** ΔΦ(t) detects regime transitions more robustly than generic baselines (e.g., variance, simple entropy proxies) under disorder and noise.

**Protocol.** Operate **P2**/**P3** near criticality, inject controlled perturbations (phase/frequency kick), and compare detection lead-time and false-positive rate of ΔΦ(t) against thresholding on *R(t)* and *σ_θ(t)*.

---

### G. Scaling and Sweep Regime Table

| Regime | Condition | Dominant Effect | Interpretation |
|---|---|---|---|
| Undercoupled | κ̃ ≲ 1 | Local drift dominates | No global phase lock; low *R* |
| Critical | κ̃ ∼ 1 | Marginal stability | High sensitivity; optimal for transition tests |
| Overcoupled | κ̃ ≫ 1 | Rapid convergence | Robust collective mode; fast settling |
| Memory-limited | τ̃ₘ ≫ 1 | Strong temporal inertia | Delayed recovery; hysteresis possible |
| Noise-limited | σ̃ ≳ 1 | Stochastic diffusion | False positives increase; test observer robustness |
| Offset-dominated | φ₀ ≉ 0 | Phase frustration / chirality | Clustered or chiral attractors may emerge |

---

### H. Scientific Validity and Non-Circularity

These tests do not assume correctness of any higher-level framework. They isolate measurable nonlinear-network properties:
- (i) memory-dependent stability modification,
- (ii) offset-induced attractor formation, and
- (iii) deterministic regime-detection robustness.

All claims remain falsifiable at the dynamical-systems level and can be benchmarked against standard baselines within the same platform.

---

## Appendix A: Reproducibility and Artifact Specification (FEEN Sweeps)

This Appendix defines a minimal artifact standard suitable for APS-style reproducibility.

### A.1 Run Configuration Schema

Each run **MUST** export a machine-readable configuration record (JSON recommended) including:

| Field | Description |
|---|---|
| Code version | Git commit hash and platform version string |
| Physics plugin | Plugin name and full parameter dictionary |
| Graph descriptor | Topology class, *N*, edge list or RNG seed, weight distribution |
| Simulation config | Integrator, timestep *dt*, end time *t_end*, noise model (if used) |
| Random seeds | Seeds for initialization, graph RNG, and noise RNG |

---

### A.2 Artifact Bundle

Each sweep produces one bundle directory:

```
run_id/
├── config.json      # full run configuration
├── metrics.csv      # time series (see below)
├── events.jsonl     # perturbation log with timestamps
└── hash.txt         # SHA-256 over config.json + metrics.csv + events.jsonl
```

**`metrics.csv` header:**

```
t,  R,  ψ,  σ_θ,  ΔΦ  [, optional subchannels]
```

**`events.jsonl`:** injected perturbations with timestamps and magnitudes.

---

### A.3 Sweep Protocol and Statistical Reporting

For each sweep point (e.g., fixed κ), report:

| Metric | Description |
|---|---|
| Seeds *S* | Number of independent seeds |
| Mean ± SE of *R* | Final-window time average |
| Settling time | Time to reach *R* > *R_thr* |
| Detection lead-time | ΔΦ vs. baselines (E3) |
| False-positive rate | Under null perturbations (no injection) |

---

### A.4 Null Tests

To guard against circularity:

| Control | Procedure | Expected Result |
|---|---|---|
| Time-shuffle | Shuffle node indices or permute edges preserving degree sequence | Degradation of structured attractors |
| Noise-only | Set κ = 0 | ΔΦ does not spuriously trigger |
| Offset-off | Set φᵢⱼ = 0 | Loss of chiral patterns confirmed |

---

## Figure 1: APS-Style Implementation Pipeline

```
┌─────────────────────┐
│     Graph Layer     │
│   G : (Aᵢⱼ, φᵢⱼ)  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   Physics Plugin    │
│   ẋ = F(·)         │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Integrator / Run   │
│       x(t)          │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   Observer Hook     │
│   y(t) = O(·)       │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│      Metrics        │
│ R(t), ψ(t), σ_θ(t),│
│       ΔΦ(t)         │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│      Artifacts      │
│ CSV/JSONL + config  │
│        hash         │
└──────────┬──────────┘
           │
      sweep & reproduce
```

*APS-style implementation pipeline for FEEN tests: (i) define the graph layer (weights and optional offsets), (ii) select a physics plugin, (iii) integrate dynamics, (iv) compute observers, and (v) export reproducible artifacts (time series plus configuration hash).*

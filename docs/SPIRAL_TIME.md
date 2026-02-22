# FEEN: A Phononic Mesh Network Without a Central Clock
### A Rigorous Dynamical Model, Memory Extensions, and Falsifiable Signatures

**Marcel Krüger**  
Independent Researcher, Meiningen, Germany  
marcelkrueger092@gmail.com | ORCID: 0009-0002-5709-9729

**Don Feeney**  
Independent Researcher, Pennsylvania, USA  
dfeen87@gmail.com | ORCID: 0009-0003-1350-4160

*February 22, 2026*

> Spiral‑Time is a semantic and observational framework for FEEN trajectories and does not alter the underlying physical dynamics unless explicitly enabled.

---

## Abstract

We formalize FEEN, a distributed phononic mesh network that performs timing, sensing, and control without a globally broadcast clock. Each node hosts a damped resonant mode, coupled locally to its neighbors. We derive a coupled-mode network model and show how, in appropriate limits, it reduces to phase oscillator dynamics admitting an emergent time reference via synchronization. We then extend the framework to non-Markovian regimes by introducing intrinsic memory kernels and provide a deterministic observer functional (optionally compatible with triadic spiral-time analysis) for regime detection and control. We present stability criteria, measurable performance metrics (energy-per-operation, coherence time, synchronization error), and a falsification program including null hypotheses and ablation tests. The result is a testable architecture bridging wave-based computing, oscillator networks, and open-system dynamics.

**Keywords:** phononic networks; synchronization; non-Markovian dynamics; coupled-mode theory; wave-based computing

---

## Contents

1. [Introduction](#1-introduction)
2. [Related Work and Engineering Positioning](#2-related-work-and-engineering-positioning)
3. [System Definition and Architecture](#3-system-definition-and-architecture)
4. [Coupled-Mode Model (Phononic Mesh Dynamics)](#4-coupled-mode-model-phononic-mesh-dynamics)
5. [Phase Reduction and Emergent Synchronization](#5-phase-reduction-and-emergent-synchronization)
6. [Intrinsic Memory Extensions (Non-Markovian FEEN)](#6-intrinsic-memory-extensions-non-markovian-feen)
7. [Deterministic Observer Layer](#7-deterministic-observer-layer-optional-δφ-type-functional)
8. [Pipeline Diagram](#8-pipeline-diagram)
9. [Performance Metrics and Evaluation Protocol](#9-performance-metrics-and-evaluation-protocol)
10. [Falsification Program and Null Hypotheses](#10-falsification-program-and-null-hypotheses)
11. [Experimental Roadmap (Prototype)](#11-experimental-roadmap-prototype)
12. [Discussion](#12-discussion)
13. [Conclusion](#13-conclusion)
14. [References](#references)

---

## 1 Introduction

Modern distributed systems rely on centralized timing (global clocks, GPS time, synchronous buses) or complex clock-recovery protocols. In contrast, physical wave media — optical, mechanical, or acoustic — support information processing where phase and frequency carry meaning, and global synchronization may emerge from local coupling alone.

This paper introduces and formalizes **FEEN**, a phononic mesh network without a central clock. The core hypothesis is that a network of locally coupled resonant modes can generate a stable, coherent reference phase ("emergent time") and perform sensing/control tasks with potentially favorable energy scaling compared to conventional clocked architectures.

**Main contributions:**

1. A coupled-mode network model for phononic resonator meshes, including damping and forcing.
2. A phase reduction yielding Kuramoto-type dynamics and an emergent order parameter.
3. Non-Markovian extensions via intrinsic memory kernels and state-space embeddings.
4. Stability and falsification criteria, including explicit null hypotheses and ablations.
5. A complete simulation and experimental roadmap with measurable metrics.

**Scope and positioning.** We do not claim that FEEN requires new physics: synchronization and wave computing are established. The novelty lies in a rigorous end-to-end formulation targeting clockless operation with explicit falsifiable signatures, and in a structured pathway to incorporate intrinsic memory and deterministic observers (for open systems) as a separable layer.

---

## 2 Related Work and Engineering Positioning

### 2.1 Clock distribution vs. clockless operation

Conventional distributed platforms rely on explicit clock distribution (e.g., PLL-based clock trees, time-stamping protocols, GPS-disciplined references) or on digital clock recovery in communication links. In these settings, timing is treated as a dedicated infrastructure layer.

FEEN targets an alternative: timing emerges from local coupling of physical oscillators and is quantified by a measurable coherence order parameter (Eq. 6). This moves timing from a global service to a distributed collective state.

### 2.2 Coupled oscillator networks and synchronization engineering

Synchronization phenomena in coupled oscillators (Kuramoto-type models and their extensions) are well established and used in engineering contexts, including distributed time-keeping, sensor networks, and neuromorphic timing primitives. The contribution of FEEN is not the existence of synchronization itself, but a full-stack formulation connecting (i) coupled-mode physics (Eq. 1), (ii) phase reduction (Eq. 5), and (iii) system-level metrics plus falsification tests (Sec. 10).

### 2.3 Wave-based and reservoir computing

Physical reservoir computing and wave-based computing architectures exploit the rich transient dynamics of analog substrates (optical, mechanical, or electronic) for inference and control. FEEN is compatible with this direction but focuses on a distinct requirement: clockless operation with an emergent timing reference. In practice, FEEN can be viewed as a wave-based computing substrate whose collective phase ψ(t) provides an internal time coordinate usable for sensing/control tasks without external timing distribution.

### 2.4 Memory in dynamical systems: bath-induced vs. intrinsic

Non-Markovianity in open-system dynamics is commonly attributed to finite environments, structured spectral densities, or engineered baths. FEEN introduces a separable modeling layer for intrinsic temporal memory via kernels (Eq. 7) and provides explicit null hypotheses (H₀–H₂, Sec. 10) to distinguish intrinsic memory signatures from finite-bath artifacts using reset/ablation protocols. This is framed as an engineering validation problem: identify operating regimes where measured memory metrics remain inconsistent with standard bath explanations under controlled interventions.

### 2.5 Engineering novelty statement

In engineering terms, FEEN is positioned as a clockless oscillator-mesh platform with (i) a physics-grounded dynamical model, (ii) measurable coherence and energy metrics, and (iii) a falsification program that explicitly rules out common artifact classes (hidden drives, parameter drift, finite-bath memory).

---

## 3 System Definition and Architecture

### 3.1 Graph topology and local coupling

We model the mesh as an undirected (or directed) graph **G = (V, E)** with **N = |V|** nodes and edge set **E**. Neighborhoods are defined as:

> **𝒩(i) = { j ∈ V : (i, j) ∈ E }**

Each node hosts a localized phononic mode, physically realizable as a MEMS/NEMS resonator, acoustic cavity, SAW device, or metamaterial cell.

### 3.2 Physical signals and information encoding

Information is encoded in one (or more) of the following observables:

| Observable | Symbol | Description |
|---|---|---|
| Phase | θᵢ(t) | Local oscillation phase |
| Instantaneous frequency | θ̇ᵢ(t) | Rate of phase change |
| Amplitude envelope | rᵢ(t) | Signal amplitude |
| Mode energy | \|aᵢ(t)\|² | Complex amplitude squared |

A "clock" is not broadcast. Instead, a coherent reference is defined by the **collective phase** of the network (Sec. 5).

---

## 4 Coupled-Mode Model (Phononic Mesh Dynamics)

### 4.1 Complex amplitude dynamics

Let **aᵢ(t) ∈ ℂ** be the modal amplitude at node *i*, with resonance frequency **Ωᵢ**, damping rate **γᵢ > 0**, and coupling **κᵢⱼ**. A standard coupled-mode model is:

```
                         ȧᵢ(t) = −(iΩᵢ + γᵢ) aᵢ(t) − i Σⱼ∈𝒩(ᵢ) κᵢⱼ aⱼ(t) + sᵢ(t)    (1)
```

where **sᵢ(t)** is a local injection/perturbation term (drive, sensing input, or noise).

**Matrix form.** Let **a = (a₁, …, aₙ)ᵀ** and define a (generally complex) system matrix:

```
                         ȧ = Ma + s(t),    M := −iΩ − Γ − iK                           (2)
```

with **Ω = diag(Ωᵢ)**, **Γ = diag(γᵢ)**, and **K** encoding couplings.

### 4.2 Linear stability criterion

A steady state (or operating point) **a\*** is linearly stable if the Jacobian **J = M** has eigenvalues **λₖ** satisfying:

```
                         Re[λₖ(M)] < 0    ∀k                                            (3)
```

This provides a direct engineering test: damping and coupling must yield **net contraction** in the linearized dynamics.

### 4.3 Inclusion of nonlinear saturation (optional but realistic)

Real resonators exhibit amplitude saturation. A minimal extension is:

```
        ȧᵢ = −(iΩᵢ + γᵢ) aᵢ − (ηᵢ + iβᵢ)|aᵢ|² aᵢ − i Σⱼ∈𝒩(ᵢ) κᵢⱼ aⱼ + sᵢ(t)       (4)
```

with **ηᵢ > 0** for nonlinear damping and **βᵢ** for Duffing-type frequency shift. This supports stable limit cycles and robust phase locking.

---

## 5 Phase Reduction and Emergent Synchronization

### 5.1 Phase extraction and weak-coupling limit

Write **aᵢ(t) = rᵢ(t) eⁱθᵢ⁽ᵗ⁾**. In weak coupling and near steady amplitude, one obtains an effective phase model of Kuramoto type:

```
                         θ̇ᵢ = ωᵢ + Σⱼ∈𝒩(ᵢ) Kᵢⱼ sin(θⱼ − θᵢ) + ξᵢ(t)               (5)
```

where **ωᵢ** are effective frequencies and **Kᵢⱼ ≥ 0** effective couplings; **ξᵢ(t)** captures fluctuations.

### 5.2 Order parameter and emergent time reference

Define the **global order parameter**:

```
                         R(t) eⁱψ⁽ᵗ⁾ := (1/N) Σⱼ₌₁ᴺ eⁱθʲ⁽ᵗ⁾,    R(t) ∈ [0, 1]       (6)
```

> **Definition (Emergent clock).** We say FEEN exhibits an emergent time reference if **R(t) → R\* > 0** and the phase dispersion remains bounded, implying a stable collective phase **ψ(t)** usable as a global timing surrogate.

### Engineering Pipeline

```
┌─────────────────────────────────────────────────────────────────────┐
│                     FEEN Synchronization Pipeline                   │
├─────────────────────────┐                                           │
│   Graph Structure       │                                           │
│   G = (V, E)            │  Mesh topology & couplings                │
│                         │                                           │
│   Weighted Laplacian    │                                           │
│   L = D − W            │                                           │
└──────────┬──────────────┘                                           │
           │                                                          │
           ▼                                                          │
┌──────────────────────────┐    ┌──────────────────────────┐         │
│   Laplacian Spectrum     │    │   Phase Dynamics         │         │
│                          │───▶│   Eq. (5)                │         │
│  0 = λ₁ < λ₂ ≤ … ≤ λₙ  │    │                          │         │
└──────────────────────────┘    └──────────┬───────────────┘         │
                                           │                         │
           ┌───────────────────────────────┘                         │
           ▼                                                          │
┌──────────────────────────┐    ┌──────────────────────────┐         │
│   Coherence Metric       │    │   Engineering Handle     │         │
│   R(t), σ_θ(t)          │    │   connectivity ↑ ⟹ λ₂ ↑ │         │
│                          │◀───│   (algebraic             │         │
│                          │    │    connectivity)         │         │
└──────────────────────────┘    └──────────────────────────┘         │
           │                                                          │
           ▼                                                          │
     Design Feedback                                                  │
└─────────────────────────────────────────────────────────────────────┘
```

*Figure 1: Engineering pipeline connecting mesh topology to synchronizability. The Laplacian spectrum provides a design handle: increasing algebraic connectivity λ₂ typically improves synchronization robustness (lower phase dispersion) for fixed disorder/noise levels.*

**Design implication.** For fixed oscillator heterogeneity, increased coupling strength and improved graph connectivity (higher **λ₂**) expand the practical locking range and reduce the steady-state phase dispersion, directly improving the emergent clock stability.

### 5.3 Synchronization threshold (testable scaling)

For heterogeneous **ωᵢ**, coherence requires sufficiently strong coupling. On general graphs, the threshold depends on topology and disorder scale. A falsifiable signature is the existence of a coupling regime where **R\*** transitions from ≈ 0 to > 0 as coupling increases.

---

## 6 Intrinsic Memory Extensions (Non-Markovian FEEN)

### 6.1 Memory kernel formulation

To capture intrinsic temporal memory (beyond simple damping), we augment node dynamics by a memory kernel:

```
                 ẋᵢ(t) = fᵢ(xᵢ(t)) + Σⱼ∈𝒩(ᵢ) gᵢⱼ(xⱼ(t), xᵢ(t)) + ∫₀ᵗ Kᵢ(t−τ) hᵢ(xᵢ(τ)) dτ   (7)
```

where **xᵢ** may represent **(rᵢ, θᵢ)** or **aᵢ**, and **Kᵢ** is a kernel encoding intrinsic persistence.

**State-space embedding (Prony / auxiliary modes).** If **Kᵢ(t)** admits a sum-of-exponentials approximation:

```
                         Kᵢ(t) ≈ Σₘ₌₁ᴹ cᵢₘ e^(−λᵢₘt)
```

the model becomes Markovian in an extended state with auxiliary variables **uᵢₘ**:

```
                 ẋᵢ(t) = fᵢ(xᵢ) + Σⱼ gᵢⱼ(xⱼ, xᵢ) + Σₘ₌₁ᴹ uᵢₘ(t)                    (8)

                 u̇ᵢₘ(t) = −λᵢₘ uᵢₘ(t) + cᵢₘ hᵢ(xᵢ(t))                                (9)
```

This yields a practical route for simulation and control.

### 6.2 Null hypothesis and falsification target

A key empirical question is whether observed memory signatures can be explained by finite baths or engineered environmental coupling. FEEN + intrinsic memory predicts regimes where history-dependence persists under resets/ablations that would eliminate standard bath memory (Sec. 10).

---

## 7 Deterministic Observer Layer (Optional: ΔΦ-type Functional)

### 7.1 Observer functional

Define a deterministic observer functional on multichannel trajectories **y(t)**:

```
                 ΔΦ[y](t) := F( y(t), ẏ(t), ∫₀ᵗ w(t−τ) y(τ) dτ )                     (10)
```

where **F** is deterministic and **w** is a weighting kernel. This layer produces a compact state descriptor for regime detection and control.

> **Remark (separation of concerns).** The observer layer does *not* modify the underlying physics; it is a measurement/control map applied to trajectories. This is crucial for reviewer-proof positioning.

---

## 8 Pipeline Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                        FEEN Full Pipeline                        │
└──────────────────────────────────────────────────────────────────┘

        ┌─────────────────────────┐
        │     Physical Mesh       │
        │  (phononic resonators)  │
        └────────────┬────────────┘
                     │
                     ▼
        ┌─────────────────────────┐
        │   Coupled-Mode Model    │
        │        Eq. (1)          │
        └────────────┬────────────┘
                     │
                     ▼
        ┌─────────────────────────┐
        │     Phase Reduction     │
        │        Eq. (5)          │
        └────────────┬────────────┘
                     │
              ┌──────┴──────┐
              ▼             ▼
  ┌──────────────────┐  ┌──────────────────┐
  │  Emergent Clock  │  │ Intrinsic Memory  │
  │  Order param.    │  │    Eq. (7)        │
  │    Eq. (6)       │  └────────┬─────────┘
  └────────┬─────────┘           │
           │                     ▼
           │          ┌──────────────────────┐
           │          │ Deterministic Observer│
           │          │    ΔΦ  Eq. (10)      │
           │          └────────┬─────────────┘
           │                   │
           └─────────┬─────────┘
                     │
                     ▼
        ┌─────────────────────────┐
        │   Control / Sensing     │
        │  (metrics & tasks)      │
        └─────────────────────────┘
```

*Figure 2: FEEN pipeline: physical mesh → coupled-mode dynamics → phase reduction and emergent timing, with optional intrinsic memory and deterministic observer layers for detection/control.*

---

## 9 Performance Metrics and Evaluation Protocol

### 9.1 Metrics

| Metric | Formula | Description |
|---|---|---|
| Synchronization error | σ_θ(t) = √(1/N · Σᵢ (θᵢ − ψ)²) | RMS phase deviation from collective |
| Coherence time | Tᶜ : R(t) ≥ Rₘᵢₙ | Duration of maintained coherence |
| Energy-per-operation | Eₒₚ | Energy dissipated per timing/sensing update |
| Robustness | ΔR, ΔTᶜ | Degradation under noise/disorder |

### 9.2 Parameter-to-model mapping

**Table 1: Mapping physical parameters to reduced models.**

| Physical quantity | Coupled-mode parameter | Phase model effect |
|---|---|---|
| Resonance frequency | Ωᵢ in Eq. (1) | sets effective ωᵢ |
| Damping / loss | γᵢ in Eq. (1) | affects locking range, Tᶜ |
| Neighbor coupling | κᵢⱼ in Eq. (1) | sets Kᵢⱼ in Eq. (5) |
| Nonlinear saturation | ηᵢ, βᵢ in Eq. (4) | stabilizes amplitude, improves robustness |
| Intrinsic memory kernel | Kᵢ in Eq. (7) | history dependence beyond Markov |
| Noise / perturbations | sᵢ(t) or ξᵢ(t) | limits R\* and Tᶜ |

**Table 2: Claims vs. evidence status and required validation steps (engineering QA view).**

| Claim | Operational definition (measurable) | Current status | Validation / falsification |
|---|---|---|---|
| Emergent clock without global timing | R(t) → R\* > 0 and bounded σ_θ(t) across operating window | Model-derived (Sec. 5) | Measure R, σ_θ vs. coupling/disorder; verify reproducibility across runs |
| Drive-free sidebands | Spectral sidebands present when sᵢ(t) has no periodic component (verified instrumentation) | Prediction | Instrument ablation: disconnect external references; vary topology/coupling; verify persistence and scaling |
| Coherence plateau | Tᶜ and R\* remain stable over parameter range where Markovian baseline decays | Prediction | Compare against Markovian baseline model; show plateau survives reset/thermal drift controls |
| Topology-dependent phase offsets | Stable phase offsets correlated with graph cycles / symmetry classes | Prediction | Swap topology with same node count; check phase-offset changes follow topology not hardware placement |
| Reset-resistant memory | Non-Markovian metrics persist under "reset" interventions designed to erase bath correlations | Core discriminant prediction | Implement reset protocol; if memory vanishes, H₀ (finite bath) supported; if persists, supports intrinsic memory |
| Energy scaling advantage | Lower Eₒₚ at equal timing error / coherence target | Open (engineering question) | Measure dissipation vs. N and compare to digital clock distribution overhead under comparable accuracy targets |
| Observer functional utility (optional) | ΔΦ improves detection/control performance without altering physics | Optional layer | Ablation: compare control/regime detection with/without ΔΦ under identical signals and constraints |

### 9.3 Algorithm: simulation protocol

```
Algorithm 1 — FEEN Evaluation Protocol (Simulation)
─────────────────────────────────────────────────────────────────────
Require: Graph G, parameters {Ωᵢ, γᵢ, κᵢⱼ}, noise level,
         kernel choice (optional)

 1: Initialize a(0) [or θᵢ(0)], set t = 0
 2: for t = 0 to T do
 3:     Integrate coupled-mode Eq. (1) [or phase Eq. (5)]
 4:     if memory enabled then
 5:         Integrate kernel dynamics Eq. (7) or embedded form Eq. (9)
 6:     end if
 7:     Compute R(t) via Eq. (6) and synchronization error σ_θ(t)
 8:     if observer enabled then
 9:         Compute ΔΦ[y](t) via Eq. (10)
10:     end if
11: end for
12: Estimate Tᶜ, Eₒₚ, robustness curves vs. disorder/noise
─────────────────────────────────────────────────────────────────────
```

---

## 10 Falsification Program and Null Hypotheses

> **Reviewer-facing note.** Items labeled as "Prediction" are explicitly framed as falsifiable signatures. No performance advantage is assumed without measurement; the contribution is the end-to-end model plus a validation program that rules out common artifact classes (hidden drives, parameter drift, finite-bath memory).

### 10.1 Core falsifiable signatures

The architecture becomes scientifically meaningful if at least one of the following is reproducibly observed:

1. **Drive-free sidebands:** spectral sidebands emerge without external periodic drive, attributable to network-internal coupling dynamics.
2. **Coherence plateau:** R(t) exhibits a stable plateau over a parameter range where standard models would predict decay.
3. **Topology-dependent phase offsets:** stable offsets locked to graph topology (e.g., cycles) persist under perturbations.
4. **Reset-resistant memory:** history-dependent behavior persists under interventions designed to erase bath correlations.

### 10.2 Null hypotheses (what must be ruled out)

```
┌──────────────────────────────────────────────────────────────────────┐
│                     Null Hypothesis Decision Tree                    │
└──────────────────────────────────────────────────────────────────────┘

 Observed anomalous behavior
          │
          ├──▶ H₀ (Finite bath): Are memory effects reproduced by a
          │         finite engineered bath model?
          │         YES → H₀ supported; not intrinsic memory
          │         NO  → proceed ↓
          │
          ├──▶ H₁ (Drive artifact): Do sidebands/coherence arise from
          │         hidden external periodicities or measurement artifacts?
          │         YES → H₁ supported; not emergent
          │         NO  → proceed ↓
          │
          └──▶ H₂ (Classical reduction): Is all behavior captured by
                    Markovian oscillator networks with parameter drift?
                    YES → H₂ supported; no intrinsic memory needed
                    NO  → FEEN + intrinsic memory supported ✓
```

FEEN + intrinsic memory is supported **only if** these null hypotheses fail under controlled tests.

---

## 11 Experimental Roadmap (Prototype)

### 11.1 Minimal hardware concept

A minimal prototype can be implemented using a small (**N ~ 8–64**) resonator array with programmable couplings (e.g., via tunable mechanical links, piezoelectric coupling, or electronic feedback emulating coupling). Key observables are phase, frequency, and spectrum at each node.

### 11.2 Measurement plan

Measure:
- Node phases **θᵢ(t)** and compute **R(t)**
- Spectra for sidebands and topology-dependent features
- Coherence time **Tᶜ** under controlled perturbations
- Energy dissipation and scaling vs. **N** and coupling strength

---

## 12 Discussion

### 12.1 What is genuinely new?

Synchronization is known; the value of FEEN is a complete, falsifiable, engineering-facing formulation for a clockless mesh and a pathway to distinguish intrinsic memory from bath memory using explicit ablation tests.

### 12.2 Limitations

- Any claim of superiority (energy, robustness) must be demonstrated experimentally.
- The intrinsic memory kernel must be constrained by data; otherwise it remains a modeling ansatz.

---

## 13 Conclusion

We provided a rigorous, testable formulation of FEEN as a phononic mesh network without a central clock. Starting from coupled-mode dynamics, we derived a phase reduction yielding an emergent time reference through synchronization, extended the model to non-Markovian memory kernels, and proposed a falsification program with null hypotheses and measurable signatures. This establishes a concrete route from concept to prototype and to peer-review evaluation.

---

## References

[1] Y. Kuramoto, *Self-entrainment of a population of coupled non-linear oscillators*, in International Symposium on Mathematical Problems in Theoretical Physics, Lecture Notes in Physics 39, Springer (1975).

[2] S. H. Strogatz, *From Kuramoto to Crawford: exploring the onset of synchronization in populations of coupled oscillators*, Physica D **143**, 1–20 (2000).

[3] J. A. Acebrón et al., *The Kuramoto model: A simple paradigm for synchronization phenomena*, Rev. Mod. Phys. **77**, 137–185 (2005).

[4] W. Suh, Z. Wang, and S. Fan, *Temporal coupled-mode theory and the Fano resonance in optical resonators* (classic coupled-mode formalism; cite your preferred standard reference).

[5] G. Tanaka et al., *Recent advances in physical reservoir computing: A review*, Neural Networks **115**, 100–123 (2019).

[6] H.-P. Breuer, E.-M. Laine, J. Piilo, and B. Vacchini, *Colloquium: Non-Markovian dynamics in open quantum systems*, Rev. Mod. Phys. **88**, 021002 (2016).

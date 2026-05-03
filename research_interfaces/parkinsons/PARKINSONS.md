# Domain Parkinson's — Research Interface
`research_interfaces/domain_parkinsons.py`

---

## Overview
`DomainParkinsons` is a computational research module that models Parkinson's Disease (PD) as a **topological wave-state failure** within the FEEN (Frequency-Encoded Elastic Network) framework. 

Rather than treating PD as a simple neurotransmitter deficiency (the classical dopamine-depletion model), this module represents the pathological state as a breakdown in **neuro-coherence**—a failure of phase synchronization and thermodynamic efficiency across the basal ganglia motor loop. This perspective aligns with emerging electrophysiological research showing that pathological **beta-band oscillations** (~13–30 Hz) in the subthalamic nucleus and striatum are a reliable biomarker of motor dysfunction in PD patients.

The module also models a hypothetical therapeutic agent called an **Adaptive Coherence Modulator (ACM)**—a state-dependent scaffold designed to *restore* coherence rather than simply suppress symptoms, with the explicit long-term goal of **pharmacological obsolescence** (i.e., the scaffold enables enough neuroplastic recovery that the patient no longer needs it).

---

## Biological Mapping

| FEEN Variable | Symbol | Biological Correlate |
| :--- | :--- | :--- |
| Regional Variance | $\Delta_{GR}$ | Pathological beta-band oscillation amplitude (tremor intensity) |
| Phase Alignment | $\Lambda$ | Synchronization between the Substantia Nigra pars compacta (SNpc) and Striatum |
| Thermodynamic Stability | $\Theta$ | Mitochondrial ATP synthesis efficiency in dopaminergic neurons |
| Adaptive Gain | $\Gamma$ | Neuroplastic responsiveness of the motor circuit (long-term potentiation capacity) |
| External Influence | $\Phi$ | Active dosage / presence of the ACM scaffold |
| Neuro-Coherence Function | $M$ | Total motor-loop stability—the primary output metric |

---

## The Neuro-Coherence Function $M$

$$M=\frac{\Lambda\cdot\Theta\cdot(1+\Gamma)}{1+\Delta_{GR}}$$

$M$ is the central readout of system health. A high $M$ means the motor loop is stable, synchronized, and energetically efficient. A low $M$ reflects the Parkinsonian state: high variance (tremor), poor synchronization, depleted ATP, and minimal neuroplastic reserve. 

The denominator $(1+\Delta_{GR})$ acts as a **coherence penalty**—even small increases in beta-band oscillatory noise strongly suppress the overall system score, which matches the outsized clinical impact that even mild tremor can have on voluntary motor control.

### Baseline vs. Target Values

| State | $\Lambda$ | $\Theta$ | $\Gamma$ | $\Delta_{GR}$ | $M$ (approx.) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Parkinsonian (initial) | 0.15 | 0.20 | 0.05 | 0.85 | ~0.017 |
| Post-ACM target | $\ge0.40$ | $\ge0.40$ | $\ge0.50$ | $\le0.30$ | $>0.085$ |

The `verify_systemic_independence` method uses a threshold of $5\times\text{baseline\_M}\approx0.085$ together with $\Lambda\ge0.4$ to determine whether the intervention was a genuine ACM or merely a **conventional suppressor**.

---

## The Three-Phase ACM Protocol

### Phase 1 — Variance Damper (`apply_phase1_scaffold`)

$$\Delta_{GR}\leftarrow\Delta_{GR}\cdot\exp(-0.8\cdot\Phi)$$
$$\Theta\leftarrow\max(0.1,\Theta-0.05\cdot\Phi)$$

This phase targets the most debilitating symptom first: the pathological oscillations. Using an **exponential damping** model, even a small dose ($\Phi$) produces a meaningful, non-linear reduction in beta-band variance. The slight reduction in $\Theta$ captures the real metabolic cost of any early-phase pharmacological intervention—mitochondrial resources are transiently diverted. The $\max(0.1,\dots)$ floor prevents thermodynamic collapse.

**Scientific relevance:** This phase mirrors the early mechanism of DBS (Deep Brain Stimulation) or high-frequency oscillatory disruption—both achieve symptom relief by disrupting synchrony rather than by restoring dopamine levels.

---

### Phase 2 — Resonant Tutoring (`apply_phase2_tutoring`)

$$\text{gross\_thermo\_support}=0.6\cdot\Phi$$
$$\text{plasticity\_growth}=0.5\cdot\Phi$$
$$\text{metabolic\_cost}=\text{plasticity\_growth}\cdot0.4$$
$$\Gamma\leftarrow\min(1.0,\Gamma+\text{plasticity\_growth})$$
$$\Theta\leftarrow\min(1.0,\Theta+\text{gross\_thermo\_support}-\text{metabolic\_cost})$$
$$\Lambda\leftarrow\min(1.0,\Lambda+0.4\cdot\Phi)$$

This is the most scientifically substantive phase. It models three simultaneous processes:
1. **Thermodynamic support:** The scaffold actively improves mitochondrial ATP yield, consistent with proposed neuroprotective strategies that target Complex I of the mitochondrial respiratory chain.
2. **Neuroplastic induction:** The ACM drives long-term potentiation in the motor circuit, calibrated against a **metabolic penalty**. This captures the known bioenergetic cost of synaptic remodeling—learning and repair consume ATP. Scientists can vary the $0.4$ coefficient to simulate patients with different baseline metabolic reserves.
3. **Phase re-alignment:** Synchrony between SNpc and Striatum is actively restored, mirroring the effect of dopamine replacement in re-coupling the nigrostriatal pathway.

**Scientific relevance:** Phase 2 is the distinguishing step between an ACM and a conventional dopamine agonist. A classical agonist only boosts $\Lambda$ transiently; the ACM simultaneously builds $\Gamma$—the neuroplastic reserve that will sustain $\Lambda$ after the scaffold is removed.

---

### Phase 3 — Taper (`execute_phase3_taper`)

$$\text{step\_size}=\frac{\Phi}{\text{steps}}$$

For each step:
$$\Phi\leftarrow\Phi-\text{step\_size}$$
$$\text{degradation\_factor}=\max(0.0,1.0-\Gamma)$$
$$\Lambda\leftarrow\max(0.15,\Lambda-0.05\cdot\text{degradation\_factor})$$
$$\Theta\leftarrow\max(0.20,\Theta-0.05\cdot\text{degradation\_factor})$$
$$\Delta_{GR}\leftarrow\min(0.85,\Delta_{GR}+0.1\cdot\text{degradation\_factor})$$

The scaffold is gradually withdrawn. The $\text{degradation\_factor}$ is the critical parameter: it is **inversely proportional to the achieved neuroplastic gain $\Gamma$**. If $\Gamma$ is near $1.0$ (maximum plasticity), the degradation factor is near $0$, and the motor loop sustains itself—the hallmark of a true ACM. If $\Gamma$ is low (the scaffold never induced lasting plasticity), the system regresses toward the Parkinsonian attractor.

**Scientific relevance:** This phase provides a quantitative model for **drug discontinuation protocols**. Researchers can simulate what happens when a patient is tapered off treatment after differing durations of Phase 2, directly reading the post-taper $M$ value to predict relapse risk.

---

## Independence Verification (`verify_systemic_independence`)

$$\text{baseline\_M}\approx0.017$$
$$\text{is\_stable}=(M>5\times\text{baseline\_M})\land(\Lambda\ge0.4)$$

After $\Phi$ reaches zero, this diagnostic determines the **classification of the intervention**:
* **True ACM (returns `True`):** $M>0.085$ and $\Lambda\ge0.4$. The motor loop is stable without external support. The patient achieved durable remission.
* **Conventional Suppressor (returns `False`):** The system has regressed. The intervention was symptom-masking only, and the patient is back near the Parkinsonian attractor.

This binary distinction gives scientists a clean success criterion for in-silico drug candidate screening.

---

## Suggested Research Workflows

### 1. Dose–Response Curves
Call `apply_phase1_scaffold` with a range of $\Phi$ values and record $M$ and $\Delta_{GR}$ at each step to build a computational dose–response curve. Compare the shape (linear vs. exponential saturation) to empirical PD tremor-suppression data.

### 2. Metabolic Sensitivity Analysis
Vary the metabolic penalty coefficient in `apply_phase2_tutoring` (currently $0.4$) to simulate patients with mitochondrial co-morbidities (e.g., Complex I mutations). Measure how the threshold dose of $\Phi$ required to pass `verify_systemic_independence` changes.

### 3. Tapering Protocol Optimization
For a fixed total Phase 2 dose, vary the number of tapering `steps` (fast vs. slow withdrawal) and the inter-phase gap to identify the optimal protocol that maximizes post-taper $M$ while minimizing cumulative $\Phi$ exposure (surrogate for side-effect burden).

### 4. Multi-Run Monte Carlo
Introduce Gaussian noise into the initial Parkinsonian state parameters ($\Lambda$, $\Theta$, $\Gamma$, $\Delta_{GR}$) to represent inter-patient variability, then run the full three-phase protocol across N samples. Use the distribution of final $M$ values and `verify_systemic_independence` pass-rates to characterize the robustness of the ACM protocol.

### 5. Side-by-Side vs. Classical Model
Initialize two instances—one run through the three-phase ACM protocol and one with only Phase 1 repeated indefinitely (chronic suppressor model). Compare their trajectories to quantify the long-term divergence in neuroplasticity $\Gamma$ and thermodynamic stability $\Theta$, illustrating the ACM's advantage over indefinite pharmacological maintenance.

---

## Limitations and Future Extensions
* All coefficients ($0.8, 0.6, 0.5, 0.4$, etc.) are dimensionless proxies. Mapping them to nanomolar concentrations, Hz frequencies, or ATP mol/min values requires experimental calibration.
* The model is deterministic; stochastic extensions (Langevin noise, Poisson spike noise) would better reflect biological variability.
* A future `apply_phase4_consolidation` step could model long-term synaptic homeostasis and immune-mediated pruning following scaffold clearance.
* The scalar $M$ compresses a spatially distributed circuit into one number. A graph-topology extension could represent individual nuclei (SNpc, Striatum, GPi, STN, Thalamus) as nodes with edge-weighted coherence.

# FEEN: A Unified Theory of Phononic Mesh Computing

## — Applications —

**Don Michael Feeney Jr.¹ · Marcel Krüger²**

¹ *Independent Researcher, Pennsylvania, USA*  
² *Independent Researcher, Meiningen, Germany*

ORCID: 0009-0003-1350-4160 │ ORCID: 0009-0002-5709-9729  
dfeen87@gmail.com │ marcelkrueger092@gmail.com

*Preprint · 2026*

---

## I. Introduction

The dominant paradigm in computing rests on a separation that is so familiar it rarely warrants comment: physical signals from the world are sampled, digitized, and handed off to a processor that manipulates discrete symbols according to a global clock. That clock is the organizing principle of modern computation. It enforces synchrony, enables pipelining, and makes correctness proofs tractable. It also makes computation fundamentally discontinuous, power-hungry at high clock rates, and structurally mismatched to the physical environment that most real-world systems must sense and control.

FEEN — Phononic Mesh Computing — is an alternative substrate built around a different organizing principle: resonance. Rather than storing state in flip-flops clocked at gigahertz rates, a FEEN mesh stores state as sustained mechanical oscillation in an array of coupled microresonators. Information is encoded in amplitude, phase, and frequency of these oscillations, manipulated through nonlinear coupling, and read out as a spatial pattern of wave activity across the mesh. There is no global clock. Synchronization, where it is needed, emerges from the dynamics of the mesh itself through Kuramoto-type phase locking — the same mechanism that coordinates firefly flashing and cardiac pacemaker cells.

This paper presents the application landscape for FEEN technology: the domains where the substrate's physical properties create genuine engineering advantages, the prototype targets that are immediately accessible given current MEMS fabrication capabilities, and the limitations and open problems that constrain near-term deployment. It is written as a standalone document, but it draws on a physical theory — governing equations for nonlinear coupled resonators, fading-memory dynamics, Kuramoto synchronization thresholds, and non-Markovian relaxation kernels — that is developed in full in the companion theoretical paper.

FEEN is not a general-purpose replacement for CMOS digital logic. It is a substrate optimized for a specific and well-defined regime: continuous-time, low-frequency (sub-100 Hz to low-kHz), low-power, distributed dynamical computation. The clearest way to characterize that regime is to ask what kinds of signals and tasks the physical world presents at the interface between silicon and environment — and to notice how poorly matched the clocked digital paradigm is to answering them.

Mechanical vibrations, acoustic waves, biological oscillations, seismic motion, and robotic proprioception all share a common character: they are continuous, time-varying, and richly structured in the frequency domain below a few kilohertz. Sensing and processing such signals with CMOS requires analog front-ends, high-rate ADCs, and digital signal processors — layers of transduction that introduce latency, quantization noise, and power overhead. A FEEN mesh couples to these signals directly, as a physical resonator couples to a driving force, and processes them through dynamics that are native to the same frequency range.

Three physical properties of the FEEN substrate make this possible and distinguish it from competing approaches. First, nonlinear coupled-mode dynamics governed by Duffing-type equations produce a high-dimensional state space capable of the nonlinear projections required for temporal pattern recognition — without the training overhead of a neural network. Second, fading memory implemented through Q-factor-controlled exponential decay provides tunable temporal integration windows directly in the physics, spanning milliseconds to hundreds of milliseconds depending on resonator design. Third, Kuramoto-type synchronization among coupled nodes enables clockless distributed coordination, allowing arrays of FEEN nodes to lock phase and share a common time reference without any node serving as master.

These three properties motivate four primary application domains, each grounded directly in the physics: physical reservoir computing for temporal pattern recognition; sensorimotor control for autonomous robotics via central pattern generator (CPG) dynamics; structural health monitoring using non-Markovian memory kernels; and clockless distributed synchronization for GPS-free sensor arrays and fractionated architectures. Each domain is analyzed below in terms of the physical mechanism that makes FEEN advantageous, the engineering barriers that must be overcome, and a concrete prototype target accessible with current MEMS fabrication. A further set of domains — aerospace electronics, geophysical networks, underwater acoustics, medical implants, and power grid monitoring — is treated in Appendix A as physically motivated but not yet prototype-ready.

A note on scope: this paper presents engineering justification and prototype targets, not deployment claims. The synchronization mechanism has not yet been demonstrated in hardware; the reservoir computing prototype has not been benchmarked; the structural sensor node has not been fabricated. What the paper claims is that the physics motivates each application, that the prototype targets are within reach of current fabrication, and that the falsification criteria are clear enough to guide the experimental program. The ambition is a substrate that blurs the boundary between sensor, computer, and actuator — one whose intelligence is not installed but grown from the dynamics of its material.

---

## II. Application Domains

FEEN is not a general-purpose computing substrate. Its value lies in specific physical regimes where its native dynamics produce capabilities that clocked digital architectures cannot match without significant overhead. Each application domain discussed below is selected because it satisfies two criteria: the domain's signal and control requirements lie within FEEN's operational envelope (sub-100 Hz to low-kHz, continuous-time, spatially distributed), and the substrate's physical properties — nonlinearity, fading memory, or emergent synchronization — provide a direct engineering advantage rather than a marginal one. FEEN is not a good fit for applications requiring high clock rates, deep pipelining, or tasks that demand the full expressivity of a trained neural network operating over arbitrary data domains. Those constraints are enumerated alongside the opportunities.

Table 1 summarizes the four primary domains, their physical basis, and their associated prototype targets.

*Table 1. Primary FEEN application domains.*

| **Application Domain** | **FEEN Physical Basis** | **Engineering Relevance** | **Prototype Target** |
|---|---|---|---|
| **Reservoir Computing** | Nonlinear coupled-mode dynamics; Q-factor fading memory | Temporal pattern recognition; spoken digit recognition (TI-46 benchmark) | Physical RC prototype (N = 16–64); MEMS fabrication |
| **Sensorimotor Control (CPG)** | Kuramoto phase locking; continuous-time dynamics | Legged gait coordination; reactive manipulation; sub-100 Hz control loops | CPG locomotion controller for legged microrobots (N = 4–8) |
| **Structural Health Monitoring** | Non-Markovian memory kernel Kᵢ(t); Prony coefficients | Continuous damage-state estimation; vibration-powered sensor nodes | Vibration-powered structural sensor node (N = 8–16) |
| **Distributed Synchronization** | Order parameter R(t), ψ(t); emergent Kuramoto locking | Clockless coordination; GPS-free phase reference; fractionated architectures | Ground-based synchronization testbed (N = 8–32) |

---

### II.I Reservoir Computing and Physical Neural Networks

FEEN is a physical reservoir computer by construction. A reservoir computer consists of a high-dimensional dynamical system — the reservoir — whose internal state is driven by an input signal and read out by a simple trained linear layer. The reservoir itself is not trained; only the readout weights are adjusted. This architecture exploits the reservoir's natural dynamics: nonlinear projections map low-dimensional inputs into a high-dimensional feature space where temporal patterns become linearly separable. FEEN satisfies all three canonical reservoir requirements without additional engineering, because those requirements are direct consequences of its nonlinear coupled-mode physics.

#### Fading Memory

A useful reservoir must have fading memory: past inputs must influence current states, but that influence must decay over time so that the reservoir does not saturate. FEEN resonators implement fading memory through exponential decay of stored energy at rate γ = ω₀/(2Q), with retention time τ = Q/(πf₀). The Q-factor directly controls the memory horizon: Q = 50 provides a 16 ms window suitable for phoneme-level speech processing, while Q = 500 provides a 159 ms window suitable for syllable-level temporal integration. Unlike digital reservoir implementations, which set the memory horizon by architecture, FEEN's memory horizon is a continuously tunable physical parameter.

#### High-Dimensional Nonlinear State Space

The nonlinear coupled-mode equations produce a high-dimensional state space even for modest mesh sizes. For N nodes each described by a complex amplitude aᵢ ∈ ℂ, the mesh state space is 2N-dimensional. Nonlinear amplitude saturation and Duffing frequency shifts produce the nonlinear projections required for temporal feature extraction. Crucially, spectral multiplexing — running multiple frequency channels on each physical node simultaneously — effectively increases the functional dimensionality of the reservoir beyond N without increasing node count.

#### Target Tasks and Comparison with Photonic Reservoirs

Physical reservoir computing is particularly effective for tasks requiring temporal pattern recognition with low latency and low power: spoken digit recognition, electromyographic gesture classification for prosthetic control, anomaly detection in time-series sensor data, and nonlinear equalization in communication channels. FEEN offers three distinct advantages over photonic reservoir substrates for these tasks: operation at mechanical rather than optical frequencies allows direct coupling to acoustic and biological signals without transduction; the Duffing nonlinearity is stronger and more controllable than Kerr optical nonlinearity at equivalent power levels; and MEMS fabrication is more compatible with miniaturization and integration with CMOS readout circuitry than free-space optical setups.

**Prototype target:** an N = 16–64 FEEN mesh with Q-factors tuned across the speech-relevant sub-band (50–300 Hz) serves as a physical reservoir with a trained linear readout implemented in analog circuitry. Performance is benchmarked against the TI-46 spoken digit corpus, providing a direct comparison point with photonic reservoir implementations.

One limitation is inherent to the reservoir computing paradigm: the absence of in-situ training of physical coupling weights means that adaptation to new tasks requires either reconfiguration of the physical mesh or retraining only the linear readout layer. This limits expressivity to linear separability in the reservoir's feature space, which may be insufficient for tasks requiring compositional generalization.

---

### II.II Autonomous Robotics and Low-Frequency Sensorimotor Control

Conventional robotic control architectures separate sensing, computation, and actuation into discrete digital pipeline stages, each synchronized to a global clock and coupled through software abstraction layers. This introduces a fundamental mismatch with the physical environment: the world is continuous, mechanical, and time-varying, whereas the processor output is discrete, bursty, and phase-locked to an internal reference that has no physical relationship to the dynamics being controlled.

FEEN resolves this mismatch by design. Because each resonator node operates as a continuous dynamical system with natural frequency, damping, and phase, a FEEN-based sensorimotor loop can be coupled directly to the mechanical modes of the robot body. Proprioceptive signals — joint angles, inertial measurements, contact forces — are already oscillatory in the sub-100 Hz range; they require no analog-to-digital conversion before entering the phononic mesh as injected amplitudes. This eliminates conversion latency, quantization error, and the power overhead of high-rate sampling clocks. FEEN complements, rather than replaces, high-level digital planning; its role is the continuous-time sensorimotor loop where mechanical bandwidth and phase coherence dominate.

#### Rhythm-Based Locomotion and CPG Implementation

Legged locomotion in animals is governed by spinal central pattern generators (CPGs) — distributed networks of oscillators that produce rhythmic motor commands through mutual entrainment rather than centralized sequencing. FEEN's Kuramoto-type synchronization dynamics provide a direct physical substrate for artificial CPG implementation. The Kuramoto model is not an analogy invoked for motivation but the precise dynamical regime in which the FEEN mesh operates: coupled oscillators with natural frequency spread Δω synchronize above a coupling threshold κλ₂(L) ≳ C·Δω, with a stable collective phase emerging from local interactions alone.

A FEEN mesh of N = 8–32 nodes can embed the phase-locking conditions for multi-limb gait coordination, with gait transitions triggered by changes in coupling strength κ rather than software state machines. Because synchronization emerges from dissipative relaxation, the gait remains stable against external perturbations without requiring an explicit recovery subroutine.

#### Reactive Manipulation and Energy Efficiency

Dexterous manipulation requires sub-millisecond reactive correction to unexpected contact events. FEEN resonators operating at f₀ = 1 kHz exhibit response timescales on the order of τ = Q/(πf₀) ≈ 48 ms for Q = 150 — well within the mechanical bandwidth of compliant end-effectors. Because the FEEN mesh reacts to amplitude perturbations through coupled-mode dynamics rather than interrupt-driven software handlers, the latency between contact detection and corrective actuation is governed by the physical coupling coefficient κᵢⱼ rather than processor scheduling delays.

Battery-powered mobile robots operate under strict energy budgets. FEEN's energy per operation of approximately E_compute ≈ 2 × 10⁻²¹ J is several orders of magnitude below CMOS digital switching energies at comparable logic complexity. Although amplification overhead currently dominates total system energy, this overhead is an engineering constraint rather than a fundamental thermodynamic one, and is expected to diminish as passive phononic coupling substrates mature.

**Prototype target:** a FEEN mesh of N = 4–8 bistable resonators, coupled to piezoelectric leg actuators, implements a stable four- or six-legged gait through passive entrainment. Gait adaptation to terrain requires only a change in coupling weights κᵢⱼ, implementable through a simple analog bias circuit. The primary current constraint is the 100 Hz clock-rate ceiling: applications requiring sensorimotor loops faster than this cannot be addressed by a FEEN architecture without active reset schemes that themselves introduce energy overhead.

---

### II.III Structural Health Monitoring and Experimental Prototypes

Structural health monitoring represents the most immediately accessible prototype opportunity for FEEN hardware, because it requires exactly the combination of properties the substrate provides natively: continuous-time sensing, ultra-low power, non-Markovian memory for detecting slowly evolving damage signatures, and emergent synchronization for clockless distributed arrays — all within the sub-100 Hz frequency range of structural vibration modes.

#### Non-Markovian Memory for Damage Detection

FEEN's non-Markovian memory kernel Kᵢ(t) captures long-tail relaxation signatures that change characteristically when material properties degrade. A FEEN mesh bonded to a structural panel will exhibit shifts in the Prony expansion coefficients {cᵢⱼₗ, λᵢⱼₗ} as cracks propagate, providing a continuously updated mechanical state estimate without dedicated sensing hardware. This is a qualitatively different capability from a simple threshold sensor: the kernel fitting procedure extracts the temporal structure of relaxation — not merely its amplitude — and is therefore sensitive to changes in material microstructure that precede macroscopic failure.

#### Vibration-Powered Sensor Node

The first near-term prototype target is a vibration-powered structural sensor node: an N = 8–16 FEEN mesh bonded to a structural element, harvesting ambient vibration energy through piezoelectric coupling, processing strain signals through the phononic mesh, and transmitting a compressed health state via the emergent order parameter R(t) — all without an external power supply or clock reference. This prototype is within reach of current MEMS fabrication tolerances and directly tests the non-Markovian kernel's damage-detection sensitivity under controlled crack-propagation conditions.

This application extends naturally to aerospace structures. Launch vehicles, spacecraft panels, and deployable mechanisms are subject to fatigue, micro-cracking, and joint degradation that is difficult to detect prior to failure. The qualification of FEEN devices under combined radiation, vibration, and vacuum conditions remains an open engineering item, but the structural sensing function itself can be validated on terrestrial hardware before flight qualification is pursued.

#### Distributed Synchronization as a Validated Capability

A ground-based synchronization testbed of N = 8–32 FEEN nodes connected by programmable coupling links validates the synchronization threshold κλ₂(L) ≳ C·Δω and measures coherence time T₁ as a function of topology and noise injection. This testbed is the most important near-term experimental milestone: it provides the empirical foundation that all distributed FEEN applications — whether in aerospace, robotics, or sensor networks — depend upon. The demonstration is deliberately narrow in scope, claiming only that the synchronization mechanism functions as the theory predicts at laboratory scale.

---

### II.IV Distributed Synchronization: Theory to Demonstration

Clockless distributed synchronization is FEEN's most distinctive capability and the one most in need of experimental grounding. The theoretical picture is as follows: a stable collective phase ψ(t) with order parameter R(t) → R* > 0 arises from local Kuramoto-type coupling alone, without any node designated as master. The synchronization threshold κλ₂(L) ≳ C·Δω — where λ₂(L) is the algebraic connectivity of the coupling graph and Δω is the natural frequency spread — provides a design rule for coupling topology. What the theory does not yet provide is hardware validation.

#### What the Demonstration Must Show

The synchronization demonstrator has three specific objectives:

1. Verify that R(t) converges to R* > 0 above the threshold coupling κ_c and remains disordered for κ < κ_c, as H₀ predicts.
2. Measure coherence time T_c as a function of Q and noise intensity, testing H₁.
3. Demonstrate that phase recovery after deliberate node dropout follows the predicted exponential timescale τ_recover = 1/(κλ₂(L)), testing H₂.

These three measurements together provide sufficient empirical grounding to claim that FEEN's synchronization mechanism is physically real and behaves as the theory predicts.

#### Physical Implementation

A laboratory testbed of N = 8–32 MEMS resonators, connected by tunable analog coupling circuits, replicates the Kuramoto network in hardware. Each node is individually addressable for frequency detuning Δωᵢ and coupling weight κᵢⱼ. Phase readout is achieved through lock-in detection of the carrier amplitude at each node. The testbed is instrumented to record R(t) and ψ(t) at the full sampling rate of the coupling dynamics, enabling direct comparison with stochastic Kuramoto simulations.

The scope of this demonstrator is intentionally narrow: N ≤ 32, bench-top operation, no environmental qualification. It is a theory validation experiment, not a prototype for any specific deployment context. The engineering path from this demonstrator to mission-specific applications (satellite formation flying, distributed arrays, autonomous swarms) is a subject for subsequent work, once the physics is confirmed.

---

### II.V Near-Term Prototype Opportunities

The four application domains above map onto four immediately accessible prototype targets, given practical channel counts up to O(100), clock rates up to 100 Hz, and MEMS fabrication tolerances achievable at the 10–100 channel scale.

**(i) Physical Reservoir for Spoken Digit Recognition.** A FEEN mesh of N = 16–64 resonators with Q-factors tuned across the speech-relevant sub-band (50–300 Hz) serves as a physical reservoir with a trained linear readout implemented in analog circuitry. Performance is benchmarked against the TI-46 spoken digit corpus.

**(ii) CPG-Based Locomotion Controller for Legged Microrobots.** A FEEN mesh of N = 4–8 bistable resonators, coupled to piezoelectric leg actuators, implements a stable four- or six-legged gait through passive entrainment. Gait adaptation to terrain requires only a change in coupling weights κᵢⱼ, implementable through a simple analog bias circuit.

**(iii) Vibration-Powered Structural Sensor Node.** An N = 8–16 FEEN mesh bonded to a structural element harvests ambient vibration energy through piezoelectric coupling and transmits a compressed health state via R(t) without an external power supply or clock reference. The non-Markovian memory kernel provides damage-detection sensitivity that a threshold sensor cannot replicate.

**(iv) Distributed Synchronization Testbed.** A bench-top testbed of N = 8–32 FEEN nodes validates κλ₂(L) ≳ C·Δω and measures coherence time T_c as a function of topology and noise. This is the foundational experimental milestone for all distributed FEEN applications.

Across all four prototypes, the compiler toolchain — which maps a wave program to a physical mesh with fabrication-induced frequency dispersion, parasitic couplings, and temperature-dependent parameter drift — remains unvalidated against real hardware targets. A feedback-based calibration loop is a prerequisite for any of the prototype targets above.

---

### II.VI Application-Specific Limitations and Open Problems

Not all application domains are equally accessible under current fabrication and engineering constraints.

For **reservoir computing**, the absence of in-situ training of physical coupling weights means adaptation to new tasks requires reconfiguration of the physical mesh or retraining only the linear readout layer. Training occurs only in the linear readout layer; the FEEN reservoir itself remains fixed, as in standard physical reservoir computing. This limits expressivity to linear separability in the reservoir's feature space, which may be insufficient for tasks requiring compositional generalization.

For **robotics and real-time control**, the primary constraint is the 100 Hz clock-rate ceiling. Applications requiring sensorimotor loops faster than this cannot currently be addressed by FEEN without active reset schemes that introduce energy overhead. The maximum pipeline depth limits in-substrate computation complexity; tasks requiring deep decision trees must be offloaded to a conventional co-processor, making FEEN a reflex layer rather than a complete control system.

For **structural health monitoring**, the non-Markovian kernel fitting procedure assumes stationarity of the baseline relaxation structure. In environments with time-varying ambient vibration spectra — operating machinery, variable-load structures — baseline drift may confound damage signatures. The vibration-powered prototype also requires that ambient structural vibration at the target site is sufficient to sustain resonator oscillation, a site-qualification requirement not addressed in the current design.

For **distributed synchronization**, the threshold condition κλ₂(L) ≳ C·Δω depends on the algebraic connectivity λ₂(L) of the coupling graph, which in a physical deployment is determined by the physical coupling medium and may not be independently controllable. Engineering the coupling topology to meet the threshold under realistic constraints — link asymmetry, delay, and intermittent connectivity — requires hardware experiments that the laboratory testbed is designed to provide.

---

## Appendix A: Additional Application Domains

The following application domains are physically motivated but involve engineering barriers that are not yet resolved or regulatory requirements that have not been assessed. They are presented here for completeness and to support future research directions. No claims of near-term prototype readiness are made for any domain in this appendix.

Table A1 provides a compact summary of the physical basis and key barriers for each domain.

*Table A1. Additional application domains: physical basis and engineering barriers.*

| **Domain** | **Physical Basis** | **Engineering Barriers & Caveats** |
|---|---|---|
| **Satellite Systems & Aerospace** | Radiation hardness by architecture (bistable barrier ΔU ≫ k_B T); clockless distributed synchronization; structural health monitoring via non-Markovian kernel drift. | Qualification of MEMS resonators under combined radiation + vibration + vacuum has not been demonstrated. Temperature stabilization (ΔT < 0.1 K) demands thermal control hardware. Heavy-ion SEU immunity requires experimental verification. |
| **Seismic & Geophysical Networks** | GPS-free array synchronization via Kuramoto locking; long-tail precursor detection via Prony kernel; planetary and borehole deployment compatibility. | GPS-free synchronization has not been demonstrated in a geophysical deployment. Extraterrestrial qualification is an additional engineering barrier beyond current scope. |
| **Underwater Acoustics & AUVs** | Native phononic medium; emergent synchronization for acoustic arrays; lateral-line sensorimotor control. | Waterproofing and pressure tolerance are engineering requirements not yet addressed. Competitive against existing acoustic modems only with demonstrated integration. |
| **Medical Implants** | Sub-100 Hz bio-signal compatibility; ultra-low power front-end; bistable hysteresis for cardiac sensing; cochlear filter bank. | FDA Class III regulatory pathway not assessed. Frequency ceiling limits cochlear applicability without engineering advancement. Requires in-vivo validation before any clinical claims. |
| **Power Grid Monitoring (WAMS)** | 50/60 Hz operation; GPS-free phasor coherence; non-Markovian inter-area oscillation detection. | Grid fault isolation circuitry not yet designed. No prototype demonstrated against PMU standards. Coupling to live grid infrastructure is a significant safety engineering challenge. |

---

### A.I Satellite Systems and Aerospace Electronics

The aerospace operating environment presents constraints collectively hostile to conventional digital electronics and simultaneously compatible with FEEN's physical properties: ionizing radiation, extreme thermal cycling, and the mass-power-reliability trade-off of deep-space missions.

#### Radiation Hardness by Architecture

Single-event upsets (SEUs) are a primary reliability concern for semiconductor digital logic in space. FEEN's oscillation-based state storage changes the failure mode fundamentally: a single high-energy particle deposits a perturbation in amplitude or phase rather than a permanent logic inversion. In the monostable regime, the energy perturbation decays exponentially at rate γ and the resonator returns to its natural steady state. In the bistable regime, the energy barrier ΔU = ω₀⁴/(4|β|) must be exceeded to flip the stored bit, yielding upset immunity that scales with ΔU/k_BT. This argument is physically sound but requires experimental verification under representative heavy-ion fluence before any radiation-hardness claim can be made in a qualification context.

#### Clockless Operation in Distributed Spacecraft

Distributed space architectures — fractionated satellites, formation-flying constellations, lunar surface networks — require inter-node synchronization without a broadcast clock. FEEN's emergent synchronization provides exactly this capability: a stable collective phase ψ(t) arises from local coupling alone, without any node designated as master. The threshold κλ₂(L) ≳ C·Δω can in principle be satisfied by low-bandwidth inter-satellite links. Engineering translation to flight hardware requires qualification under combined radiation, vibration, and vacuum conditions that have not yet been demonstrated for FEEN-class MEMS devices.

---

### A.II Seismic and Geophysical Sensor Networks

Seismic P-waves and S-waves have characteristic frequencies spanning 0.01–100 Hz — entirely within FEEN's operational range. Distributed seismic networks currently rely on GPS-disciplined timing at each station; in environments where GPS is unavailable (deep boreholes, extraterrestrial seismometers, underwater arrays), synchronization is a fundamental unresolved problem. FEEN's emergent synchronization and non-Markovian kernel provide physically motivated solutions to both timing and precursor-detection problems.

Seismic precursor signals — slow-slip events, tremor, and b-value anomalies preceding major earthquakes — are characterized by non-exponential relaxation signatures stretching over hours to days. These are precisely the class of signal for which the non-Markovian memory kernel provides unique sensitivity. A FEEN mesh whose kernel Kᵢ(t) has been fitted to the baseline autocorrelation structure of ambient seismic noise will exhibit measurable changes in its Prony coefficients as the local stress field evolves.

**Near-term prototype concept (deferred):** three to eight FEEN nodes deployed at known separations on a passive seismic array, coupled by acoustic ground transmission, demonstrate emergent phase locking without GPS. Phase residuals measured against GPS reference quantify synchronization accuracy under realistic geophysical noise. This experiment is deferred pending validation of the laboratory synchronization testbed described in Section II.IV.

---

### A.III Underwater Acoustics and Autonomous Underwater Vehicles

The underwater domain is already phononic: acoustic waves are the primary information carrier, and electromagnetic signals attenuate rapidly in seawater. FEEN's mechanical wave substrate operates natively in the same physical medium as the signals it must interpret. Lateral-line inspired flow sensing in fish, which detects water motion through mechanoreceptive hair cells operating at sub-100 Hz, provides a biological existence proof for FEEN-class sensorimotor architectures in underwater vehicles.

The alignment with FEEN's physics is genuine, but the engineering barriers are significant: pressure tolerance, waterproofing of MEMS packaging, and interface design to underwater acoustic transducers have not been addressed. These are solvable problems in principle, but they require dedicated engineering effort outside the scope of current FEEN development. This domain is best approached after the terrestrial sensorimotor control prototype has been demonstrated.

---

### A.IV Medical Implants and Chronic Bioelectronic Devices

Implantable medical devices — cardiac pacemakers, cochlear implants, spinal cord stimulators, and deep brain stimulators — share engineering requirements that map directly onto FEEN's physical characteristics: ultra-low power consumption over decade-long operational lifetimes, reliable operation in a biochemically active environment, and processing of oscillatory biosignals in the sub-100 Hz range. FEEN's thermodynamic energy floor, bistable hysteresis for cardiac sensing, and Lorentzian filter bank for cochlear processing are all physically justified.

Any implantable FEEN-based architecture would require compliance with existing regulatory frameworks — ISO 14708, FDA Class III device requirements — which impose clinical validation requirements not yet assessed. The frequency ceiling is a fundamental constraint for cochlear and high-gamma neural applications: reaching 8 kHz for cochlear processing or 300 Hz for high-gamma BCI requires Q-factors and fabrication tolerances beyond the currently feasible regime. Regulatory pathways represent a significant non-technical barrier to clinical deployment and are not addressed in this paper.

---

### A.V Power Grid Monitoring and Wide-Area Measurement Systems

Modern electrical power grids operate at 50 or 60 Hz and their harmonics — frequencies within FEEN's operational range. Wide-area monitoring systems (WAMS) deploy phasor measurement units (PMUs) across transmission networks to detect fault signatures, inter-area oscillation modes, and frequency deviations. Current PMUs require GPS-disciplined clocks; FEEN's emergent synchronization could in principle provide a GPS-free alternative, and its non-Markovian memory kernel provides sensitivity to the 0.1–2 Hz inter-area oscillation modes that are a documented source of large-scale blackout risk.

The coupling mechanism between FEEN nodes and the grid must survive the full range of fault conditions — voltage sags, frequency excursions, short-circuit transients — without damaging the phononic substrate. This requires isolation circuitry that has not yet been designed. Deployment in live grid infrastructure also requires safety certification processes well beyond the scope of current development. This domain is included for its physical motivation but is not a near-term target.

---

## AI Use Disclosure

*The authors acknowledge the use of Claude (Anthropic) for assistance in drafting portions of this paper. All technical claims, mathematical derivations, and physical models remain the intellectual work of the authors and were reviewed and validated by them prior to inclusion.*

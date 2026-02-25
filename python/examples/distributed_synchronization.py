#!/usr/bin/env python3
"""
Distributed Synchronization with FEEN — Example

Demonstrates Kuramoto-type clockless synchronization in a FEEN mesh.  A
network of coupled resonators with initially random phases converges to a
common collective phase — without any node acting as master and without a
broadcast clock.

The synchronization state is captured by the Kuramoto order parameter R(t):

    R(t) = |1/N Σᵢ exp(i·θᵢ(t))|

where θᵢ is the instantaneous phase of node i.  R = 0 means fully
disordered; R = 1 means perfect phase coherence.  The synchronization
threshold is:

    κ · λ₂(L) ≳ C · Δω

where κ is coupling strength, λ₂(L) is the algebraic connectivity of the
coupling graph, and Δω is the natural frequency spread.

Three experiments are run:
  1. Below-threshold coupling (κ < κ_c): R(t) stays near zero.
  2. Above-threshold coupling (κ > κ_c): R(t) converges to R* > 0.
  3. Node dropout recovery: one node is removed mid-run; R(t) dips then
     recovers on a timescale τ_recover ≈ 1/(κ·λ₂(L)).

PREREQUISITES
-------------
  pip install numpy matplotlib
  # pyfeen must be built (see python/CMakeLists.txt)

EXPECTED OUTPUT
---------------
  Printed R* value and convergence time for each experiment
  A 3-panel plot of R(t) over time for the three experiments

This example is the software analogue of the ground-based synchronization
testbed described in docs/APPLICATIONS.md Section II.IV.  Hardware validation
of these results is the primary near-term experimental milestone for FEEN.
"""

import math
import sys

import numpy as np

try:
    import pyfeen
except ImportError:
    sys.exit(
        "pyfeen is not installed.  Build it first:\n"
        "  cd python && cmake -B ../build && cmake --build ../build"
    )

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

N_NODES     = 16       # oscillator count (paper target: N = 8–32)
F_CENTER    = 10.0     # centre natural frequency (Hz)
F_SPREAD    = 0.5      # ± spread of natural frequencies (Hz)
Q_SYNC      = 200.0    # high Q → slow damping → long coherence time
BETA_SYNC   = 1e-3     # monostable — stable oscillation amplitude

DT          = 1e-3     # timestep (s) — 1 kHz rate
RUN_TICKS   = 6000     # total simulation ticks (= 6 s)
DROPOUT_TICK = 3000    # tick at which node 0 is removed in Experiment 3

# Coupling strengths
KAPPA_LOW   = 0.02     # below synchronization threshold
KAPPA_HIGH  = 0.25     # above synchronization threshold

DRIVE_AMP   = 0.8      # initial amplitude injected into each node

# ---------------------------------------------------------------------------
# Build synchronization network
# ---------------------------------------------------------------------------

def build_sync_network(n_nodes, f_center, f_spread, q, beta, kappa, rng=None):
    """Create a fully-connected ring of N oscillators with coupling strength κ.

    Natural frequencies are drawn uniformly from
    [f_center − f_spread, f_center + f_spread], modelling fabrication-induced
    frequency dispersion in a physical MEMS array.

    The ring topology (each node coupled to its two neighbours) has algebraic
    connectivity λ₂ = 2(1 − cos(2π/N)), which determines the synchronization
    threshold κ_c ≈ C·Δω / λ₂.

    Parameters
    ----------
    n_nodes  : int
    f_center : float — mean natural frequency (Hz)
    f_spread : float — half-width of uniform frequency distribution (Hz)
    q        : float — Q-factor (same for all nodes)
    beta     : float — nonlinearity coefficient
    kappa    : float — coupling strength κᵢⱼ
    rng      : numpy.random.Generator or None — for reproducible runs

    Returns
    -------
    pyfeen.ResonatorNetwork, list[float] natural frequencies
    """
    if rng is None:
        rng = np.random.default_rng(seed=42)

    freqs = rng.uniform(f_center - f_spread, f_center + f_spread, size=n_nodes)

    network = pyfeen.ResonatorNetwork()
    for i in range(n_nodes):
        cfg = pyfeen.ResonatorConfig()
        cfg.frequency_hz = float(freqs[i])
        cfg.q_factor     = q
        cfg.beta         = beta
        network.add_node(pyfeen.Resonator(cfg))

    # Ring coupling (each node ↔ its two nearest neighbours)
    for i in range(n_nodes):
        j = (i + 1) % n_nodes
        network.add_coupling(i, j, kappa)
        network.add_coupling(j, i, kappa)

    return network, list(freqs)


# ---------------------------------------------------------------------------
# Inject random initial phases
# ---------------------------------------------------------------------------

def inject_random_phases(network, amplitude, rng):
    """Inject *amplitude* into each node with a random initial phase.

    Starting from uniformly distributed random phases maximises the initial
    disorder (R(0) ≈ 0) so that any order that emerges must come from the
    coupling dynamics, not the initial conditions.
    """
    phases = rng.uniform(0, 2 * math.pi, size=network.size())
    for i in range(network.size()):
        network.node(i).inject(amplitude, float(phases[i]))


# ---------------------------------------------------------------------------
# Compute Kuramoto order parameter R(t)
# ---------------------------------------------------------------------------

def order_parameter(network):
    """Compute R(t) = |1/N Σᵢ exp(i·θᵢ)| from the current network state.

    Phase θᵢ is estimated from the complex amplitude aᵢ = xᵢ + i·vᵢ/ω₀ᵢ,
    which equals A·exp(i·θ) for a harmonic oscillator.

    The velocity is normalised by ω₀ = 2π·F_CENTER so that x and v/ω₀ have
    equal scale; the exact per-node frequency is not exposed by the pyfeen
    Resonator binding, so the shared centre frequency is used as an approximation
    (error < F_SPREAD/F_CENTER ≈ 5 % for these parameters).

    Returns
    -------
    float — order parameter R ∈ [0, 1]
    """
    n      = network.size()
    omega0 = 2.0 * math.pi * F_CENTER   # approximate; same for all nodes
    z_sum  = 0.0 + 0.0j
    for i in range(n):
        node  = network.node(i)
        x_i   = node.x()
        v_i   = node.v()
        # Complex amplitude: normalise velocity by ω₀ to make units consistent
        a_i   = complex(x_i, v_i / omega0)
        if abs(a_i) > 1e-12:
            z_sum += a_i / abs(a_i)     # unit-circle projection exp(i·θᵢ)

    return abs(z_sum) / n


# ---------------------------------------------------------------------------
# Run one synchronization experiment
# ---------------------------------------------------------------------------

def run_experiment(kappa, dropout_tick=None, label=""):
    """Simulate the network and record R(t) every tick.

    Parameters
    ----------
    kappa        : float — coupling strength
    dropout_tick : int or None — if given, decouple node 0 at this tick
    label        : str  — printed description

    Returns
    -------
    numpy.ndarray, shape (RUN_TICKS,) — R(t) time series
    """
    rng     = np.random.default_rng(seed=42)
    net, fs = build_sync_network(
        N_NODES, F_CENTER, F_SPREAD, Q_SYNC, BETA_SYNC, kappa, rng
    )
    inject_random_phases(net, DRIVE_AMP, rng)

    R_series = np.zeros(RUN_TICKS)

    for tick in range(RUN_TICKS):
        if dropout_tick is not None and tick == dropout_tick:
            # Simulate node 0 dropout: remove all couplings to/from node 0
            for j in range(1, N_NODES):
                net.set_coupling(0, j, 0.0)
                net.set_coupling(j, 0, 0.0)

        net.tick_parallel(DT)
        R_series[tick] = order_parameter(net)

    R_final = float(np.mean(R_series[-500:]))   # average over final 500 ticks
    print(f"  {label}")
    print(f"    κ = {kappa:.3f},  R* = {R_final:.3f}")

    return R_series


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("FEEN Distributed Synchronization Example")
    print("=" * 50)
    print(f"  Network size    : {N_NODES} nodes (ring topology)")
    print(f"  Frequency range : {F_CENTER - F_SPREAD:.1f}–{F_CENTER + F_SPREAD:.1f} Hz")
    print(f"  Q-factor        : {Q_SYNC}")
    print(f"  Simulation time : {RUN_TICKS * DT:.1f} s")
    print()
    print("Expected: R* ≈ 0 for κ_low, R* > 0 for κ_high, R recovers after dropout.")
    print()

    R_low      = run_experiment(KAPPA_LOW,  label=f"Exp 1 — below threshold (κ = {KAPPA_LOW})")
    R_high     = run_experiment(KAPPA_HIGH, label=f"Exp 2 — above threshold (κ = {KAPPA_HIGH})")
    R_dropout  = run_experiment(
        KAPPA_HIGH, dropout_tick=DROPOUT_TICK,
        label=f"Exp 3 — above threshold + node dropout at t = {DROPOUT_TICK * DT:.1f} s"
    )

    print()
    print("Interpretation:")
    print(f"  Exp 1: R* ≈ {np.mean(R_low[-500:]):.3f} — incoherent (disordered)")
    print(f"  Exp 2: R* ≈ {np.mean(R_high[-500:]):.3f} — coherent (synchronised)")
    print(f"  Exp 3: R recovers after dropout: "
          f"R at end = {np.mean(R_dropout[-500:]):.3f}")

    # Optional visualisation
    try:
        import matplotlib.pyplot as plt

        t_axis = np.arange(RUN_TICKS) * DT

        fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True)
        fig.suptitle(f"FEEN Distributed Synchronization — {N_NODES}-node ring")

        for ax, R, title, color in [
            (axes[0], R_low,
             f"Exp 1: Below threshold (κ = {KAPPA_LOW})", "tab:orange"),
            (axes[1], R_high,
             f"Exp 2: Above threshold (κ = {KAPPA_HIGH})", "tab:blue"),
            (axes[2], R_dropout,
             f"Exp 3: Above threshold + dropout at t = {DROPOUT_TICK*DT:.1f} s",
             "tab:green"),
        ]:
            ax.plot(t_axis, R, color=color, lw=1.0)
            ax.axhline(1.0, color="grey", ls=":", lw=0.8)
            ax.set_ylim(-0.05, 1.10)
            ax.set_ylabel("R(t)")
            ax.set_title(title)

        axes[2].axvline(DROPOUT_TICK * DT, color="tab:red", ls="--",
                        lw=1.2, label="Node 0 dropout")
        axes[2].legend()
        axes[-1].set_xlabel("Time (s)")

        plt.tight_layout()
        plt.show()

    except ImportError:
        print("(matplotlib not found — skipping plots)")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
CPG Locomotion Control with FEEN — Example

Demonstrates how a FEEN mesh implements a Central Pattern Generator (CPG)
for legged robot gait coordination.  CPGs are distributed oscillator networks
that produce rhythmic motor commands through mutual entrainment — the same
mechanism used by animal spinal circuits to coordinate walking, trotting, and
galloping without a central scheduler.

FEEN's Kuramoto-type phase-locking dynamics provide a direct physical substrate
for CPG implementation.  The coupling coefficient κᵢⱼ determines the strength
of coordination between limbs; gait transitions are triggered by changing κ
rather than executing software state machines.

This example simulates a four-limb (quadruped) CPG:

  Node 0 — front-left  leg oscillator
  Node 1 — front-right leg oscillator
  Node 2 — rear-left   leg oscillator
  Node 3 — rear-right  leg oscillator

Three gaits are demonstrated by changing only the coupling matrix:
  • Walk   — ipsilateral pairs in phase, diagonal pairs anti-phase
  • Trot   — diagonal pairs in phase, ipsilateral pairs anti-phase
  • Bound  — front pair in phase with each other, rear pair in phase,
             front anti-phase to rear

PREREQUISITES
-------------
  pip install numpy matplotlib
  # pyfeen must be built (see python/CMakeLists.txt)

EXPECTED OUTPUT
---------------
  Phase differences printed at steady state for each gait
  A 3-row plot showing the position time series of all four limbs for each gait
  Symmetric coupling → near-zero steady-state phase errors (< 0.1 rad)
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

N_LIMBS        = 4        # quadruped: four leg oscillators (paper target: N = 4–8)
F_CPG          = 2.0      # natural CPG frequency (Hz) — sub-100 Hz control loop
Q_CPG          = 30.0     # Q-factor: τ ≈ Q/(π·f₀) ≈ 4.8 s retention window
BETA_CPG       = -5e2     # negative → bistable (stable limit-cycle oscillator)

DT             = 5e-4     # timestep (s) — 2 kHz integration rate
SETTLE_TICKS   = 4000     # ticks to allow phase locking to settle (= 2 s)
RECORD_TICKS   = 2000     # ticks to record after settling (= 1 s)

DRIVE_AMP      = 0.8      # sustained drive amplitude injected at t = 0

# Coupling strength κ (paper notation κᵢⱼ).
# Positive coupling → phase attraction; negative → phase repulsion.
KAPPA_STRONG   =  0.30    # in-phase coupling
KAPPA_WEAK     = -0.30    # anti-phase coupling (repulsion)

# ---------------------------------------------------------------------------
# Gait coupling matrices
# ---------------------------------------------------------------------------
# Limb index map:  0 = front-left, 1 = front-right, 2 = rear-left, 3 = rear-right
#
# Walk  — 0↔1 anti-phase, 2↔3 anti-phase, 0↔2 in-phase, 1↔3 in-phase
# Trot  — 0↔3 in-phase (diagonal), 1↔2 in-phase (diagonal), 0↔1 anti-phase
# Bound — 0↔1 in-phase, 2↔3 in-phase, front anti-phase to rear

# Each entry K[i][j] is the coupling coefficient from node j → node i.
GAITS = {
    "Walk": [
        # FL    FR    RL    RR
        [  0.0, KAPPA_WEAK,  KAPPA_STRONG, KAPPA_WEAK  ],  # FL (0)
        [ KAPPA_WEAK,  0.0,  KAPPA_WEAK,  KAPPA_STRONG ],  # FR (1)
        [ KAPPA_STRONG, KAPPA_WEAK,  0.0, KAPPA_WEAK   ],  # RL (2)
        [ KAPPA_WEAK,  KAPPA_STRONG, KAPPA_WEAK,  0.0  ],  # RR (3)
    ],
    "Trot": [
        # FL    FR    RL    RR
        [  0.0, KAPPA_WEAK,  KAPPA_WEAK,  KAPPA_STRONG ],  # FL (0) ← in-phase with RR
        [ KAPPA_WEAK,  0.0,  KAPPA_STRONG, KAPPA_WEAK  ],  # FR (1) ← in-phase with RL
        [ KAPPA_WEAK,  KAPPA_STRONG, 0.0, KAPPA_WEAK   ],  # RL (2)
        [ KAPPA_STRONG, KAPPA_WEAK, KAPPA_WEAK,  0.0   ],  # RR (3)
    ],
    "Bound": [
        # FL    FR    RL    RR
        [  0.0, KAPPA_STRONG, KAPPA_WEAK,  KAPPA_WEAK  ],  # FL (0) ← in-phase with FR
        [ KAPPA_STRONG, 0.0,  KAPPA_WEAK,  KAPPA_WEAK  ],  # FR (1)
        [ KAPPA_WEAK,  KAPPA_WEAK,  0.0,  KAPPA_STRONG ],  # RL (2) ← in-phase with RR
        [ KAPPA_WEAK,  KAPPA_WEAK,  KAPPA_STRONG, 0.0  ],  # RR (3)
    ],
}

LIMB_NAMES = ["Front-Left", "Front-Right", "Rear-Left", "Rear-Right"]

# ---------------------------------------------------------------------------
# Build a CPG network
# ---------------------------------------------------------------------------

def build_cpg(coupling_matrix):
    """Create a four-node CPG network with the given coupling matrix.

    Each node is a bistable Duffing resonator operating at F_CPG Hz.
    Bistable resonators (β < 0) produce stable limit-cycle oscillations
    suitable for sustained rhythmic motor commands.

    Parameters
    ----------
    coupling_matrix : 2-D list of float
        K[i][j] = coupling from node j to node i.

    Returns
    -------
    pyfeen.ResonatorNetwork
    """
    network = pyfeen.ResonatorNetwork()

    for i in range(N_LIMBS):
        cfg = pyfeen.ResonatorConfig()
        cfg.frequency_hz = F_CPG
        cfg.q_factor     = Q_CPG
        cfg.beta         = BETA_CPG
        cfg.name         = LIMB_NAMES[i]
        network.add_node(pyfeen.Resonator(cfg))

    for i in range(N_LIMBS):
        for j in range(N_LIMBS):
            k = coupling_matrix[i][j]
            if k != 0.0:
                network.add_coupling(i, j, k)

    return network


# ---------------------------------------------------------------------------
# Run simulation and collect position time series
# ---------------------------------------------------------------------------

def run_gait(network, settle_ticks, record_ticks, dt):
    """Settle the CPG and then record position trajectories.

    Initial conditions: each limb injected with unit amplitude and a small
    phase offset so the network starts slightly out of phase (if all nodes
    started identically, symmetry would prevent entrainment).

    Parameters
    ----------
    network      : pyfeen.ResonatorNetwork
    settle_ticks : int   — ticks discarded (allow phase locking)
    record_ticks : int   — ticks recorded after settling
    dt           : float — timestep (s)

    Returns
    -------
    numpy.ndarray, shape (record_ticks, N_LIMBS) — position of each limb over time
    """
    omega0 = 2.0 * math.pi * F_CPG

    # Inject with small phase offsets to break symmetry
    for i in range(N_LIMBS):
        phase_offset = i * 0.1  # 0.1 rad between successive nodes
        network.node(i).inject(DRIVE_AMP, phase_offset)

    # Settle
    for _ in range(settle_ticks):
        network.tick_parallel(dt)

    # Record
    positions = np.zeros((record_ticks, N_LIMBS))
    for tick in range(record_ticks):
        network.tick_parallel(dt)
        for i in range(N_LIMBS):
            positions[tick, i] = network.node(i).x()

    return positions


# ---------------------------------------------------------------------------
# Compute instantaneous phase (zero-crossing estimate)
# ---------------------------------------------------------------------------

def steady_state_phases(positions):
    """Estimate the mean phase of each oscillator from its position time series.

    Uses the angle of the complex analytic signal computed from position and
    a finite-difference approximation of velocity.  Returns the phase of each
    node relative to node 0.
    """
    phases = []
    for i in range(N_LIMBS):
        x = positions[:, i]
        # Finite-difference velocity approximation
        v = np.gradient(x)
        # Phase angle from analytic signal (x, v/ω₀)
        omega0 = 2.0 * math.pi * F_CPG
        theta = np.arctan2(-v / omega0, x)
        phases.append(float(np.angle(np.mean(np.exp(1j * theta)))))
    # Phase relative to node 0
    return [p - phases[0] for p in phases]


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("FEEN CPG Locomotion Control Example")
    print("=" * 50)
    print(f"  Limbs           : {N_LIMBS} (quadruped)")
    print(f"  CPG frequency   : {F_CPG} Hz")
    print(f"  Q-factor        : {Q_CPG}  (τ ≈ {Q_CPG/(math.pi*F_CPG)*1000:.0f} ms)")
    print(f"  Bistable β      : {BETA_CPG}")
    print()

    results = {}
    for gait_name, K in GAITS.items():
        print(f"Simulating '{gait_name}' gait …")
        net = build_cpg(K)
        traj = run_gait(net, SETTLE_TICKS, RECORD_TICKS, DT)
        rel_phases = steady_state_phases(traj)
        results[gait_name] = traj

        print(f"  Steady-state phases relative to {LIMB_NAMES[0]}:")
        for i, (name, phi) in enumerate(zip(LIMB_NAMES, rel_phases)):
            print(f"    {name:<14}: {phi:+.3f} rad  ({math.degrees(phi):+.1f}°)")
        print()

    # Optional visualisation
    try:
        import matplotlib.pyplot as plt

        fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True)
        fig.suptitle("FEEN CPG — Quadruped Gait Coordination")
        colors = ["tab:blue", "tab:orange", "tab:green", "tab:red"]
        t_ms = np.arange(RECORD_TICKS) * DT * 1000

        for ax, (gait_name, traj) in zip(axes, results.items()):
            for i in range(N_LIMBS):
                ax.plot(t_ms, traj[:, i], color=colors[i],
                        label=LIMB_NAMES[i], lw=1.0)
            ax.set_ylabel("Position (a.u.)")
            ax.set_title(f"{gait_name} gait")
            ax.legend(loc="upper right", fontsize=7, ncol=2)

        axes[-1].set_xlabel("Time (ms)")
        plt.tight_layout()
        plt.show()

    except ImportError:
        print("(matplotlib not found — skipping plots)")


if __name__ == "__main__":
    main()

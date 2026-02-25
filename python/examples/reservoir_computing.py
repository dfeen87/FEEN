#!/usr/bin/env python3
"""
Reservoir Computing with FEEN — Example

Demonstrates how a FEEN mesh acts as a physical reservoir computer for
temporal pattern recognition.  A reservoir computer feeds an input signal into
a high-dimensional dynamical system (the "reservoir") and trains only a simple
linear readout layer.  FEEN satisfies the three canonical reservoir requirements
directly from its physics:

  1. Fading memory — exponential energy decay at rate γ = ω₀/(2Q)
  2. High-dimensional nonlinear state space — N coupled Duffing oscillators
  3. Echo state property — diverse, non-synchronized node responses

This example uses a 16-node FEEN mesh driven by two alternating waveform
classes ("slow" and "fast" sinusoids).  The reservoir state is recorded after
each input burst, and a least-squares linear readout is trained to classify
the two classes.

PREREQUISITES
-------------
  pip install numpy matplotlib
  # pyfeen must be built (see python/CMakeLists.txt)

EXPECTED OUTPUT
---------------
  Node energies printed after each input burst
  Confusion matrix showing classification accuracy printed to stdout
  A 2-panel plot:
    Left  — energy trajectories of all 16 nodes during one burst
    Right — 2-D PCA projection of reservoir states coloured by class label

Typical accuracy on this toy dataset: 90–100 %.
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

N_NODES    = 16        # reservoir size (paper target: 16–64)
F_CENTER   = 100.0     # centre frequency of the reservoir band (Hz)
F_SPREAD   = 5.0       # Hz between adjacent node frequencies
Q_FACTOR   = 50.0      # Q = 50 → τ ≈ 16 ms memory window (speech-relevant)
BETA       = 1e-4      # positive → monostable (analog reservoir node)

DT         = 1e-4      # simulation timestep (s) — 10 kHz sampling rate
BURST_LEN  = 200       # ticks per input burst (= 20 ms)
WASHOUT    = 100       # ticks discarded before recording state (= 10 ms)
N_TRAIN    = 40        # training samples per class
N_TEST     = 10        # test samples per class

# Class 0: low-frequency sinusoidal burst  (~50 Hz)
FREQ_CLASS0 = 50.0
# Class 1: high-frequency sinusoidal burst (~200 Hz)
FREQ_CLASS1 = 200.0

DRIVE_AMPLITUDE = 0.5  # amplitude of injected driving force (arbitrary units)

# ---------------------------------------------------------------------------
# Build the reservoir network
# ---------------------------------------------------------------------------

def build_reservoir(n_nodes, f_center, f_spread, q, beta):
    """Return a ResonatorNetwork of *n_nodes* monostable resonators.

    Frequencies are spread uniformly around *f_center* ± (*n_nodes*/2 × f_spread)
    so that the reservoir covers a broad sub-band rather than clustering at one
    frequency.  Nearest-neighbour coupling is added to enrich cross-node
    dynamics.
    """
    network = pyfeen.ResonatorNetwork()
    for i in range(n_nodes):
        cfg = pyfeen.ResonatorConfig()
        cfg.frequency_hz = f_center + (i - n_nodes / 2) * f_spread
        cfg.q_factor     = q
        cfg.beta         = beta
        cfg.sustain_s    = 0.0   # use default: Q/(π·f₀)
        node = pyfeen.Resonator(cfg)
        network.add_node(node)

    # Nearest-neighbour coupling (ring topology) — promotes diverse responses
    coupling_strength = 0.05
    for i in range(n_nodes):
        j = (i + 1) % n_nodes
        network.add_coupling(i, j, coupling_strength)
        network.add_coupling(j, i, coupling_strength)

    return network


# ---------------------------------------------------------------------------
# Drive and record reservoir state
# ---------------------------------------------------------------------------

def run_burst(network, drive_freq, amplitude, dt, burst_len, washout,
              coupling_strength=0.05):
    """Drive the reservoir with a sinusoidal burst and return the final state.

    Both the external drive and internal coupling forces are applied at every
    step.  Time is tracked locally because ``network.time_s()`` only advances
    when ``tick_parallel()`` is called; here we drive individual nodes manually
    so that we can also pass the external drive force.

    The first *washout* ticks are discarded so the reservoir reaches a forced
    steady state before the state snapshot is taken.  The snapshot is the
    interleaved [x₀, v₀, x₁, v₁, …] vector from ``get_state_vector()``.

    Parameters
    ----------
    network          : pyfeen.ResonatorNetwork
    drive_freq       : float — frequency of the input sinusoid (Hz)
    amplitude        : float — peak amplitude of the driving force
    dt               : float — timestep (s)
    burst_len        : int   — total ticks to simulate
    washout          : int   — ignored (kept for API consistency with callers)
    coupling_strength: float — κᵢⱼ between ring-connected neighbours

    Returns
    -------
    list[float] — reservoir state vector [x₀, v₀, x₁, v₁, …]
    """
    omega_d = 2.0 * math.pi * drive_freq
    n       = network.size()
    t       = 0.0   # local time (not from network.time_s(), which only advances
                    # when tick_parallel() is called)

    for _ in range(burst_len):
        F_ext  = amplitude * math.sin(omega_d * t)

        # Snapshot current displacements for synchronous coupling computation
        x_snap = [network.node(i).x() for i in range(n)]

        for i in range(n):
            # Ring coupling: sum of spring forces from left and right neighbours
            j_l    = (i - 1) % n
            j_r    = (i + 1) % n
            F_coup = coupling_strength * (
                (x_snap[j_l] - x_snap[i]) + (x_snap[j_r] - x_snap[i])
            )
            # tick(dt, F_external, omega_drive, internal_force)
            network.node(i).tick(dt, F_ext, omega_d, F_coup)

        t += dt

    return list(network.get_state_vector())


# ---------------------------------------------------------------------------
# Dataset generation
# ---------------------------------------------------------------------------

def generate_dataset(n_train, n_test, dt, burst_len, washout):
    """Generate training and test datasets.

    Each sample is a reservoir state vector collected after one burst.
    Labels: 0 → slow burst (FREQ_CLASS0), 1 → fast burst (FREQ_CLASS1).

    Returns
    -------
    X_train, y_train, X_test, y_test : numpy arrays
    """
    X, y = [], []

    for label, freq in [(0, FREQ_CLASS0), (1, FREQ_CLASS1)]:
        for _ in range(n_train + n_test):
            net = build_reservoir(N_NODES, F_CENTER, F_SPREAD, Q_FACTOR, BETA)
            state = run_burst(net, freq, DRIVE_AMPLITUDE, dt, burst_len, washout)
            X.append(state)
            y.append(label)

    X = np.array(X, dtype=float)
    y = np.array(y, dtype=int)

    # Interleave classes so splits are balanced
    idx = np.argsort(np.arange(len(y)) % (n_train + n_test) < n_train,
                     stable=True)[::-1]
    X, y = X[idx], y[idx]

    split = 2 * n_train
    return X[:split], y[:split], X[split:], y[split:]


# ---------------------------------------------------------------------------
# Linear readout
# ---------------------------------------------------------------------------

def train_readout(X_train, y_train):
    """Fit a least-squares linear readout.

    Returns weight vector *w* such that sign(X @ w) predicts the class.
    """
    # Binary labels: class 0 → −1, class 1 → +1
    t = np.where(y_train == 0, -1.0, 1.0)
    w, _, _, _ = np.linalg.lstsq(X_train, t, rcond=None)
    return w


def predict(X, w):
    return (X @ w >= 0.0).astype(int)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("FEEN Reservoir Computing Example")
    print("=" * 50)
    print(f"  Reservoir size    : {N_NODES} nodes")
    print(f"  Frequency range   : {F_CENTER - N_NODES/2*F_SPREAD:.0f}"
          f"–{F_CENTER + N_NODES/2*F_SPREAD:.0f} Hz")
    print(f"  Q-factor          : {Q_FACTOR}  (τ ≈ {Q_FACTOR/(math.pi*F_CENTER)*1000:.1f} ms)")
    print(f"  Class 0 / Class 1 : {FREQ_CLASS0:.0f} Hz / {FREQ_CLASS1:.0f} Hz bursts")
    print()

    print("Generating dataset …")
    X_train, y_train, X_test, y_test = generate_dataset(
        N_TRAIN, N_TEST, DT, BURST_LEN, WASHOUT
    )
    print(f"  Training samples  : {len(X_train)}  ({np.sum(y_train==0)} class-0, {np.sum(y_train==1)} class-1)")
    print(f"  Test samples      : {len(X_test)}   ({np.sum(y_test==0)} class-0,  {np.sum(y_test==1)} class-1)")
    print()

    print("Training linear readout …")
    w = train_readout(X_train, y_train)
    train_acc = np.mean(predict(X_train, w) == y_train)
    test_acc  = np.mean(predict(X_test,  w) == y_test)
    print(f"  Train accuracy    : {train_acc*100:.1f} %")
    print(f"  Test  accuracy    : {test_acc*100:.1f} %")
    print()

    # Confusion matrix
    y_pred = predict(X_test, w)
    tp = int(np.sum((y_pred == 1) & (y_test == 1)))
    tn = int(np.sum((y_pred == 0) & (y_test == 0)))
    fp = int(np.sum((y_pred == 1) & (y_test == 0)))
    fn = int(np.sum((y_pred == 0) & (y_test == 1)))
    print("Confusion matrix (test set):")
    print(f"              Pred 0  Pred 1")
    print(f"  Actual 0     {tn:4d}    {fp:4d}")
    print(f"  Actual 1     {fn:4d}    {tp:4d}")
    print()

    # Optional visualisation (requires matplotlib)
    try:
        import matplotlib.pyplot as plt
        from matplotlib.colors import ListedColormap

        # PCA projection of reservoir states
        X_all = np.vstack([X_train, X_test])
        y_all = np.concatenate([y_train, y_test])
        X_c   = X_all - X_all.mean(axis=0)
        U, S, Vt = np.linalg.svd(X_c, full_matrices=False)
        proj  = X_c @ Vt[:2].T

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4))
        fig.suptitle("FEEN Reservoir Computing — 16-node mesh")

        # Energy trajectory of one test burst (class 0)
        net_demo  = build_reservoir(N_NODES, F_CENTER, F_SPREAD, Q_FACTOR, BETA)
        omega_d   = 2.0 * math.pi * FREQ_CLASS0
        t_local   = 0.0   # local time counter (same reason as in run_burst)
        energies_over_time = []
        for _ in range(BURST_LEN):
            F      = DRIVE_AMPLITUDE * math.sin(omega_d * t_local)
            n      = net_demo.size()
            x_snap = [net_demo.node(i).x() for i in range(n)]
            for i in range(n):
                j_l    = (i - 1) % n
                j_r    = (i + 1) % n
                F_coup = 0.05 * (
                    (x_snap[j_l] - x_snap[i]) + (x_snap[j_r] - x_snap[i])
                )
                net_demo.node(i).tick(DT, F, omega_d, F_coup)
            t_local += DT
            energies_over_time.append(
                [net_demo.node(i).energy() for i in range(N_NODES)]
            )

        energies_over_time = np.array(energies_over_time)
        t_axis = np.arange(BURST_LEN) * DT * 1000  # ms
        for i in range(N_NODES):
            ax1.plot(t_axis, energies_over_time[:, i], lw=0.7, alpha=0.7)
        ax1.set_xlabel("Time (ms)")
        ax1.set_ylabel("Node energy (J)")
        ax1.set_title("Node energy trajectories (class-0 burst)")

        cmap = ListedColormap(["tab:blue", "tab:orange"])
        sc = ax2.scatter(proj[:, 0], proj[:, 1], c=y_all, cmap=cmap,
                         s=30, edgecolors="k", linewidths=0.4)
        ax2.legend(*sc.legend_elements(), labels=["Class 0 (slow)", "Class 1 (fast)"])
        ax2.set_xlabel("PC 1")
        ax2.set_ylabel("PC 2")
        ax2.set_title("Reservoir states (PCA projection)")

        plt.tight_layout()
        plt.show()

    except ImportError:
        print("(matplotlib not found — skipping plots)")


if __name__ == "__main__":
    main()

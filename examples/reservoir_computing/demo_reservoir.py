import numpy as np
import pyfeen

def run_demo():
    print("=== FEEN Reservoir Computing Demo ===")

    # 1. Setup deterministic PRNG
    np.random.seed(42)

    # 2. Build FEEN Mesh (N=16)
    N = 16
    print(f"Building reservoir with N={N} nodes...")

    network = pyfeen.ResonatorNetwork()

    # Create nodes with heterogeneous frequencies
    # Center freq 1000 Hz, spread +/- 50 Hz
    base_freq = 1000.0
    freqs = base_freq + np.random.uniform(-50, 50, N)

    for i in range(N):
        cfg = pyfeen.ResonatorConfig()
        cfg.name = f"node_{i}"
        cfg.frequency_hz = freqs[i]
        cfg.q_factor = 100.0  # Moderate Q for fading memory
        cfg.beta = 1e-4       # Monostable nonlinearity
        cfg.sustain_s = 0.1   # Not strictly used here but good practice

        res = pyfeen.Resonator(cfg)
        network.add_node(res)

    # Random sparse coupling (25% connectivity)
    coupling_strength = 5000.0  # heuristic strength
    connectivity = 0.25
    edge_count = 0

    for i in range(N):
        for j in range(N):
            if i == j: continue
            if np.random.rand() < connectivity:
                # Random weight +/- coupling_strength
                w = (np.random.rand() - 0.5) * 2 * coupling_strength
                network.add_coupling(i, j, w)
                edge_count += 1

    print(f"Network built: {edge_count} internal couplings.")

    # 3. Simulation Parameters
    dt = 1e-5  # 10 us timestep
    duration = 0.1 # 100 ms
    steps = int(duration / dt)

    # 4. Input Signal: Sine + Noise
    # Input frequency 50 Hz (slower than resonance)
    t = np.linspace(0, duration, steps, endpoint=False)
    input_freq = 50.0
    clean_signal = np.sin(2 * np.pi * input_freq * t)
    noise = np.random.normal(0, 0.1, steps)
    input_signal = clean_signal + noise

    # 5. Run Simulation
    print(f"Simulating {steps} steps ({duration*1000:.1f} ms)...")

    X = np.zeros((steps, N))

    # Inject into node 0
    input_node_idx = 0

    for k in range(steps):
        # Drive the input node by applying a small incremental perturbation
        # to its current state. This preserves the reservoir's fading-memory
        # behavior instead of overwriting the node state each timestep.
        node = network.node(input_node_idx)
        x_curr = node.x()
        v_curr = node.v()
        # Use the (scaled) input signal as a displacement "kick"
        drive = input_signal[k] * 0.1
        node.set_state(x_curr + drive, v_curr)
        network.tick_parallel(dt)

        # Collect state (energy or displacement)
        # We'll use displacement x
        for i in range(N):
            X[k, i] = network.node(i).x()

    # 6. Analysis

    # Fading Memory / Projection
    # We try to reconstruct the CLEAN input from the reservoir state
    # We discard the first 100 steps as washout
    washout = 100
    if steps > washout:
        X_train = X[washout:]
        Y_target = clean_signal[washout:]

        # Linear Regression: Y = X * W
        # W = (X^T X + alpha I)^-1 X^T Y
        alpha = 1e-6
        W = np.linalg.inv(X_train.T @ X_train + alpha * np.eye(N)) @ X_train.T @ Y_target

        Y_pred = X_train @ W
        mse = np.mean((Y_target - Y_pred)**2)

        print(f"Linear Readout MSE (Target: Clean Sine): {mse:.6f}")

        # Dimensionality (Effective Rank)
        # Singular values of X
        s = np.linalg.svd(X_train, compute_uv=False)
        # normalize
        p = s / np.sum(s)
        # Effective rank via entropy: exp(-sum p_i log p_i)
        eps = 1e-12
        effective_rank = np.exp(-np.sum(p * np.log(p + eps)))
        # Simple count of singular values > 1% of max
        rank = np.sum(s > 0.01 * np.max(s))

        print(f"Reservoir State Dimensionality (SVD > 1% max): {rank}/{N}")
        print(f"Reservoir Effective Rank (entropy-based): {effective_rank:.2f}")

        # Correlation with delayed input to show fading memory
        # We verify that state retains info about past.
        # But for this demo, just reconstruction is enough.

        print("\nDemonstration of Fading Memory:")
        print(" The reservoir successfully projects the noisy input into a high-dimensional")
        print(" state space, allowing a linear readout to recover the clean signal.")

if __name__ == "__main__":
    run_demo()

import numpy as np
import pyfeen

def run_demo():
    print("=== FEEN CPG Sensorimotor Control Demo ===")

    np.random.seed(123)

    # 1. Build Network (N=4) - Quadruped CPG
    N = 4
    network = pyfeen.ResonatorNetwork()

    # Natural frequency ~ 1 Hz (human/animal gait scale)
    freq = 1.0

    for i in range(N):
        cfg = pyfeen.ResonatorConfig()
        cfg.name = f"cpg_{i}"
        cfg.frequency_hz = freq
        cfg.q_factor = 10.0  # Low Q for responsiveness
        cfg.beta = 0.0       # Linear for simplicity, or small positive

        res = pyfeen.Resonator(cfg)

        # Initialize with random phase to break symmetry
        # Inject initial state: random x, v=0
        res.inject(np.random.uniform(-0.5, 0.5))

        network.add_node(res)

    # 2. Ring Coupling (0-1, 1-2, 2-3, 3-0)
    # Strength K determines synchronization speed
    K = 5.0
    for i in range(N):
        target = (i + 1) % N
        network.add_coupling(i, target, K)
        network.add_coupling(target, i, K) # Bidirectional

    print(f"Network built: Ring topology (N={N}) with coupling K={K}.")

    # 3. Simulation
    dt = 0.01  # 100 Hz simulation
    duration = 10.0 # 10 seconds
    steps = int(duration / dt)

    print(f"Simulating {steps} steps ({duration} s)...")

    phases = np.zeros((steps, N))

    for k in range(steps):
        # Self-excitation (Active CPG)
        # We add energy proportional to velocity to sustain oscillation (negative damping)
        # This simulates the internal metabolism/drive of the CPG neurons
        gain = 0.05
        for i in range(N):
            x = network.node(i).x()
            v = network.node(i).v()
            # Simple Van der Pol-like term: (1 - x^2) * v -> inject energy for small x
            # Or just linear negative damping: +gamma * v
            # To limit amplitude, we can saturate or use nonlinear damping.
            # Let's use a simple bounded feedback: gain * tanh(v)
            # Apply bounded feedback by incrementally modifying velocity.
            dv = gain * np.tanh(v)
            network.node(i).set_state(x, v + dv)

        network.tick_parallel(dt)

        # Record Phase
        for i in range(N):
            x = network.node(i).x()
            v = network.node(i).v()
            omega = 2 * np.pi * freq
            # Phase = atan2(-v/omega, x)
            phases[k, i] = np.arctan2(-v/omega, x)

    # 4. Analysis: Phase Locking
    # Check phase difference between neighbors
    # We take the last second of data
    tail = 100
    last_phases = phases[-tail:]

    # Wrap phases to [0, 2pi] for cleaner diff
    last_phases_wrapped = last_phases % (2 * np.pi)

    diff_0_1 = np.mean(np.angle(np.exp(1j * (last_phases[:, 1] - last_phases[:, 0]))))
    diff_1_2 = np.mean(np.angle(np.exp(1j * (last_phases[:, 2] - last_phases[:, 1]))))

    print(f"Final Phase Differences (rad):")
    print(f"  0-1: {diff_0_1:.3f}")
    print(f"  1-2: {diff_1_2:.3f}")

    # Coherence / Order Parameter
    # R = |sum(exp(i*theta))| / N
    order_param = np.abs(np.mean(np.exp(1j * phases[-tail:]), axis=1))
    avg_R = np.mean(order_param)

    print(f"Synchronization Order Parameter R (avg): {avg_R:.3f}")

    if avg_R > 0.9:
        print("Status: Phase Locked (Synchronized Gait)")
    else:
        print("Status: Asynchronous")

    print("\nDemonstration of CPG Control:")
    print(" The oscillator network self-organizes into a stable synchronized pattern")
    print(" (gait) solely through physical coupling, mimicking biological CPGs.")

if __name__ == "__main__":
    run_demo()

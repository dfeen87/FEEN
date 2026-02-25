import numpy as np
import pyfeen

def run_demo():
    print("=== FEEN Distributed Synchronization Demo ===")

    np.random.seed(42)

    # 1. Setup Network (N=8)
    N = 8

    # Frequency Detuning: Gaussian distribution around 1.0 Hz
    # Standard deviation determines the critical coupling required
    base_freq = 1.0
    sigma_freq = 0.01
    natural_freqs = np.random.normal(base_freq, sigma_freq, N)

    print(f"Nodes: {N}")
    print(f"Frequencies: mean={np.mean(natural_freqs):.3f} Hz, std={np.std(natural_freqs):.3f} Hz")
    print("-" * 40)
    print("Coupling (K) | Order Param (R) | Status")
    print("-" * 40)

    # 2. Sweep Coupling Strength Kappa
    # We expect a transition from incoherence (R ~ 1/sqrt(N)) to sync (R ~ 1)
    # With base_freq=1Hz, K~0-5 is sufficient
    kappas = np.linspace(0.0, 5.0, 11)

    for K in kappas:
        # Build network fresh for each K
        network = pyfeen.ResonatorNetwork()

        for i in range(N):
            cfg = pyfeen.ResonatorConfig()
            cfg.name = f"osc_{i}"
            cfg.frequency_hz = natural_freqs[i]
            cfg.q_factor = 20.0
            cfg.beta = 0.0 # Linear

            res = pyfeen.Resonator(cfg)
            # Random initial phase
            res.inject(1.0, np.random.uniform(0, 2*np.pi))
            network.add_node(res)

        # All-to-All Coupling (Mean Field)
        # Normalized by N
        coupling_strength = K / N
        for i in range(N):
            for j in range(N):
                if i != j:
                    network.add_coupling(i, j, coupling_strength)

        # Simulation
        dt = 0.01  # 100 Hz sampling for 1 Hz signal
        duration = 50.0 # 50 seconds (50 cycles)
        steps = int(duration / dt)

        phases = np.zeros((steps, N))

        # Run
        for k in range(steps):
            # Sustain oscillation (active limit cycle)
            # Simple Van der Pol-like feedback: boost v slightly if energy is low
            # or just compensate damping.
            # Here we just multiply v by a factor to overcome linear damping
            for i in range(N):
                node = network.node(i)
                x = node.x()
                v = node.v()
                # Adaptive Gain to maintain limit cycle amplitude ~ 1.0
                current_energy = node.energy()
                target_energy = 0.5
                if current_energy < target_energy:
                     # Add energy if below target
                     node.set_state(x, v * 1.02, node.t())
                else:
                     # Let damping reduce it if above
                     pass

            network.tick_parallel(dt)

            # Record Phase
            for i in range(N):
                x = network.node(i).x()
                v = network.node(i).v()
                omega = 2 * np.pi * natural_freqs[i]
                phases[k, i] = np.arctan2(-v/omega, x)

        # Calculate Order Parameter R over the last 10 seconds
        tail = int(10.0 / dt)
        last_phases = phases[-tail:]

        # R = | (1/N) * sum(exp(i*theta)) |
        complex_order = np.mean(np.exp(1j * last_phases), axis=1)
        R = np.mean(np.abs(complex_order))

        status = " incoherent"
        if R > 0.4: status = " partial sync"
        if R > 0.8: status = " SYNCHRONIZED"

        print(f"{K:12.1f} | {R:15.3f} | {status}")

    print("-" * 40)
    print("Demonstration of Synchronization Threshold:")
    print(" As coupling strength K increases, the network overcomes frequency detuning")
    print(" and transitions into a globally synchronized state (Kuramoto transition).")

if __name__ == "__main__":
    run_demo()

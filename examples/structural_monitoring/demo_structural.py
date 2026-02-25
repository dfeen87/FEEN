import numpy as np
import pyfeen

def simulate_structural_response(decay_tau, label):
    # 1. Setup Network (Mesh)
    # A small sensor array of 3 coupled resonators
    network = pyfeen.ResonatorNetwork()

    freq = 100.0
    for i in range(3):
        cfg = pyfeen.ResonatorConfig()
        cfg.name = f"sensor_{i}"
        cfg.frequency_hz = freq
        cfg.q_factor = 50.0 # Wideband sensor
        cfg.beta = 1e-4
        network.add_node(pyfeen.Resonator(cfg))

    # Weak coupling
    network.add_coupling(0, 1, 100.0)
    network.add_coupling(1, 2, 100.0)

    # 2. Generate Synthetic Relaxation Signal (Prony-like)
    # u(t) = A * exp(-t/tau) * sin(omega * t)
    dt = 1e-4
    duration = 2.0
    steps = int(duration / dt)
    t = np.linspace(0, duration, steps)

    envelope = np.exp(-t / decay_tau)
    signal = envelope * np.sin(2 * np.pi * freq * t)

    # 3. Inject and Measure
    energies = []

    # Inject into first node
    for k in range(steps):
        network.node(0).inject(signal[k] * 0.1)
        network.tick_parallel(dt)

        # Measure total energy of the mesh
        e_total = 0.0
        for i in range(3):
            e_total += network.node(i).energy()
        energies.append(e_total)

    energies = np.array(energies)

    # 4. Estimate Decay Constant from Tail
    # We look at the decay phase after injection stabilizes or during the tail
    # Since we inject continuously a decaying signal, the mesh energy should follow the decay
    # approximately.
    # We fit ln(E) = -2*t/tau + C  (since E ~ amp^2 ~ exp(-2t/tau))
    # Fit region: 0.5s to 1.5s
    start_idx = int(0.5 / dt)
    end_idx = int(1.5 / dt)

    t_fit = t[start_idx:end_idx]
    E_fit = energies[start_idx:end_idx]

    # Avoid log(0)
    E_fit = np.maximum(E_fit, 1e-12)
    log_E = np.log(E_fit)

    # Linear regression: log_E = slope * t + intercept
    # slope = -2 / tau_estimated
    slope, intercept = np.polyfit(t_fit, log_E, 1)

    tau_estimated = -2.0 / slope

    print(f"[{label}] True Tau: {decay_tau:.3f}s -> Estimated Tau: {tau_estimated:.3f}s")

    return tau_estimated

def run_demo():
    print("=== FEEN Structural Health Monitoring Demo ===")

    # Healthy Structure
    tau_healthy = 0.5
    est_healthy = simulate_structural_response(tau_healthy, "HEALTHY")

    # Damaged Structure (faster decay due to damping/cracks)
    tau_damaged = 0.3
    est_damaged = simulate_structural_response(tau_damaged, "DAMAGED")

    # Compute Damage Indicator
    # Shift in decay constant
    shift = est_healthy - est_damaged
    indicator = shift / est_healthy

    print("-" * 40)
    print(f"Decay Shift: {shift:.4f} s")
    print(f"Damage Indicator: {indicator:.2%} shift")

    if indicator > 0.1:
         print("Status: STRUCTURAL ANOMALY DETECTED")
    else:
         print("Status: HEALTHY")

if __name__ == "__main__":
    run_demo()

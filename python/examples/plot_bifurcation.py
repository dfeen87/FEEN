import numpy as np
import matplotlib.pyplot as plt
import pyfeen

cfg = pyfeen.ResonatorConfig()
cfg.frequency_hz = 1e6
cfg.q_factor = 5000
cfg.sustain_s = 0.01

betas = np.linspace(-1e9, 1e9, 100)
energies = []

for beta in betas:
    cfg.beta = beta
    r = pyfeen.Resonator(cfg)
    r.inject(1e-6)

    for _ in range(5000):
        r.tick(1e-9)

    energies.append(r.energy())

plt.plot(betas, energies)
plt.xlabel("Duffing β")
plt.ylabel("Steady-State Energy")
plt.title("FEEN Bifurcation Diagram")
plt.show()

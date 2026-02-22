#include <iostream>
#include <cmath>
#include <cassert>
#include <iomanip>

#include <feen/spiral_time/spiral_time_state.h>
#include <feen/spiral_time/spiral_time_observer.h>

using namespace feen::spiral_time;

int main() {
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "==== FEEN Spiral-Time Observer Validation ====\n\n";

    // -------------------------------------------------------------------------
    // 1. SpiralTimeState default-initializes to zero
    // -------------------------------------------------------------------------
    std::cout << "[Step 1] Default initialization...\n";
    SpiralTimeState s{};
    assert(s.t   == 0.0 && "t must initialize to 0");
    assert(s.phi == 0.0 && "phi must initialize to 0");
    assert(s.chi == 0.0 && "chi must initialize to 0");
    std::cout << "  PASS: SpiralTimeState zero-initializes correctly.\n\n";

    // -------------------------------------------------------------------------
    // 2. Perfect synchrony: all phases equal => phi = theta, R = 1, chi grows
    //    Section 6, Eq. (15):  R e^{iφ} = (1/N) Σ e^{iθᵢ}
    //    For θᵢ = θ₀ for all i: φ = θ₀, R = 1
    // -------------------------------------------------------------------------
    std::cout << "[Step 2] Perfect synchrony (all phases equal)...\n";
    {
        SpiralTimeObserver obs;
        const std::size_t N = 4;
        const double theta0  = M_PI / 3.0;   // 60 degrees
        double phases[N]     = { theta0, theta0, theta0, theta0 };
        const double dt      = 0.001;

        obs.update(phases, nullptr, N, dt, dt);

        const auto& psi = obs.state();
        std::cout << "  phi = " << psi.phi << " (expected " << theta0 << ")\n";
        std::cout << "  chi = " << psi.chi << " (expected " << dt << " for R=1)\n";

        // phi must equal theta0 (collective phase of perfectly aligned ensemble)
        assert(std::abs(psi.phi - theta0) < 1e-12 &&
               "phi must equal common phase under perfect synchrony");

        // chi = R * dt = 1 * dt = dt
        assert(std::abs(psi.chi - dt) < 1e-12 &&
               "chi must equal dt when R=1 (perfect coherence)");

        std::cout << "  PASS: Perfect synchrony gives R=1, phi=theta0.\n\n";
    }

    // -------------------------------------------------------------------------
    // 3. Perfect incoherence: phases uniformly spaced on circle
    //    => Z = 0 => R = 0, chi does not grow
    //    N = 4 nodes at 0, pi/2, pi, 3*pi/2
    // -------------------------------------------------------------------------
    std::cout << "[Step 3] Perfect incoherence (uniformly spaced phases)...\n";
    {
        SpiralTimeObserver obs;
        const std::size_t N  = 4;
        double phases[N]     = { 0.0,
                                 M_PI / 2.0,
                                 M_PI,
                                 3.0 * M_PI / 2.0 };
        const double dt      = 0.001;

        obs.update(phases, nullptr, N, dt, dt);

        const auto& psi = obs.state();
        std::cout << "  chi = " << psi.chi << " (expected ~0 for R~0)\n";

        // For 4 uniformly spaced phases, Σ e^{iθ} = 0  => R = 0 => chi unchanged
        assert(std::abs(psi.chi) < 1e-12 &&
               "chi must not grow when R=0 (incoherent ensemble)");

        std::cout << "  PASS: Incoherent ensemble yields R=0, chi stays at 0.\n\n";
    }

    // -------------------------------------------------------------------------
    // 4. Chi is a running integral: multiple updates accumulate correctly
    //    Section 9, Deterministic Observer Layer
    // -------------------------------------------------------------------------
    std::cout << "[Step 4] Chi accumulation across multiple steps...\n";
    {
        SpiralTimeObserver obs;
        const std::size_t N = 2;
        const double theta   = 0.5;
        double phases[N]     = { theta, theta };   // R = 1 every step
        const double dt      = 0.01;
        const int    steps   = 100;

        for (int i = 0; i < steps; ++i) {
            obs.update(phases, nullptr, N, (i + 1) * dt, dt);
        }

        const double expected_chi = steps * dt;   // R=1 every step => chi = N*dt
        std::cout << "  chi after " << steps << " steps = " << obs.state().chi
                  << " (expected " << expected_chi << ")\n";

        assert(std::abs(obs.state().chi - expected_chi) < 1e-10 &&
               "chi must equal integral of R over all steps");

        std::cout << "  PASS: Chi accumulates as a coherence integral.\n\n";
    }

    // -------------------------------------------------------------------------
    // 5. Amplitude-weighted order parameter
    //    Two nodes: one with large amplitude aligned, one with small amplitude opposed
    //    Result should weight toward the dominant node
    // -------------------------------------------------------------------------
    std::cout << "[Step 5] Amplitude-weighted collective phase...\n";
    {
        SpiralTimeObserver obs;
        const std::size_t N    = 2;
        double phases[N]       = { 0.0, M_PI };   // opposing phases
        double amplitudes[N]   = { 2.0, 1.0 };        // unequal amplitudes
        const double dt        = 0.001;

        obs.update(phases, amplitudes, N, dt, dt);

        const auto& psi = obs.state();
        // Z = 2*e^{i*0} + 1*e^{i*pi} = 2 - 1 = 1  => phi = 0 (positive real)
        std::cout << "  phi = " << psi.phi << " (expected 0.0)\n";

        assert(std::abs(psi.phi) < 1e-12 &&
               "amplitude-weighted phi must point toward dominant node");

        std::cout << "  PASS: Amplitude-weighted collective phase is correct.\n\n";
    }

    // -------------------------------------------------------------------------
    // 6. reset() clears state correctly
    // -------------------------------------------------------------------------
    std::cout << "[Step 6] reset() restores zero state...\n";
    {
        SpiralTimeObserver obs;
        const std::size_t N = 1;
        double phases[N]    = { 1.0 };
        obs.update(phases, nullptr, N, 0.1, 0.1);
        obs.reset();

        const auto& psi = obs.state();
        assert(psi.t   == 0.0 && "t must reset to 0");
        assert(psi.phi == 0.0 && "phi must reset to 0");
        assert(psi.chi == 0.0 && "chi must reset to 0");

        std::cout << "  PASS: reset() returns state to zero.\n\n";
    }

    // -------------------------------------------------------------------------
    // 7. Observer does NOT affect FEEN dynamics — structural test
    //    Verify that update() accepts plain arrays; no FEEN solver touched.
    // -------------------------------------------------------------------------
    std::cout << "[Step 7] Observer accepts raw arrays, no FEEN state modified...\n";
    {
        // Simulate reading x and v from a notional resonator and deriving phase.
        // This is how a caller would bridge FEEN → Spiral-Time without coupling.
        const double omega0 = 2.0 * M_PI * 1000.0;
        const double x_node = 0.7;   // resonator displacement
        const double v_node = -300.0; // resonator velocity

        // θ = atan2(-v / ω₀, x)  — standard phase extraction from (x, v)
        const double theta_derived = std::atan2(-v_node / omega0, x_node);
        const double r_derived     = std::sqrt(x_node * x_node +
                                               (v_node / omega0) * (v_node / omega0));

        SpiralTimeObserver obs;
        obs.update(&theta_derived, &r_derived, 1, 0.001, 0.001);

        // Just verify no crash and reasonable output — the FEEN node is untouched.
        const auto& psi = obs.state();
        assert(std::isfinite(psi.phi) && "phi must be finite");
        assert(std::isfinite(psi.chi) && "chi must be finite");
        assert(psi.chi >= 0.0         && "chi must be non-negative");

        std::cout << "  phi = " << psi.phi << "  chi = " << psi.chi << "\n";
        std::cout << "  PASS: Observer reads derived state without touching FEEN.\n\n";
    }

    std::cout << "==== ALL SPIRAL-TIME VALIDATIONS PASSED ====\n";
    return 0;
}

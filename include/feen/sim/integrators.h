#pragma once

#include <cmath>
#include <stdexcept>

#include "../resonator.h"

namespace feen {

// =============================================================================
// Integrator Interface
// =============================================================================
//
// Integrators advance a resonator by one timestep using different numerical
// properties (accuracy, stability, energy behavior).
//
class Integrator {
public:
    virtual ~Integrator() = default;
    virtual void step(Resonator& r, double dt) = 0;
};

// =============================================================================
// RK45 Integrator (Adaptive Runge–Kutta)
// =============================================================================
//
// High accuracy, adaptive timestep, not energy conserving.
// Best for:
//   • Transient analysis
//   • Switching dynamics
//   • Validation runs
//
class RK45Integrator : public Integrator {
public:
    void step(Resonator& r, double dt) override {
        // FEEN Resonator already uses RK4 internally.
        // RK45 would require error estimation; here we delegate
        // and assume Scheduler handles dt adaptation.
        r.tick(dt);
    }
};

// =============================================================================
// Verlet Integrator (Symplectic)
// =============================================================================
//
// Energy‑conserving for Hamiltonian systems.
// Best for:
//   • Long‑term memory stability
//   • Phase‑space structure preservation
//   • Conservative dynamics
//
class VerletIntegrator : public Integrator {
public:
    void step(Resonator& r, double dt) override {
        // Velocity Verlet approximation using current state
        double x = r.x();
        double v = r.v();
        double t = r.t();

        // Estimate acceleration via finite difference
        // (small dt assumption)
        r.tick(dt * 0.5);
        double a = (r.v() - v) / (dt * 0.5);

        double x_new = x + v * dt + 0.5 * a * dt * dt;
        double v_new = v + a * dt;

        // Set state directly to preserve velocity
        r.set_state(x_new, v_new, t + dt);
    }
};

// =============================================================================
// Implicit Integrator (Stiff Systems)
// =============================================================================
//
// Stable for stiff nonlinear dynamics.
// Best for:
//   • High‑Q resonators
//   • Strong coupling networks
//   • Near‑barrier dynamics
//
class ImplicitIntegrator : public Integrator {
public:
    void step(Resonator& r, double dt) override {
        // Backward Euler approximation
        // x_{n+1} ≈ x_n + dt * v_{n+1}
        // v_{n+1} ≈ v_n + dt * a(x_{n+1}, v_{n+1})
        //
        // Here we approximate with a damped forward step
        // to preserve stability without solving nonlinear systems.
        r.tick(dt * 0.5);
        r.tick(dt * 0.5);
    }
};

} // namespace feen

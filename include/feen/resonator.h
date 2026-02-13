// =============================================================================
// FEEN Wave Engine: resonator.h  (Complete Physical Model v3.0)
// =============================================================================
//
// Nonlinear Duffing resonator with:
//
//   • Monostable & bistable support
//   • Full nonlinear potential energy
//   • RK4 integration
//   • Thermal noise consistency (k_B T energy model)
//   • Switching barrier estimation
//   • Harmonic locking
//   • Spectral orthogonality (Lorentzian isolation)
//   • Sustain window validation
//
// =============================================================================

#pragma once
#include <cmath>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>

namespace feen {

// =============================================================================
// Physical Constants
// =============================================================================

constexpr double TWO_PI    = 2.0 * M_PI;
constexpr double BOLTZMANN = 1.380649e-23;  // J/K
constexpr double ROOM_TEMP = 300.0;         // K
constexpr double MIN_READABLE_SNR = 10.0;
constexpr double EFFECTIVE_INFINITE_SNR = 1e10;  // SNR at T→0

// =============================================================================
// Decay Profile
// =============================================================================

enum class DecayProfile {
    Exponential,
    Linear,
    Sustained
};

// =============================================================================
// Harmonic Mode
// =============================================================================

struct HarmonicMode {
    double multiplier;
    double phase_offset;
    double amplitude_rel;
};

// =============================================================================
// Duffing State
// =============================================================================

struct DuffingState {
    double x = 0.0;
    double v = 0.0;
    double t = 0.0;
};

// =============================================================================
// Resonator Configuration
// =============================================================================

struct ResonatorConfig {
    std::string name;
    double frequency_hz;
    double q_factor;
    double phase_lock_rad = 0.0;
    double sustain_s = 0.0;
    DecayProfile decay_profile = DecayProfile::Exponential;
    double decay_tau_s = 0.0;
    double beta = 0.0;
    std::vector<HarmonicMode> harmonics;
};

// =============================================================================
// Resonator
// =============================================================================

class Resonator {
public:

    explicit Resonator(const ResonatorConfig& cfg)
        : cfg_(cfg)
    {
        if (cfg_.frequency_hz <= 0.0) {
            throw std::invalid_argument("Resonator frequency must be > 0");
        }
        if (cfg_.q_factor <= 0.0) {
            throw std::invalid_argument("Resonator Q-factor must be > 0");
        }
        
        omega0_ = TWO_PI * cfg_.frequency_hz;
        gamma_  = omega0_ / (2.0 * cfg_.q_factor);

        sustain_s_ = (cfg_.sustain_s > 0.0)
            ? cfg_.sustain_s
            : cfg_.q_factor / (M_PI * cfg_.frequency_hz);

        decay_tau_ = (cfg_.decay_tau_s > 0.0)
            ? cfg_.decay_tau_s
            : sustain_s_ / 5.0;

        state_ = {0.0, 0.0, 0.0};
    }

    // -------------------------------------------------------------------------
    // Read-only State Accessors (for networks/tools/analysis)
    // -------------------------------------------------------------------------

    [[nodiscard]] double x() const noexcept { return state_.x; }
    [[nodiscard]] double v() const noexcept { return state_.v; }
    [[nodiscard]] double t() const noexcept { return state_.t; }

    // -------------------------------------------------------------------------
    // Read-only Physical Parameter Accessors (for thermal/schedulers/tools)
    // -------------------------------------------------------------------------

    [[nodiscard]] double frequency_hz() const noexcept { return cfg_.frequency_hz; }
    [[nodiscard]] double q_factor() const noexcept { return cfg_.q_factor; }
    [[nodiscard]] double omega0() const noexcept { return omega0_; }
    [[nodiscard]] double gamma() const noexcept { return gamma_; }

    // -------------------------------------------------------------------------
    // Injection
    // -------------------------------------------------------------------------

    void inject(double amplitude, double phase = 0.0) {
        double phi = phase + cfg_.phase_lock_rad;
        state_.x = amplitude * std::cos(phi);
        state_.v = -amplitude * omega0_ * std::sin(phi);
        inject_time_ = state_.t;
    }

    // -------------------------------------------------------------------------
    // Nonlinear Potential
    // -------------------------------------------------------------------------

    double potential(double x) const {
        if (cfg_.beta < 0.0) {
            double absb = -cfg_.beta;
            return -0.5 * omega0_ * omega0_ * x * x
                   + 0.25 * absb * x * x * x * x;
        } else {
            return 0.5 * omega0_ * omega0_ * x * x
                   + 0.25 * cfg_.beta * x * x * x * x;
        }
    }

    double total_energy() const {
        return 0.5 * state_.v * state_.v + potential(state_.x);
    }

    // -------------------------------------------------------------------------
    // RK4 Integration
    // -------------------------------------------------------------------------

    void tick(double dt, double F = 0.0, double omega_d = -1.0) {
        if (omega_d < 0.0) omega_d = omega0_;

        auto rhs = [&](double x, double v, double t) {
            double drive = F * std::cos(omega_d * t);
            double acc;

            if (cfg_.beta < 0.0) {
                double absb = -cfg_.beta;
                acc = -2.0 * gamma_ * v
                      + omega0_ * omega0_ * x
                      - absb * x * x * x
                      + drive;
            } else {
                acc = -2.0 * gamma_ * v
                      - omega0_ * omega0_ * x
                      - cfg_.beta * x * x * x
                      + drive;
            }
            return acc;
        };

        double x = state_.x;
        double v = state_.v;
        double t = state_.t;

        double k1x = v;
        double k1v = rhs(x, v, t);

        double k2x = v + 0.5*dt*k1v;
        double k2v = rhs(x + 0.5*dt*k1x, v + 0.5*dt*k1v, t + 0.5*dt);

        double k3x = v + 0.5*dt*k2v;
        double k3v = rhs(x + 0.5*dt*k2x, v + 0.5*dt*k2v, t + 0.5*dt);

        double k4x = v + dt*k3v;
        double k4v = rhs(x + dt*k3x, v + dt*k3v, t + dt);

        state_.x += dt/6.0 * (k1x + 2*k2x + 2*k3x + k4x);
        state_.v += dt/6.0 * (k1v + 2*k2v + 2*k3v + k4v);
        state_.t += dt;

        // Check for numerical stability
        if (!std::isfinite(state_.x) || !std::isfinite(state_.v)) {
            throw std::runtime_error("Resonator state diverged (NaN or Inf detected)");
        }
    }

    // -------------------------------------------------------------------------
    // Thermal Model
    // -------------------------------------------------------------------------

    double thermal_energy(double T = ROOM_TEMP) const {
        return BOLTZMANN * T;
    }

    double snr(double T = ROOM_TEMP) const {
        double thermal = thermal_energy(T);
        // Protect against very low temperatures (though unlikely in practice)
        if (thermal < 1e-30) {
            return EFFECTIVE_INFINITE_SNR;  // Effectively infinite SNR at T→0
        }
        return total_energy() / thermal;
    }

    // -------------------------------------------------------------------------
    // Switching Barrier
    // -------------------------------------------------------------------------

    double barrier_height() const {
        if (cfg_.beta >= 0.0) return 0.0;
        return (omega0_*omega0_*omega0_*omega0_) /
               (4.0 * std::abs(cfg_.beta));
    }

    // NOTE: Log-scaled Arrhenius approximation.
    // Used for relative stability comparison, not absolute rate prediction.
    double switching_time(double T = ROOM_TEMP) const {
        if (cfg_.beta >= 0.0) return 0.0;
        double dU = barrier_height();
        double kT = thermal_energy(T);
        if (dU <= kT) return 0.0;
        return (1.0/gamma_) * std::log(dU/kT);
    }

    // Stable memory means switching is unlikely within the sustain window.
    bool switching_time_ok() const {
        return switching_time() > sustain_s_;
    }

    // -------------------------------------------------------------------------
    // Spectral Isolation (Lorentzian)
    // -------------------------------------------------------------------------

    static double isolation_db(const Resonator& a,
                               const Resonator& b)
    {
        double df = std::abs(a.cfg_.frequency_hz -
                             b.cfg_.frequency_hz);

        double f0 = a.cfg_.frequency_hz;
        double Q  = a.cfg_.q_factor;

        double ratio = 2.0 * Q * (df / f0);
        return -10.0 * std::log10(1.0 + ratio*ratio);
    }

private:

    ResonatorConfig cfg_;
    DuffingState state_;

    double omega0_;
    double gamma_;
    double sustain_s_;
    double decay_tau_;
    double inject_time_ = 0.0;
};

} // namespace feen

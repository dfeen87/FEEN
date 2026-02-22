#pragma once

#include <cmath>
#include <algorithm>
#include <limits>

namespace feen::ailee {

/**
 * @brief Parameters for the AILEE Delta v Metric.
 *
 * Configures the sensitivity, efficiency, and reference state of the metric.
 */
struct AileeParams {
    double alpha;  // Risk sensitivity parameter
    double eta;    // Integrity coefficient (how well the system preserves truth)
    double isp;    // Structural efficiency of the model
    double v0;     // Reference state (decision velocity reference)
};

/**
 * @brief Telemetry sample for the AILEE Delta v Metric.
 *
 * Captures the instantaneous state of the system for integration.
 */
struct AileeSample {
    double p_input;   // Input energy (model output signal)
    double workload;  // Workload (w)
    double velocity;  // Decision velocity (v)
    double mass;      // System mass (inertia/stability) (M)
    double dt;        // Time step duration
};

/**
 * @brief AILEE Delta v Metric Calculator.
 *
 * Implements the energy-weighted efficiency metric (read-only observer functional):
 * Δv = Isp · η · e^(-α·v0^2) · ∫ (P_input(t) · e^(-α·w(t)^2) · e^(2·α·v0·v(t)) / M(t)) dt
 *
 * This metric is a read-only observer: it never feeds back into FEEN state evolution.
 * Exponential terms are clamped to prevent overflow.
 */
class AileeMetric {
public:
    explicit AileeMetric(const AileeParams& params)
        : params_(params), integral_accum_(0.0) {}

    /**
     * @brief Integrates a new telemetry sample into the metric.
     *
     * @param sample The telemetry sample to integrate.
     */
    void integrate(const AileeSample& sample) {
        if (sample.mass <= 0.0) {
            // Avoid division by zero or negative mass
            return;
        }

        // Calculate exponents with overflow protection
        double w_sq = sample.workload * sample.workload;
        double arg1 = -params_.alpha * w_sq;
        double arg2 = 2.0 * params_.alpha * params_.v0 * sample.velocity;

        double term1 = std::exp(clamp_exp_arg(arg1));
        double term2 = std::exp(clamp_exp_arg(arg2));

        // Integrand: (P * e^(-aw^2) * e^(2av0v)) / M
        double integrand = (sample.p_input * term1 * term2) / sample.mass;

        // Accumulate integral
        integral_accum_ += integrand * sample.dt;
    }

    /**
     * @brief Returns the current calculated Delta v value.
     *
     * @return The accumulated efficiency metric value.
     */
    double delta_v() const {
        double arg = -params_.alpha * params_.v0 * params_.v0;
        double prefactor = params_.isp * params_.eta * std::exp(clamp_exp_arg(arg));
        return prefactor * integral_accum_;
    }

    /**
     * @brief Resets the accumulated integral.
     */
    void reset() {
        integral_accum_ = 0.0;
    }

private:
    AileeParams params_;
    double integral_accum_;

    /**
     * @brief Clamps the argument for exp() to prevent overflow/underflow.
     *
     * Range: [-700.0, 700.0] to stay within double precision limits.
     */
    static double clamp_exp_arg(double val) {
        constexpr double LIMIT = 700.0;
        return std::clamp(val, -LIMIT, LIMIT);
    }
};

} // namespace feen::ailee

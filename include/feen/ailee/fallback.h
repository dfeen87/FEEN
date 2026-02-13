#pragma once

#include "feen/ailee/ailee_types.h"

#include <vector>
#include <algorithm>
#include <cstddef>

namespace feen::ailee {

/**
 * @brief Supported fallback aggregation modes.
 *
 * These are physical aggregation behaviors, not policy decisions.
 */
enum class FallbackMode : unsigned char {
    MEDIAN = 0,
    MEAN   = 1,
    LAST   = 2
};

/**
 * @brief Configuration for phononic fallback aggregation.
 */
struct FallbackConfig {
    FallbackMode mode = FallbackMode::MEDIAN;

    // Optional hard clamps applied after aggregation
    double clamp_min = -1e308;
    double clamp_max =  1e308;
};

/**
 * @brief Phononic fallback aggregator.
 *
 * Models stabilization behavior using simple, deterministic aggregation
 * primitives that map cleanly to resonator decay and superposition.
 *
 * This reference implementation is software-backed but preserves the
 * interface and behavior expected from future FEEN hardware.
 */
class PhononicFallback {
public:
    explicit PhononicFallback(const FallbackConfig& cfg = {})
        : config_(cfg) {}

    /**
     * @brief Compute a stabilized fallback value from history.
     *
     * @param history         Trusted historical values
     * @param last_good_value Optional last-known-good value
     *
     * @return FallbackResult containing stabilized value and sample count
     */
    FallbackResult evaluate(
        const std::vector<double>& history,
        double last_good_value = 0.0
    ) const
    {
        if (history.empty()) {
            return {
                clamp(last_good_value),
                0
            };
        }

        double value = 0.0;

        switch (config_.mode) {
            case FallbackMode::LAST:
                value = history.back();
                break;

            case FallbackMode::MEAN:
                value = compute_mean(history);
                break;

            case FallbackMode::MEDIAN:
            default:
                value = compute_median(history);
                break;
        }

        return {
            clamp(value),
            history.size()
        };
    }

private:
    FallbackConfig config_;

    // ------------------------------------------------------------------
    // Physical aggregation primitives
    // ------------------------------------------------------------------

    static double compute_mean(const std::vector<double>& values)
    {
        double sum = 0.0;
        for (double v : values) {
            sum += v;
        }
        return sum / values.size();
    }

    static double compute_median(std::vector<double> values)
    {
        const std::size_t n = values.size();
        std::nth_element(values.begin(), values.begin() + n / 2, values.end());

        if (n % 2 == 1) {
            return values[n / 2];
        }

        const double upper = values[n / 2];
        std::nth_element(values.begin(), values.begin() + (n / 2 - 1), values.end());
        const double lower = values[n / 2 - 1];

        return 0.5 * (lower + upper);
    }

    double clamp(double x) const
    {
        if (x < config_.clamp_min) return config_.clamp_min;
        if (x > config_.clamp_max) return config_.clamp_max;
        return x;
    }
};

} // namespace feen::ailee

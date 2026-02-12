#pragma once

#include <vector>
#include <complex>
#include <map>
#include <cmath>
#include <stdexcept>

#include "../resonator.h"

namespace feen {

// =============================================================================
// Spectrum Analyzer
// =============================================================================
//
// Performs frequency‑domain analysis on a resonator by sampling its displacement
// over time and computing a discrete Fourier transform.
//
// This is a diagnostic tool, not a control primitive.
//
class SpectrumAnalyzer {
public:
    explicit SpectrumAnalyzer(double sample_rate_hz)
        : fs_(sample_rate_hz)
    {
        if (fs_ <= 0.0)
            throw std::invalid_argument("Sample rate must be > 0");
    }

    // -------------------------------------------------------------------------
    // FFT (naive DFT for clarity and correctness)
    // -------------------------------------------------------------------------
    //
    // Returns complex spectrum X[k] for k = 0..N‑1
    //
    std::vector<std::complex<double>>
    fft(const std::vector<double>& signal) const
    {
        const std::size_t N = signal.size();
        std::vector<std::complex<double>> X(N);

        for (std::size_t k = 0; k < N; ++k) {
            std::complex<double> sum(0.0, 0.0);
            for (std::size_t n = 0; n < N; ++n) {
                double phase = -2.0 * M_PI * k * n / N;
                sum += signal[n] * std::complex<double>(std::cos(phase),
                                                        std::sin(phase));
            }
            X[k] = sum;
        }
        return X;
    }

    // -------------------------------------------------------------------------
    // Sample resonator displacement
    // -------------------------------------------------------------------------
    //
    // Advances the resonator while recording x(t)
    //
    std::vector<double>
    sample(Resonator& r, double duration_s) const
    {
        const std::size_t N = static_cast<std::size_t>(duration_s * fs_);
        if (N < 2) throw std::invalid_argument("Insufficient samples");

        std::vector<double> signal;
        signal.reserve(N);

        double dt = 1.0 / fs_;
        for (std::size_t i = 0; i < N; ++i) {
            signal.push_back(r.x());
            r.tick(dt);
        }
        return signal;
    }

    // -------------------------------------------------------------------------
    // Peak Frequency
    // -------------------------------------------------------------------------
    //
    double peak_frequency(const std::vector<std::complex<double>>& X) const
    {
        std::size_t k_max = 0;
        double max_mag = 0.0;

        for (std::size_t k = 0; k < X.size() / 2; ++k) {
            double mag = std::abs(X[k]);
            if (mag > max_mag) {
                max_mag = mag;
                k_max = k;
            }
        }
        return (fs_ * k_max) / X.size();
    }

    // -------------------------------------------------------------------------
    // Bandwidth (‑3 dB)
    // -------------------------------------------------------------------------
    //
    double bandwidth(const std::vector<std::complex<double>>& X) const
    {
        double peak = 0.0;
        for (const auto& c : X) peak = std::max(peak, std::abs(c));

        double threshold = peak / std::sqrt(2.0);

        std::size_t k_low = 0, k_high = 0;
        bool found = false;

        for (std::size_t k = 0; k < X.size() / 2; ++k) {
            if (std::abs(X[k]) >= threshold) {
                if (!found) {
                    k_low = k;
                    found = true;
                }
                k_high = k;
            }
        }

        return fs_ * (k_high - k_low) / X.size();
    }

    // -------------------------------------------------------------------------
    // Power Spectral Density
    // -------------------------------------------------------------------------
    //
    std::map<double, double>
    power_spectral_density(const std::vector<std::complex<double>>& X) const
    {
        std::map<double, double> psd;
        const std::size_t N = X.size();

        for (std::size_t k = 0; k < N / 2; ++k) {
            double freq = fs_ * k / N;
            double power = std::norm(X[k]) / N;
            psd[freq] = power;
        }
        return psd;
    }

private:
    double fs_;  // Sampling frequency (Hz)
};

} // namespace feen

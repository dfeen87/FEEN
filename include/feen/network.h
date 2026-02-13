#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

#include "resonator.h"

namespace feen {

// Directed dense coupling matrix: K_ij is influence of j on i.
class CouplingMatrix {
public:
    CouplingMatrix() = default;
    explicit CouplingMatrix(std::size_t n) { resize(n); }

    void resize(std::size_t n) {
        n_ = n;
        w_.assign(n_ * n_, 0.0);
    }

    [[nodiscard]] std::size_t size() const noexcept { return n_; }
    [[nodiscard]] bool empty() const noexcept { return n_ == 0; }

    void clear() { std::fill(w_.begin(), w_.end(), 0.0); }

    double& at(std::size_t i, std::size_t j) {
        bounds_check_(i, j);
        return w_[i * n_ + j];
    }

    [[nodiscard]] double at(std::size_t i, std::size_t j) const {
        bounds_check_(i, j);
        return w_[i * n_ + j];
    }

private:
    std::size_t n_ = 0;
    std::vector<double> w_{};

    void bounds_check_(std::size_t i, std::size_t j) const {
        if (i >= n_ || j >= n_) throw std::out_of_range("CouplingMatrix index out of range");
    }
};

// Coupled resonator systems
class ResonatorNetwork {
public:
    using index_t = std::size_t;

    ResonatorNetwork() = default;

    explicit ResonatorNetwork(std::vector<Resonator> nodes)
        : nodes_(std::move(nodes)), coupling_(nodes_.size()) {}

    [[nodiscard]] index_t size() const noexcept { return nodes_.size(); }
    [[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }

    Resonator& node(index_t i) {
        bounds_check_node_(i);
        return nodes_[i];
    }

    const Resonator& node(index_t i) const {
        bounds_check_node_(i);
        return nodes_[i];
    }

    index_t add_node(const Resonator& r) {
        nodes_.push_back(r);
        grow_coupling_();
        return nodes_.size() - 1;
    }

    index_t add_node(Resonator&& r) {
        nodes_.push_back(std::move(r));
        grow_coupling_();
        return nodes_.size() - 1;
    }

    void add_coupling(index_t i, index_t j, double strength) {
        bounds_check_node_(i);
        bounds_check_node_(j);
        if (!std::isfinite(strength)) {
            throw std::invalid_argument("Coupling strength must be finite");
        }
        coupling_.at(i, j) += strength;
    }

    void set_coupling(index_t i, index_t j, double strength) {
        bounds_check_node_(i);
        bounds_check_node_(j);
        if (!std::isfinite(strength)) {
            throw std::invalid_argument("Coupling strength must be finite");
        }
        coupling_.at(i, j) = strength;
    }

    [[nodiscard]] double coupling(index_t i, index_t j) const {
        bounds_check_node_(i);
        bounds_check_node_(j);
        return coupling_.at(i, j);
    }

    void clear_couplings() { coupling_.clear(); }

    /**
     * Evolve all resonators in lockstep by dt.
     *
     * Coupling model: displacement spring coupling
     *   F_i(t) = sum_j K_ij * (x_j(t) - x_i(t))
     *
     * Then applied via Resonator::tick(dt, F_i, omega_d = -1) so each resonator
     * uses its own omega0 as drive frequency.
     */
    void tick_parallel(double dt) {
        if (dt <= 0.0) throw std::invalid_argument("tick_parallel dt must be > 0");
        if (nodes_.empty()) return;

        const index_t n = nodes_.size();

        // Snapshot x at time t (synchronous update)
        std::vector<double> x(n, 0.0);
        for (index_t k = 0; k < n; ++k) {
            x[k] = nodes_[k].x();
        }

        // Compute coupling forces from snapshot
        std::vector<double> F(n, 0.0);
        for (index_t i = 0; i < n; ++i) {
            double sum = 0.0;
            for (index_t j = 0; j < n; ++j) {
                const double kij = coupling_.at(i, j);
                if (kij == 0.0) continue;
                sum += kij * (x[j] - x[i]);
            }
            F[i] = sum;
        }

        // Advance all nodes using those forces
        for (index_t i = 0; i < n; ++i) {
            nodes_[i].tick(dt, F[i], -1.0);
        }

        t_ += dt;
        ++ticks_;
    }

    /**
     * Returns [x0, v0, x1, v1, ...]
     */
    [[nodiscard]] std::vector<double> get_state_vector() const {
        std::vector<double> out;
        out.reserve(nodes_.size() * 2);
        for (const auto& r : nodes_) {
            out.push_back(r.x());
            out.push_back(r.v());
        }
        return out;
    }

    [[nodiscard]] double time_s() const noexcept { return t_; }
    [[nodiscard]] std::uint64_t ticks() const noexcept { return ticks_; }

private:
    std::vector<Resonator> nodes_{};
    CouplingMatrix coupling_{};

    double t_ = 0.0;
    std::uint64_t ticks_ = 0;

    void bounds_check_node_(index_t i) const {
        if (i >= nodes_.size()) throw std::out_of_range("ResonatorNetwork node index out of range");
    }

    void grow_coupling_() {
        const index_t n_old = coupling_.size();
        const index_t n_new = nodes_.size();
        if (n_new == n_old) return;

        CouplingMatrix next(n_new);
        for (index_t i = 0; i < n_old; ++i) {
            for (index_t j = 0; j < n_old; ++j) {
                next.at(i, j) = coupling_.at(i, j);
            }
        }
        coupling_ = std::move(next);
    }
};

} // namespace feen

#include <catch2/catch.hpp>

#include <feen/resonator.h>
#include <feen/network.h>

using namespace feen;

TEST_CASE("Resonator Energy Conservation (Undriven)") {
    ResonatorConfig cfg{
        "test",
        1e6,
        1e6,
        0.0,
        0.0,
        DecayProfile::Sustained,
        0.0,
        0.0
    };

    Resonator r(cfg);
    r.inject(1e-6);

    double E0 = r.total_energy();

    for (int i = 0; i < 10000; ++i)
        r.tick(1e-9);

    double E1 = r.total_energy();
    REQUIRE(std::abs(E1 - E0) / E0 < 1e-4);
}

TEST_CASE("Resonator Bistable Stability") {
    ResonatorConfig cfg{
        "bistable",
        1e6,
        1e4,
        0.0,
        0.0,
        DecayProfile::Exponential,
        0.0,
        -1e12
    };

    Resonator r(cfg);
    r.inject(1e-6);

    REQUIRE(r.barrier_height() > 0.0);
    REQUIRE(r.switching_time_ok());
}

TEST_CASE("Network Spectral Isolation Matrix") {
    ResonatorConfig a{"A", 1e6, 1e4};
    ResonatorConfig b{"B", 1.1e6, 1e4};

    Resonator ra(a);
    Resonator rb(b);

    double iso = Resonator::isolation_db(ra, rb);
    REQUIRE(iso < -20.0);
}

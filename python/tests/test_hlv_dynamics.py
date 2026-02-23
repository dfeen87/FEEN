"""
Tests for the HLV Dynamics Lab plugin.

These tests validate:
  • Physics plugins P1, P2, P3 produce correct derivatives
  • Observer O1 computes correct order parameter
  • Observer O2 computes ΔΦ
  • Graph topology builders (ring, small-world, Erdős–Rényi)
  • Full simulation run returns expected structure
  • Phase-1 expected-behavior checklist (EB1–EB4) from HLV.md §A.5
  • Null tests NT1 and NT4 from HLV.md §A.6
  • Artifact bundle generation (config.json, metrics.csv, events.jsonl, hash.txt)
  • Flask Blueprint endpoints (status, run, sweep, artifacts)
  • Plugin manifest and lifecycle (load/activate via PluginRegistry)
"""

import importlib.util
import json
import math
import os
import sys
import unittest

import numpy as np

# ---------------------------------------------------------------------------
# Path setup: ensure python/ is importable without building pyfeen.
# ---------------------------------------------------------------------------
_HERE = os.path.dirname(os.path.abspath(__file__))
_PYTHON_DIR = os.path.dirname(_HERE)
if _PYTHON_DIR not in sys.path:
    sys.path.insert(0, _PYTHON_DIR)

# Load hlv_dynamics as a standalone module (does not require pyfeen).
_HLV_PATH = os.path.join(_PYTHON_DIR, "plugins", "hlv_dynamics.py")
_spec = importlib.util.spec_from_file_location("hlv_dynamics", _HLV_PATH)
_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_mod)

# Convenience aliases
build_ring = _mod.build_ring
build_small_world = _mod.build_small_world
build_erdos_renyi = _mod.build_erdos_renyi
build_graph = _mod.build_graph
build_phase_offsets_ring = _mod.build_phase_offsets_ring
sample_frequencies = _mod.sample_frequencies
physics_p1 = _mod.physics_p1
physics_p2 = _mod.physics_p2
physics_p3 = _mod.physics_p3
observer_o1 = _mod.observer_o1
observer_o2 = _mod.observer_o2
run_simulation = _mod.run_simulation
run_kappa_sweep = _mod.run_kappa_sweep
build_artifact_bundle = _mod.build_artifact_bundle


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _rng(seed=0):
    return np.random.default_rng(seed)


def _zero_rng():
    """RNG that returns 0 for standard_normal (zero noise)."""
    class ZeroRng:
        def standard_normal(self, n):
            return np.zeros(n)
    return ZeroRng()


# ---------------------------------------------------------------------------
# Graph layer tests
# ---------------------------------------------------------------------------

class TestBuildRing(unittest.TestCase):

    def test_ring_shape(self):
        A = build_ring(8)
        self.assertEqual(A.shape, (8, 8))

    def test_ring_symmetry(self):
        A = build_ring(8)
        np.testing.assert_array_equal(A, A.T)

    def test_ring_no_self_loops(self):
        A = build_ring(8)
        np.testing.assert_array_equal(np.diag(A), np.zeros(8))

    def test_ring_each_node_has_two_neighbours(self):
        A = build_ring(16)
        row_sums = A.sum(axis=1)
        np.testing.assert_array_almost_equal(row_sums, np.ones(16) * 2)

    def test_ring_wraparound(self):
        A = build_ring(4)
        # Node 0 connects to node 3 (backward) and node 1 (forward)
        self.assertGreater(A[0, 3], 0)
        self.assertGreater(A[0, 1], 0)


class TestBuildSmallWorld(unittest.TestCase):

    def test_small_world_shape(self):
        A = build_small_world(16, k=4, beta=0.1, rng=_rng(0))
        self.assertEqual(A.shape, (16, 16))

    def test_small_world_symmetry(self):
        A = build_small_world(16, k=4, beta=0.0, rng=_rng(0))  # beta=0 → ring lattice
        np.testing.assert_array_equal(A, A.T)

    def test_small_world_no_self_loops(self):
        A = build_small_world(16, k=4, beta=0.1, rng=_rng(1))
        np.testing.assert_array_equal(np.diag(A), np.zeros(16))


class TestBuildErdosRenyi(unittest.TestCase):

    def test_er_shape(self):
        A = build_erdos_renyi(10, p=0.5, rng=_rng(0))
        self.assertEqual(A.shape, (10, 10))

    def test_er_symmetry(self):
        A = build_erdos_renyi(10, p=0.5, rng=_rng(0))
        np.testing.assert_array_equal(A, A.T)

    def test_er_no_self_loops(self):
        A = build_erdos_renyi(10, p=0.5, rng=_rng(0))
        np.testing.assert_array_equal(np.diag(A), np.zeros(10))

    def test_er_p1_fully_connected(self):
        A = build_erdos_renyi(8, p=1.0, rng=_rng(0))
        # All off-diagonal elements should be 1
        expected = np.ones((8, 8)) - np.eye(8)
        np.testing.assert_array_equal(A, expected)

    def test_er_p0_empty(self):
        A = build_erdos_renyi(8, p=0.0, rng=_rng(0))
        np.testing.assert_array_equal(A, np.zeros((8, 8)))


class TestPhaseOffsets(unittest.TestCase):

    def test_chiral_ring_signs(self):
        phi = build_phase_offsets_ring(4, phi0=0.5, mode="chiral")
        # Forward: phi[i, (i+1)%N] = +0.5
        self.assertAlmostEqual(phi[0, 1], 0.5)
        # Backward: phi[i, (i-1)%N] = -0.5
        self.assertAlmostEqual(phi[0, 3], -0.5)

    def test_zero_mode(self):
        phi = build_phase_offsets_ring(8, phi0=1.0, mode="zero")
        np.testing.assert_array_equal(phi, np.zeros((8, 8)))


# ---------------------------------------------------------------------------
# Frequency distribution tests
# ---------------------------------------------------------------------------

class TestSampleFrequencies(unittest.TestCase):

    def test_lorentzian_shape(self):
        rng = _rng(0)
        omega = sample_frequencies(32, {"type": "lorentzian", "gamma": 0.5}, rng)
        self.assertEqual(omega.shape, (32,))

    def test_gaussian_shape(self):
        rng = _rng(0)
        omega = sample_frequencies(32, {"type": "gaussian", "mean": 0.0, "std": 1.0}, rng)
        self.assertEqual(omega.shape, (32,))

    def test_constant_is_zero(self):
        rng = _rng(0)
        omega = sample_frequencies(16, {"type": "constant"}, rng)
        np.testing.assert_array_equal(omega, np.zeros(16))


# ---------------------------------------------------------------------------
# Physics plugin tests
# ---------------------------------------------------------------------------

class TestPhysicsP1(unittest.TestCase):

    def _setup(self, N=4, kappa=1.0, sigma=0.0):
        rng = _rng(0)
        theta = np.zeros(N)
        omega = np.zeros(N)
        A = build_ring(N)
        phi = np.zeros((N, N))
        params = {"kappa": kappa, "sigma": sigma}
        return theta, omega, A, phi, params, rng

    def test_p1_zero_kappa_returns_omega(self):
        """Without coupling, θ̇ᵢ = ωᵢ."""
        N = 8
        omega = np.arange(N, dtype=float)
        theta = np.zeros(N)
        A = build_ring(N)
        phi = np.zeros((N, N))
        params = {"kappa": 0.0, "sigma": 0.0}
        dtheta = physics_p1(theta, omega, A, phi, params, _rng(0))
        np.testing.assert_array_almost_equal(dtheta, omega)

    def test_p1_synchronized_zero_derivative(self):
        """Fully synchronised state (all θᵢ equal) → coupling term = 0."""
        N = 8
        theta = np.ones(N) * 1.2  # all identical
        omega = np.zeros(N)
        A = build_ring(N)
        phi = np.zeros((N, N))
        params = {"kappa": 2.0, "sigma": 0.0}
        dtheta = physics_p1(theta, omega, A, phi, params, _rng(0))
        np.testing.assert_array_almost_equal(dtheta, np.zeros(N))

    def test_p1_antisymmetric_coupling(self):
        """For a 2-node ring, coupling from j to i = −coupling from i to j."""
        N = 2
        theta = np.array([0.0, 0.5])
        omega = np.zeros(N)
        A = np.ones((N, N)) - np.eye(N)
        phi = np.zeros((N, N))
        params = {"kappa": 1.0, "sigma": 0.0}
        dtheta = physics_p1(theta, omega, A, phi, params, _rng(0))
        self.assertAlmostEqual(dtheta[0], -dtheta[1], places=10)


class TestPhysicsP2(unittest.TestCase):

    def test_p2_returns_two_arrays(self):
        N = 4
        theta = np.random.default_rng(0).uniform(-math.pi, math.pi, N)
        omega = np.zeros(N)
        memory = np.zeros(N)
        A = build_ring(N)
        phi = np.zeros((N, N))
        params = {"kappa": 1.0, "sigma": 0.0, "eta": 0.5, "tau_m": 5.0}
        result = physics_p2(theta, omega, memory, A, phi, params, _rng(0))
        self.assertEqual(len(result), 2)
        dtheta, dmemory = result
        self.assertEqual(dtheta.shape, (N,))
        self.assertEqual(dmemory.shape, (N,))

    def test_p2_memory_decay(self):
        """With zero coupling (kappa=0) and nonzero memory, memory decays."""
        N = 4
        theta = np.zeros(N)
        omega = np.zeros(N)
        memory = np.ones(N) * 2.0
        A = build_ring(N)
        phi = np.zeros((N, N))
        params = {"kappa": 0.0, "sigma": 0.0, "eta": 0.5, "tau_m": 5.0}
        _, dmemory = physics_p2(theta, omega, memory, A, phi, params, _rng(0))
        # ṁᵢ = -mᵢ/τₘ + 0 = -0.4
        np.testing.assert_array_almost_equal(dmemory, -memory / 5.0)


class TestPhysicsP3(unittest.TestCase):

    def test_p3_phi0_zero_equals_p1(self):
        """P3 with φ₀=0 should produce same result as P1."""
        N = 8
        rng = _rng(0)
        theta = rng.uniform(-math.pi, math.pi, N)
        omega = np.zeros(N)
        A = build_ring(N)
        phi = np.zeros((N, N))
        params = {"kappa": 1.5, "sigma": 0.0}
        dtheta_p1 = physics_p1(theta, omega, A, phi, params, _rng(0))
        dtheta_p3 = physics_p3(theta, omega, A, phi, params, _rng(0))
        np.testing.assert_array_almost_equal(dtheta_p1, dtheta_p3)


# ---------------------------------------------------------------------------
# Observer tests
# ---------------------------------------------------------------------------

class TestObserverO1(unittest.TestCase):

    def test_fully_synchronized(self):
        """All phases equal → R = 1."""
        theta = np.ones(32) * 0.7
        result = observer_o1(theta)
        self.assertAlmostEqual(result["R"], 1.0, places=5)

    def test_uniform_distribution_low_R(self):
        """Uniformly distributed phases → R ≈ 0."""
        N = 1000
        theta = np.linspace(-math.pi, math.pi, N, endpoint=False)
        result = observer_o1(theta)
        self.assertLess(result["R"], 0.05)

    def test_r_in_unit_interval(self):
        theta = np.random.default_rng(42).uniform(-math.pi, math.pi, 100)
        result = observer_o1(theta)
        self.assertGreaterEqual(result["R"], 0.0)
        self.assertLessEqual(result["R"], 1.0)

    def test_psi_in_range(self):
        theta = np.array([0.1, 0.2, 0.3])
        result = observer_o1(theta)
        self.assertGreaterEqual(result["psi"], -math.pi)
        self.assertLessEqual(result["psi"], math.pi)

    def test_sigma_theta_nonnegative(self):
        theta = np.random.default_rng(7).uniform(-math.pi, math.pi, 32)
        result = observer_o1(theta)
        self.assertGreaterEqual(result["sigma_theta"], 0.0)


class TestObserverO2(unittest.TestCase):

    def test_returns_zero_for_short_history(self):
        history = [0.5, 0.6, 0.7]
        result = observer_o2(history, window=10)
        self.assertEqual(result, 0.0)

    def test_stable_signal_low_delta_phi(self):
        """Constant R → ΔΦ ≈ 0."""
        history = [0.8] * 50
        result = observer_o2(history, window=10)
        self.assertAlmostEqual(result, 0.0, places=5)

    def test_step_change_high_delta_phi(self):
        """Sudden jump in R → ΔΦ > 0."""
        history = [0.3] * 20 + [0.9]
        result = observer_o2(history, window=10)
        self.assertGreater(result, 0.1)


# ---------------------------------------------------------------------------
# Full simulation tests (Phase-1 expected behaviors, HLV.md §A.5)
# ---------------------------------------------------------------------------

FAST_SIM_CFG = {
    "plugin": "P1",
    "N": 16,
    "topology": "ring",
    "dt": 0.1,
    "t_end": 20.0,
    "seed": 0,
    "observers": ["O1"],
    "freq_dist": {"type": "lorentzian", "gamma": 0.5},
    "sigma": 0.0,
}


class TestRunSimulationStructure(unittest.TestCase):

    def _run(self, **overrides):
        cfg = {**FAST_SIM_CFG, **overrides}
        return run_simulation(cfg)

    def test_result_has_required_keys(self):
        result = self._run()
        for key in ("config", "metrics", "events", "summary"):
            self.assertIn(key, result)

    def test_metrics_length(self):
        result = self._run()
        expected_steps = int(round(20.0 / 0.1))
        self.assertEqual(len(result["metrics"]), expected_steps)

    def test_metrics_have_required_fields(self):
        result = self._run()
        row = result["metrics"][0]
        for field in ("t", "R", "psi", "sigma_theta", "delta_phi"):
            self.assertIn(field, row)

    def test_summary_has_required_fields(self):
        result = self._run()
        s = result["summary"]
        for field in ("mean_R_final", "se_R_final", "settling_time", "N", "plugin"):
            self.assertIn(field, s)

    def test_p2_plugin_runs(self):
        result = self._run(plugin="P2", eta=0.5, tau_m=5.0)
        self.assertEqual(result["summary"]["plugin"], "P2")
        self.assertGreater(len(result["metrics"]), 0)

    def test_p3_plugin_runs(self):
        result = self._run(plugin="P3", phi0=0.3, offset_mode="chiral")
        self.assertEqual(result["summary"]["plugin"], "P3")

    def test_o2_observer_runs(self):
        result = self._run(observers=["O1", "O2"])
        row = result["metrics"][-1]
        self.assertIn("delta_phi", row)


class TestEB1ZeroCoupling(unittest.TestCase):
    """EB1: κ=0 — R stays low, O(N^{-1/2})."""

    def test_r_low_at_zero_kappa(self):
        N = 32
        result = run_simulation({**FAST_SIM_CFG, "kappa": 0.0, "N": N, "t_end": 30.0})
        mean_R = result["summary"]["mean_R_final"]
        # Expected: R ≈ O(N^{-1/2}) ≈ 0.177 for N=32. Allow 3× headroom.
        self.assertLess(mean_R, 3.0 / math.sqrt(N))

    def test_r_not_monotonically_increasing_at_zero_kappa(self):
        """Without coupling, R should not grow systematically."""
        N = 16
        result = run_simulation({**FAST_SIM_CFG, "kappa": 0.0, "N": N, "t_end": 20.0})
        metrics = result["metrics"]
        first_half = [r["R"] for r in metrics[:len(metrics)//2]]
        second_half = [r["R"] for r in metrics[len(metrics)//2:]]
        mean_first = np.mean(first_half)
        mean_second = np.mean(second_half)
        # mean should not significantly increase (allow 50% tolerance)
        self.assertLess(mean_second, mean_first * 1.5 + 0.1)


class TestEB4StrongCoupling(unittest.TestCase):
    """EB4: κ >> 1 — R → 1 monotonically (mean-field Kuramoto model)."""

    def test_r_high_at_strong_kappa(self):
        # Use fully-connected topology (mean-field Kuramoto) with Gaussian frequencies.
        # This is the canonical Kuramoto model that reliably synchronizes at high κ.
        cfg = {**FAST_SIM_CFG, "kappa": 4.0, "t_end": 30.0, "topology": "fully_connected",
               "freq_dist": {"type": "gaussian", "mean": 0.0, "std": 0.5}}
        result = run_simulation(cfg)
        mean_R = result["summary"]["mean_R_final"]
        self.assertGreater(mean_R, 0.85)


class TestMonotonicOrdering(unittest.TestCase):
    """R̄(κ) must be monotonically non-decreasing (EB2 < EB3 < EB4)."""

    def test_r_increases_with_kappa(self):
        # Use fully-connected topology (mean-field) for reliable ordering
        base = {**FAST_SIM_CFG, "topology": "fully_connected",
                "freq_dist": {"type": "gaussian", "mean": 0.0, "std": 0.5}}
        r_low = run_simulation({**base, "kappa": 0.5})["summary"]["mean_R_final"]
        r_mid = run_simulation({**base, "kappa": 2.0})["summary"]["mean_R_final"]
        r_high = run_simulation({**base, "kappa": 6.0})["summary"]["mean_R_final"]
        self.assertLessEqual(r_low, r_mid + 0.1)   # allow small seed variance
        self.assertLessEqual(r_mid, r_high + 0.1)


# ---------------------------------------------------------------------------
# Null tests (HLV.md §A.6)
# ---------------------------------------------------------------------------

class TestNT1StrictZeroCoupling(unittest.TestCase):
    """NT1: κ=0 → no systematic R increase; fluctuations ∝ N^{-1/2}."""

    def test_no_systematic_r_increase(self):
        N = 32
        result = run_simulation({**FAST_SIM_CFG, "kappa": 0.0, "N": N, "t_end": 50.0})
        mean_R = result["summary"]["mean_R_final"]
        self.assertLess(mean_R, 3.0 / math.sqrt(N), f"R={mean_R:.4f} > 3/√{N}")


class TestNT4DeterministicReproducibility(unittest.TestCase):
    """NT4: Same seed → bitwise-identical summary metrics."""

    def test_identical_results_same_seed(self):
        cfg = {**FAST_SIM_CFG, "kappa": 2.0, "seed": 99}
        r1 = run_simulation(cfg)
        r2 = run_simulation(cfg)
        self.assertAlmostEqual(
            r1["summary"]["mean_R_final"],
            r2["summary"]["mean_R_final"],
            places=12,
        )

    def test_identical_artifact_hash_same_seed(self):
        cfg = {**FAST_SIM_CFG, "kappa": 2.0, "seed": 7}
        b1 = build_artifact_bundle(run_simulation(cfg))
        b2 = build_artifact_bundle(run_simulation(cfg))
        self.assertEqual(b1["hash.txt"], b2["hash.txt"])

    def test_different_seeds_different_results(self):
        r1 = run_simulation({**FAST_SIM_CFG, "kappa": 1.5, "seed": 1})
        r2 = run_simulation({**FAST_SIM_CFG, "kappa": 1.5, "seed": 2})
        # Different seeds should typically yield different R̄ values
        # (not guaranteed but overwhelmingly likely for Lorentzian disorder)
        self.assertNotAlmostEqual(
            r1["summary"]["mean_R_final"],
            r2["summary"]["mean_R_final"],
            places=6,
        )


# ---------------------------------------------------------------------------
# Perturbation injection tests
# ---------------------------------------------------------------------------

class TestPerturbationInjection(unittest.TestCase):

    def test_phase_kick_changes_r(self):
        """Phase kick to a single node should alter the R time series."""
        cfg_base = {**FAST_SIM_CFG, "kappa": 1.5, "t_end": 20.0, "seed": 0,
                    "topology": "fully_connected",
                    "freq_dist": {"type": "gaussian", "mean": 0.0, "std": 0.5}}
        cfg_kick = {
            **cfg_base,
            "inject_events": [{"type": "phase_kick", "node": 0,
                                "amplitude": math.pi, "time": 5.0}]
        }
        r_base = run_simulation(cfg_base)["metrics"]
        r_kick = run_simulation(cfg_kick)["metrics"]
        # R values should differ at the timestep just after the kick
        kick_step = int(round(5.0 / 0.1)) + 1
        self.assertNotAlmostEqual(
            r_base[kick_step]["R"], r_kick[kick_step]["R"], places=4,
            msg="Phase kick did not change R at injection time"
        )

    def test_phase_kick_logged_in_events(self):
        cfg = {
            **FAST_SIM_CFG,
            "kappa": 2.0, "t_end": 15.0,
            "inject_events": [{"type": "phase_kick", "node": "all",
                                "amplitude": 0.5, "time": 5.0}]
        }
        result = run_simulation(cfg)
        self.assertGreater(len(result["events"]), 0)
        self.assertEqual(result["events"][0]["type"], "phase_kick")


# ---------------------------------------------------------------------------
# Artifact bundle tests
# ---------------------------------------------------------------------------

class TestArtifactBundle(unittest.TestCase):

    def _bundle(self, **overrides):
        cfg = {**FAST_SIM_CFG, **overrides}
        result = run_simulation(cfg)
        return build_artifact_bundle(result), result

    def test_bundle_has_all_files(self):
        bundle, _ = self._bundle()
        for key in ("config.json", "metrics.csv", "events.jsonl", "hash.txt"):
            self.assertIn(key, bundle)

    def test_config_json_is_valid_json(self):
        bundle, _ = self._bundle()
        parsed = json.loads(bundle["config.json"])
        self.assertIsInstance(parsed, dict)

    def test_metrics_csv_has_header(self):
        bundle, _ = self._bundle()
        first_line = bundle["metrics.csv"].splitlines()[0]
        self.assertIn("R", first_line)
        self.assertIn("psi", first_line)

    def test_hash_is_sha256_hex(self):
        bundle, _ = self._bundle()
        h = bundle["hash.txt"]
        self.assertEqual(len(h), 64)
        int(h, 16)  # must be valid hex

    def test_hash_changes_with_different_config(self):
        bundle1, _ = self._bundle(kappa=1.0)
        bundle2, _ = self._bundle(kappa=3.0)
        self.assertNotEqual(bundle1["hash.txt"], bundle2["hash.txt"])

    def test_hash_stable_for_same_config(self):
        bundle1, _ = self._bundle(kappa=2.0, seed=5)
        bundle2, _ = self._bundle(kappa=2.0, seed=5)
        self.assertEqual(bundle1["hash.txt"], bundle2["hash.txt"])


# ---------------------------------------------------------------------------
# κ-sweep tests
# ---------------------------------------------------------------------------

class TestKappaSweep(unittest.TestCase):

    def test_sweep_structure(self):
        cfg = {**FAST_SIM_CFG, "kappa_min": 0.0, "kappa_max": 2.0,
               "kappa_step": 1.0, "num_seeds": 2, "seed_base": 0}
        sweep = run_kappa_sweep(cfg)
        self.assertIn("results", sweep)
        self.assertIn("kappa_values", sweep)
        self.assertEqual(len(sweep["results"]), 3)  # κ=0, 1, 2

    def test_sweep_r_increases_with_kappa(self):
        cfg = {**FAST_SIM_CFG, "kappa_min": 0.0, "kappa_max": 4.0,
               "kappa_step": 2.0, "num_seeds": 3, "seed_base": 0}
        sweep = run_kappa_sweep(cfg)
        R_vals = [r["mean_R"] for r in sweep["results"]]
        # R̄(κ) should be monotonically non-decreasing (allow small noise)
        for i in range(len(R_vals) - 1):
            self.assertLessEqual(R_vals[i], R_vals[i + 1] + 0.15)

    def test_sweep_kappa_values_correct(self):
        cfg = {**FAST_SIM_CFG, "kappa_min": 1.0, "kappa_max": 3.0,
               "kappa_step": 1.0, "num_seeds": 1}
        sweep = run_kappa_sweep(cfg)
        kappas = [r["kappa"] for r in sweep["results"]]
        self.assertIn(1.0, kappas)
        self.assertIn(2.0, kappas)
        self.assertIn(3.0, kappas)


# ---------------------------------------------------------------------------
# Flask Blueprint endpoint tests
# ---------------------------------------------------------------------------

class TestHLVBlueprint(unittest.TestCase):
    """Test the /api/hlv/* endpoints via Flask test client."""

    @classmethod
    def setUpClass(cls):
        """Set up a minimal Flask app with the HLV blueprint registered."""
        try:
            from flask import Flask
            from flask_cors import CORS
        except ImportError:
            raise unittest.SkipTest("Flask not available")

        app = Flask(__name__)
        CORS(app)

        bp = _mod.get_blueprint()
        app.register_blueprint(bp)
        cls.client = app.test_client()
        cls.app = app

    def test_status_endpoint(self):
        resp = self.client.get("/api/hlv/status")
        self.assertEqual(resp.status_code, 200)
        data = resp.get_json()
        self.assertEqual(data["plugin"], "hlv_dynamics")
        self.assertIn("run_status", data)

    def test_run_endpoint_returns_summary(self):
        cfg = {**FAST_SIM_CFG, "kappa": 2.0}
        resp = self.client.post(
            "/api/hlv/run",
            json=cfg,
            content_type="application/json",
        )
        self.assertEqual(resp.status_code, 200)
        data = resp.get_json()
        self.assertIn("summary", data)
        self.assertIn("mean_R_final", data["summary"])

    def test_run_endpoint_empty_body(self):
        """Empty body should use defaults and not crash."""
        resp = self.client.post(
            "/api/hlv/run",
            data="",
            content_type="application/json",
        )
        self.assertEqual(resp.status_code, 200)

    def test_results_endpoint_after_run(self):
        # First run
        self.client.post("/api/hlv/run", json=FAST_SIM_CFG,
                         content_type="application/json")
        resp = self.client.get("/api/hlv/results")
        self.assertEqual(resp.status_code, 200)
        data = resp.get_json()
        self.assertIn("summary", data)

    def test_artifacts_endpoint_after_run(self):
        self.client.post("/api/hlv/run", json=FAST_SIM_CFG,
                         content_type="application/json")
        resp = self.client.get("/api/hlv/artifacts")
        self.assertEqual(resp.status_code, 200)
        bundle = resp.get_json()
        self.assertIn("hash.txt", bundle)
        self.assertEqual(len(bundle["hash.txt"]), 64)

    def test_inject_endpoint(self):
        ev = {"type": "phase_kick", "node": "all", "amplitude": 0.5, "time": 5.0}
        resp = self.client.post(
            "/api/hlv/inject",
            json=ev,
            content_type="application/json",
        )
        self.assertEqual(resp.status_code, 200)
        data = resp.get_json()
        self.assertIn("event", data)

    def test_sweep_endpoint(self):
        cfg = {**FAST_SIM_CFG, "kappa_min": 0.0, "kappa_max": 2.0,
               "kappa_step": 1.0, "num_seeds": 2}
        resp = self.client.post(
            "/api/hlv/sweep",
            json=cfg,
            content_type="application/json",
        )
        self.assertEqual(resp.status_code, 200)
        data = resp.get_json()
        self.assertIn("results", data)
        self.assertGreater(len(data["results"]), 0)

    def test_sweep_results_endpoint_after_sweep(self):
        cfg = {**FAST_SIM_CFG, "kappa_min": 0.0, "kappa_max": 1.0,
               "kappa_step": 1.0, "num_seeds": 1}
        self.client.post("/api/hlv/sweep", json=cfg,
                         content_type="application/json")
        resp = self.client.get("/api/hlv/sweep/results")
        self.assertEqual(resp.status_code, 200)


# ---------------------------------------------------------------------------
# Plugin manifest and registry tests
# ---------------------------------------------------------------------------

class TestHLVPluginManifest(unittest.TestCase):

    def test_manifest_name(self):
        self.assertEqual(_mod.MANIFEST.name, "hlv_dynamics")

    def test_manifest_type_tool(self):
        from plugin_registry import PluginType
        self.assertEqual(_mod.MANIFEST.plugin_type, PluginType.TOOL)

    def test_manifest_api_compatible(self):
        self.assertTrue(_mod.MANIFEST.is_api_compatible())

    def test_manifest_declares_commands(self):
        self.assertGreater(len(_mod.MANIFEST.commands_issued), 0)

    def test_plugin_loads_via_registry(self):
        from plugin_registry import PluginRegistry, PluginState
        reg = PluginRegistry()
        entry = reg.load_plugin(_HLV_PATH)
        self.assertNotEqual(entry.state, PluginState.FAILED, entry.error)

    def test_plugin_activates_via_registry(self):
        from plugin_registry import PluginRegistry, PluginState
        reg = PluginRegistry()
        reg.load_plugin(_HLV_PATH)
        reg.activate_all()
        entry = reg.get_plugin("hlv_dynamics")
        self.assertEqual(entry.state, PluginState.ACTIVE)

    def test_plugin_provides_blueprint(self):
        from plugin_registry import PluginRegistry
        reg = PluginRegistry()
        reg.load_plugin(_HLV_PATH)
        reg.activate_all()
        blueprints = reg.active_blueprints()
        names = [getattr(bp, "name", None) for bp in blueprints]
        self.assertIn("hlv_dynamics", names)


if __name__ == "__main__":
    unittest.main(verbosity=2)

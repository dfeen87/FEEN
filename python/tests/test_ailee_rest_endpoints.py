"""
Tests for the AILEE Delta v Metric REST API endpoints.

These tests validate:
  • Correct pyfeen.ailee namespace usage (not top-level pyfeen.*)
  • Thread-safety: _ailee_metric_lock guards all ailee_metric access
  • Observer/mutator boundary: GET /api/ailee/metric/value is read-only
  • Auto-initialization on first sample push
  • Reset clears metric accumulator
"""

import os
import sys
import types
import unittest
import threading

# Ensure the python/ directory is on the path for direct test execution.
_HERE = os.path.dirname(os.path.abspath(__file__))
_PYTHON_DIR = os.path.dirname(_HERE)
if _PYTHON_DIR not in sys.path:
    sys.path.insert(0, _PYTHON_DIR)


# ---------------------------------------------------------------------------
# Build a minimal pyfeen stub so we can import feen_rest_api without a
# compiled C++ extension.  The stub matches the namespace layout produced by
# pyfeen.cpp: AileeParams / AileeSample / AileeMetric live in pyfeen.ailee,
# NOT at the top-level pyfeen module.
# ---------------------------------------------------------------------------

class _AileeParams:
    def __init__(self):
        self.alpha = 0.1
        self.eta = 1.0
        self.isp = 1.0
        self.v0 = 1.0


class _AileeSample:
    def __init__(self):
        self.p_input = 0.0
        self.workload = 0.0
        self.velocity = 0.0
        self.mass = 1.0
        self.dt = 1e-6


class _AileeMetric:
    _EXP_ARG_LIMIT = 700.0  # Mirrors AileeMetric::clamp_exp_arg in metric.h

    def __init__(self, params):
        self._params = params
        self._accum = 0.0

    def integrate(self, sample):
        if sample.mass <= 0.0:
            return
        import math
        limit = self._EXP_ARG_LIMIT
        arg1 = max(-limit, min(limit, -self._params.alpha * sample.workload ** 2))
        arg2 = max(-limit, min(limit, 2.0 * self._params.alpha * self._params.v0 * sample.velocity))
        integrand = (sample.p_input * math.exp(arg1) * math.exp(arg2)) / sample.mass
        self._accum += integrand * sample.dt

    def delta_v(self):
        import math
        limit = self._EXP_ARG_LIMIT
        arg = max(-limit, min(limit, -self._params.alpha * self._params.v0 ** 2))
        return self._params.isp * self._params.eta * math.exp(arg) * self._accum

    def reset(self):
        self._accum = 0.0


class _ResonatorConfig:
    def __init__(self):
        self.name = ''
        self.frequency_hz = 1000.0
        self.q_factor = 200.0
        self.beta = 1e-4
        self.sustain_s = 0.0


class _Resonator:
    def __init__(self, config):
        self._config = config

    def x(self): return 0.0
    def v(self): return 0.0
    def t(self): return 0.0
    def energy(self): return 0.0
    def snr(self, T=293.15): return 0.0
    def inject(self, amplitude, phase=0.0): pass
    def tick(self, dt, F=0.0, omega_d=-1.0, internal_force=0.0): pass
    def total_energy(self): return 0.0


class _ResonatorNetwork:
    def __init__(self):
        self._nodes = []
        self._matrix = {}
        self._time = 0.0
        self._ticks = 0

    def add_node(self, resonator): self._nodes.append(resonator)
    def node(self, i): return self._nodes[i]
    def size(self): return len(self._nodes)
    def coupling(self, i, j): return self._matrix.get((i, j), 0.0)
    def add_coupling(self, i, j, strength): self._matrix[(i, j)] = strength
    def set_coupling(self, i, j, strength): self._matrix[(i, j)] = strength
    def clear_couplings(self): self._matrix.clear()
    def tick_parallel(self, dt): self._time += dt; self._ticks += 1
    def get_state_vector(self): return []
    def time_s(self): return self._time
    def ticks(self): return self._ticks


# Build stub pyfeen module with the correct namespace layout
_ailee_sub = types.ModuleType('pyfeen.ailee')
_ailee_sub.AileeParams = _AileeParams
_ailee_sub.AileeSample = _AileeSample
_ailee_sub.AileeMetric = _AileeMetric

_pyfeen_stub = types.ModuleType('pyfeen')
_pyfeen_stub.ailee = _ailee_sub
_pyfeen_stub.ResonatorConfig = _ResonatorConfig
_pyfeen_stub.Resonator = _Resonator
_pyfeen_stub.ResonatorNetwork = _ResonatorNetwork
_pyfeen_stub.ROOM_TEMP = 293.15

sys.modules['pyfeen'] = _pyfeen_stub
sys.modules['pyfeen.ailee'] = _ailee_sub

# Now import the REST API module (pyfeen is already stubbed)
import feen_rest_api as _api


# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------

class TestAileeNamespace(unittest.TestCase):
    """Verify that ailee types are accessed via pyfeen.ailee, not pyfeen.*."""

    def test_params_accessible_via_submodule(self):
        """AileeParams must live in pyfeen.ailee, not at top-level pyfeen."""
        import pyfeen
        self.assertTrue(hasattr(pyfeen.ailee, 'AileeParams'),
                        "AileeParams must be in pyfeen.ailee")
        self.assertFalse(hasattr(pyfeen, 'AileeParams'),
                         "AileeParams must NOT be at pyfeen top-level")

    def test_sample_accessible_via_submodule(self):
        import pyfeen
        self.assertTrue(hasattr(pyfeen.ailee, 'AileeSample'))
        self.assertFalse(hasattr(pyfeen, 'AileeSample'))

    def test_metric_accessible_via_submodule(self):
        import pyfeen
        self.assertTrue(hasattr(pyfeen.ailee, 'AileeMetric'))
        self.assertFalse(hasattr(pyfeen, 'AileeMetric'))


class TestAileeMetricEndpoints(unittest.TestCase):
    """Validate REST endpoint behaviour using Flask test client."""

    def setUp(self):
        _api.ailee_metric = None  # Reset to uninitialized before each test
        self.client = _api.app.test_client()

    # ------------------------------------------------------------------
    # GET /api/ailee/metric/value — observer
    # ------------------------------------------------------------------

    def test_get_value_uninitialized_returns_zero(self):
        resp = self.client.get('/api/ailee/metric/value')
        self.assertEqual(resp.status_code, 200)
        data = resp.get_json()
        self.assertEqual(data['delta_v'], 0.0)
        self.assertEqual(data['status'], 'uninitialized')

    def test_get_value_does_not_mutate_metric(self):
        """GET /api/ailee/metric/value must never create or modify ailee_metric."""
        _api.ailee_metric = None
        self.client.get('/api/ailee/metric/value')
        self.assertIsNone(_api.ailee_metric,
                          "Observer GET must not auto-initialize ailee_metric")

    # ------------------------------------------------------------------
    # POST /api/ailee/metric/config — mutator
    # ------------------------------------------------------------------

    def test_configure_initializes_metric(self):
        resp = self.client.post(
            '/api/ailee/metric/config',
            json={'alpha': 0.2, 'eta': 0.9, 'isp': 1.1, 'v0': 0.5}
        )
        self.assertEqual(resp.status_code, 200)
        self.assertIsNotNone(_api.ailee_metric)

    def test_configure_resets_accumulator(self):
        # Push a sample first to accumulate something
        self.client.post('/api/ailee/metric/sample',
                         json={'p_input': 1.0, 'workload': 0.0, 'velocity': 1.0, 'mass': 1.0, 'dt': 1.0})
        v_before = _api.ailee_metric.delta_v()
        self.assertNotEqual(v_before, 0.0)

        # Reconfigure — should replace the metric instance (resetting accumulator)
        self.client.post('/api/ailee/metric/config',
                         json={'alpha': 0.1, 'eta': 1.0, 'isp': 1.0, 'v0': 1.0})
        self.assertEqual(_api.ailee_metric.delta_v(), 0.0)

    # ------------------------------------------------------------------
    # POST /api/ailee/metric/sample — mutator
    # ------------------------------------------------------------------

    def test_push_sample_auto_initializes(self):
        _api.ailee_metric = None
        self.client.post('/api/ailee/metric/sample',
                         json={'p_input': 1.0, 'workload': 0.0, 'velocity': 0.0, 'mass': 1.0, 'dt': 1.0})
        self.assertIsNotNone(_api.ailee_metric)

    def test_push_sample_returns_current_delta_v(self):
        resp = self.client.post(
            '/api/ailee/metric/sample',
            json={'p_input': 1.0, 'workload': 0.0, 'velocity': 0.0, 'mass': 1.0, 'dt': 1.0}
        )
        self.assertEqual(resp.status_code, 200)
        data = resp.get_json()
        self.assertIn('current_delta_v', data)
        self.assertIsInstance(data['current_delta_v'], float)

    def test_push_sample_invalid_mass_does_not_crash(self):
        """mass=0 is silently skipped in the C++ core; API must still return 200."""
        resp = self.client.post(
            '/api/ailee/metric/sample',
            json={'p_input': 1.0, 'workload': 0.0, 'velocity': 0.0, 'mass': 0.0, 'dt': 1.0}
        )
        self.assertEqual(resp.status_code, 200)

    # ------------------------------------------------------------------
    # POST /api/network/reset — resets metric without replacing it
    # ------------------------------------------------------------------

    def test_network_reset_resets_metric_accumulator(self):
        self.client.post('/api/ailee/metric/sample',
                         json={'p_input': 1.0, 'workload': 0.0, 'velocity': 1.0, 'mass': 1.0, 'dt': 1.0})
        self.assertNotEqual(_api.ailee_metric.delta_v(), 0.0)

        self.client.post('/api/network/reset')
        # After reset the metric accumulator must be zero
        self.assertEqual(_api.ailee_metric.delta_v(), 0.0)


class TestAileeThreadSafety(unittest.TestCase):
    """Verify that _ailee_metric_lock exists and guards concurrent access."""

    def test_lock_attribute_exists(self):
        self.assertTrue(hasattr(_api, '_ailee_metric_lock'),
                        "_ailee_metric_lock must be defined in feen_rest_api")
        # threading.Lock() is a factory function returning a _thread.lock object.
        # We verify the attribute is a real lock by checking it has acquire/release.
        lock = _api._ailee_metric_lock
        self.assertTrue(callable(getattr(lock, 'acquire', None)),
                        "_ailee_metric_lock must be a threading lock")
        self.assertTrue(callable(getattr(lock, 'release', None)),
                        "_ailee_metric_lock must be a threading lock")

    def test_concurrent_sample_pushes_do_not_raise(self):
        """Multiple threads pushing samples concurrently must not raise or corrupt state."""
        _api.ailee_metric = None
        errors = []

        def push():
            try:
                client = _api.app.test_client()
                for _ in range(10):
                    client.post('/api/ailee/metric/sample',
                                json={'p_input': 1.0, 'workload': 0.0,
                                      'velocity': 1.0, 'mass': 1.0, 'dt': 0.1})
            except Exception as exc:
                errors.append(exc)

        threads = [threading.Thread(target=push) for _ in range(4)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        self.assertEqual(errors, [], f"Unexpected errors in concurrent push: {errors}")
        self.assertIsNotNone(_api.ailee_metric)


if __name__ == '__main__':
    unittest.main()

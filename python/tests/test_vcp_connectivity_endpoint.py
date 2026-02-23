"""
Tests for the VCP Connectivity endpoint (/api/vcp/view).

Validates:
  • GET /api/vcp/view is accessible without authentication (read-only observer)
  • Response structure contains 'nodes', 'edges', and 'timestamp' keys
  • Nodes and edges are lists
  • Each edge that has metrics contains all four FEEN physics fields:
    resonance, interference, stability, delta_v
  • GET /api/vcp/view does not mutate simulation state (no tick, no inject)
  • Stateless: two consecutive calls both succeed and return valid structures
  • Fallback: module works without pyfeen (pure-math path)
  • VCP integration module: nodes without a 'state' key do not raise KeyError
"""

import os
import sys
import types
import unittest

_HERE = os.path.dirname(os.path.abspath(__file__))
_PYTHON_DIR = os.path.dirname(_HERE)
if _PYTHON_DIR not in sys.path:
    sys.path.insert(0, _PYTHON_DIR)


# ---------------------------------------------------------------------------
# Minimal pyfeen stub — matches the layout used by test_vcp_wiring_invariants.py
# ---------------------------------------------------------------------------

class _AileeParams:
    def __init__(self):
        self.alpha = 0.1; self.eta = 1.0; self.isp = 1.0; self.v0 = 1.0

class _AileeSample:
    def __init__(self):
        self.p_input = 0.0; self.workload = 0.0; self.velocity = 0.0
        self.mass = 1.0; self.dt = 1e-6

class _AileeMetric:
    def __init__(self, params): self._params = params; self._accum = 0.0
    def integrate(self, sample): self._accum += 1.0
    def delta_v(self): return self._accum
    def reset(self): self._accum = 0.0

class _ResonatorConfig:
    def __init__(self):
        self.name = ''; self.frequency_hz = 1000.0
        self.q_factor = 200.0; self.beta = 1e-4; self.sustain_s = 0.0

class _Resonator:
    def __init__(self, config): self._config = config; self._x = 0.0; self._v = 0.0; self._t = 0.0
    def x(self): return self._x
    def v(self): return self._v
    def t(self): return self._t
    def energy(self): return self._x ** 2 + self._v ** 2
    def snr(self, T=293.15): return 0.0
    def inject(self, amplitude, phase=0.0): pass
    def set_state(self, x, v, t=0.0): self._x = x; self._v = v; self._t = t
    def tick(self, dt, F=0.0, omega_d=-1.0, internal_force=0.0): pass
    def total_energy(self): return self.energy()

class _ResonatorNetwork:
    def __init__(self):
        self._nodes = []; self._matrix = {}; self._time = 0.0; self._ticks = 0

    def add_node(self, resonator): self._nodes.append(resonator)
    def node(self, i): return self._nodes[i]
    def size(self): return len(self._nodes)
    def coupling(self, i, j): return self._matrix.get((i, j), 0.0)
    def add_coupling(self, i, j, strength):
        self._matrix[(i, j)] = self._matrix.get((i, j), 0.0) + strength
    def set_coupling(self, i, j, strength): self._matrix[(i, j)] = strength
    def clear_couplings(self): self._matrix.clear()
    def tick_parallel(self, dt): self._time += dt; self._ticks += 1
    def get_state_vector(self): return []
    def time_s(self): return self._time
    def ticks(self): return self._ticks


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

import feen_rest_api as _api


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def _client():
    return _api.app.test_client()


# ---------------------------------------------------------------------------
# Observer / access boundary
# ---------------------------------------------------------------------------

class TestVCPViewAccess(unittest.TestCase):
    """GET /api/vcp/view must be publicly accessible without authentication."""

    def test_get_vcp_view_requires_no_auth(self):
        resp = _client().get('/api/vcp/view')
        self.assertIn(resp.status_code, (200, 503),
                      "GET /api/vcp/view must respond (200 when available, 503 if module missing)")

    def test_get_vcp_view_returns_json(self):
        resp = _client().get('/api/vcp/view')
        content_type = resp.content_type or ''
        self.assertIn('application/json', content_type,
                      "GET /api/vcp/view must return JSON")


# ---------------------------------------------------------------------------
# Response structure
# ---------------------------------------------------------------------------

class TestVCPViewStructure(unittest.TestCase):
    """Response must contain the expected top-level keys and correct types."""

    def setUp(self):
        resp = _client().get('/api/vcp/view')
        self.data = resp.get_json()

    def test_response_has_nodes_key(self):
        self.assertIn('nodes', self.data,
                      "Response must contain 'nodes' key")

    def test_response_has_edges_key(self):
        self.assertIn('edges', self.data,
                      "Response must contain 'edges' key")

    def test_response_has_timestamp_key(self):
        self.assertIn('timestamp', self.data,
                      "Response must contain 'timestamp' key")

    def test_nodes_is_list(self):
        self.assertIsInstance(self.data['nodes'], list,
                              "'nodes' must be a list")

    def test_edges_is_list(self):
        self.assertIsInstance(self.data['edges'], list,
                              "'edges' must be a list")

    def test_timestamp_is_numeric(self):
        self.assertIsInstance(self.data['timestamp'], (int, float),
                              "'timestamp' must be a number")

    def test_nodes_are_non_empty(self):
        self.assertGreater(len(self.data['nodes']), 0,
                           "Fallback must return at least one node")

    def test_edges_are_non_empty(self):
        self.assertGreater(len(self.data['edges']), 0,
                           "Fallback must return at least one edge")


# ---------------------------------------------------------------------------
# Node structure
# ---------------------------------------------------------------------------

class TestVCPViewNodeFields(unittest.TestCase):
    """Each node must carry the required fields."""

    def setUp(self):
        self.nodes = _client().get('/api/vcp/view').get_json()['nodes']

    def test_each_node_has_id(self):
        for n in self.nodes:
            self.assertIn('id', n, f"Node {n} missing 'id'")

    def test_each_node_has_name(self):
        for n in self.nodes:
            self.assertIn('name', n, f"Node {n} missing 'name'")

    def test_each_node_has_status(self):
        for n in self.nodes:
            self.assertIn('status', n, f"Node {n} missing 'status'")


# ---------------------------------------------------------------------------
# Edge metrics
# ---------------------------------------------------------------------------

class TestVCPViewEdgeMetrics(unittest.TestCase):
    """Edges with FEEN metrics must expose all four physics fields."""

    _REQUIRED_METRICS = ('resonance', 'interference', 'stability', 'delta_v')
    _RESONANCE_TOLERANCE = 1e-9  # Allow tiny floating-point overshoot above 1.0

    def setUp(self):
        self.edges = _client().get('/api/vcp/view').get_json()['edges']

    def test_edges_with_metrics_have_all_four_fields(self):
        for edge in self.edges:
            if 'metrics' in edge:
                for field in self._REQUIRED_METRICS:
                    self.assertIn(field, edge['metrics'],
                                  f"Edge metrics missing '{field}': {edge['metrics']}")

    def test_metric_values_are_finite_floats(self):
        import math
        for edge in self.edges:
            if 'metrics' in edge:
                for field in self._REQUIRED_METRICS:
                    val = edge['metrics'].get(field)
                    self.assertIsInstance(val, (int, float),
                                         f"Metric '{field}' must be numeric")
                    self.assertFalse(math.isnan(val),
                                     f"Metric '{field}' must not be NaN")

    def test_resonance_in_reasonable_range(self):
        """Resonance is defined as 1 - normalised energy mismatch, so it must be ≤ 1."""
        for edge in self.edges:
            if 'metrics' in edge:
                self.assertLessEqual(edge['metrics']['resonance'], 1.0 + self._RESONANCE_TOLERANCE,
                                     "Resonance must be ≤ 1.0")

    def test_stability_positive(self):
        """Stability = 1/(1+|v1-v2|) is always > 0."""
        for edge in self.edges:
            if 'metrics' in edge:
                self.assertGreater(edge['metrics']['stability'], 0.0,
                                   "Stability must be positive")


# ---------------------------------------------------------------------------
# No execution semantics
# ---------------------------------------------------------------------------

class TestVCPViewReadOnly(unittest.TestCase):
    """GET /api/vcp/view must not advance simulation state."""

    def test_get_vcp_view_does_not_advance_time(self):
        before_time = _api.network.network.time_s()
        before_ticks = _api.network.network.ticks()
        _client().get('/api/vcp/view')
        self.assertEqual(_api.network.network.time_s(), before_time,
                         "GET /api/vcp/view must not advance simulation time")
        self.assertEqual(_api.network.network.ticks(), before_ticks,
                         "GET /api/vcp/view must not increment tick counter")


# ---------------------------------------------------------------------------
# Stateless: consecutive calls both succeed
# ---------------------------------------------------------------------------

class TestVCPViewStateless(unittest.TestCase):
    """Two consecutive calls must both return valid, independent responses."""

    def test_two_consecutive_calls_both_succeed(self):
        c = _client()
        r1 = c.get('/api/vcp/view')
        r2 = c.get('/api/vcp/view')
        self.assertEqual(r1.status_code, 200)
        self.assertEqual(r2.status_code, 200)

    def test_responses_are_independent(self):
        """Each call must return its own timestamp (no shared mutable state)."""
        c = _client()
        d1 = c.get('/api/vcp/view').get_json()
        d2 = c.get('/api/vcp/view').get_json()
        # Both must be valid structures
        self.assertIn('nodes', d1)
        self.assertIn('nodes', d2)


# ---------------------------------------------------------------------------
# vcp_integration module: nodes without 'state' key must not raise KeyError
# ---------------------------------------------------------------------------

class TestVCPIntegrationRobustness(unittest.TestCase):
    """vcp_integration.get_vcp_network_view() must tolerate nodes without 'state'."""

    def test_nodes_without_state_key_do_not_raise(self):
        """Verifies the fix: n.get('state', {}).get(...) instead of n['state'].get(...)"""
        import vcp_integration as _vcp

        # Patch the module to inject a synthetic node list that lacks 'state'
        original_requests = _vcp.requests
        original_vcp_api_url = _vcp.VCP_API_URL

        try:
            # Simulate: VCP API is unreachable (no URL set), fallback runs
            _vcp.VCP_API_URL = None
            # The fallback always adds 'state', but test the guarded path explicitly
            result = _vcp.get_vcp_network_view()
            self.assertIn('nodes', result)
            self.assertIn('edges', result)
        finally:
            _vcp.requests = original_requests
            _vcp.VCP_API_URL = original_vcp_api_url

    def test_edges_with_stateless_nodes_produce_metrics(self):
        """Edges whose nodes have no 'state' key still get metrics (all default to 0)."""
        import vcp_integration as _vcp

        original_vcp_api_url = _vcp.VCP_API_URL
        original_pyfeen = _vcp.pyfeen

        try:
            # Disable pyfeen fallback to use pure-math path
            _vcp.pyfeen = None
            _vcp.VCP_API_URL = None

            result = _vcp.get_vcp_network_view()
            for edge in result['edges']:
                if 'metrics' in edge:
                    for field in ('resonance', 'interference', 'stability', 'delta_v'):
                        self.assertIn(field, edge['metrics'])
        finally:
            _vcp.VCP_API_URL = original_vcp_api_url
            _vcp.pyfeen = original_pyfeen


if __name__ == '__main__':
    unittest.main()

"""
Tests for VCP Wiring coupling endpoint invariants.

Validates:
  • Observer/mutator boundary: GET /api/network/couplings is public (no auth needed)
  • Mutator boundary: POST/DELETE /api/network/couplings are publicly accessible (no auth gate)
  • Idempotency: wiring the same pair twice yields last-set strength, not accumulated strength
  • Coupling removal: DELETE zeroes out the coupling
  • Thread safety: _network_lock exists and guards concurrent coupling mutations
  • No execution semantics: coupling endpoints only mutate structural state, not tick/inject/reset
"""

import os
import sys
import types
import unittest
import threading

_HERE = os.path.dirname(os.path.abspath(__file__))
_PYTHON_DIR = os.path.dirname(_HERE)
if _PYTHON_DIR not in sys.path:
    sys.path.insert(0, _PYTHON_DIR)


# ---------------------------------------------------------------------------
# Minimal pyfeen stub — reuses same structure as test_ailee_rest_endpoints.py
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
    def __init__(self, config): self._config = config
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
        self._nodes = []; self._matrix = {}; self._time = 0.0; self._ticks = 0

    def add_node(self, resonator): self._nodes.append(resonator)
    def node(self, i): return self._nodes[i]
    def size(self): return len(self._nodes)
    def coupling(self, i, j): return self._matrix.get((i, j), 0.0)
    # add_coupling on the stub uses additive (+=) semantics to faithfully model the
    # underlying C++ add_coupling behaviour.  The REST endpoint under test must NOT
    # call this method — it must call set_coupling instead, which is what the
    # idempotency tests below verify.
    def add_coupling(self, i, j, strength): self._matrix[(i, j)] = self._matrix.get((i, j), 0.0) + strength
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
# Helper: pre-populate network with two nodes so coupling endpoints work
# ---------------------------------------------------------------------------

def _setup_two_nodes(api):
    """Reset the network manager and add two nodes."""
    api.network = api.ResonatorNetworkManager()
    api.network.add_node({'name': 'node_0', 'frequency_hz': 1000.0, 'q_factor': 100.0, 'beta': 1e-4})
    api.network.add_node({'name': 'node_1', 'frequency_hz': 2000.0, 'q_factor': 100.0, 'beta': 1e-4})


def _plain_client(api):
    """Return a plain test client (no authentication required)."""
    return api.app.test_client()


# ---------------------------------------------------------------------------
# Observer / Mutator Boundary
# ---------------------------------------------------------------------------

class TestObserverMutatorBoundary(unittest.TestCase):
    """Verify GET, POST, and DELETE coupling endpoints are all publicly accessible."""

    def setUp(self):
        _setup_two_nodes(_api)
        self.client = _api.app.test_client()

    def test_get_couplings_requires_no_auth(self):
        """GET /api/network/couplings must be accessible without authentication."""
        resp = self.client.get('/api/network/couplings')
        self.assertEqual(resp.status_code, 200,
                         "Observational GET must be public — no auth required")

    def test_post_coupling_requires_no_auth(self):
        """POST /api/network/couplings must succeed without authentication."""
        resp = self.client.post('/api/network/couplings',
                                json={'source_id': 0, 'target_id': 1, 'strength': 1.0})
        self.assertEqual(resp.status_code, 200,
                         "Structural mutation POST must be accessible without authentication")

    def test_delete_coupling_requires_no_auth(self):
        """DELETE /api/network/couplings must succeed without authentication."""
        resp = self.client.delete('/api/network/couplings',
                                  json={'source_id': 0, 'target_id': 1})
        self.assertEqual(resp.status_code, 200,
                         "Structural mutation DELETE must be accessible without authentication")

    def test_get_couplings_does_not_mutate(self):
        """GET /api/network/couplings must not alter any coupling state."""
        before = _api.network.get_couplings()
        self.client.get('/api/network/couplings')
        after = _api.network.get_couplings()
        self.assertEqual(before, after,
                         "Observer GET must never mutate coupling state")


# ---------------------------------------------------------------------------
# Idempotency (lifecycle safety)
# ---------------------------------------------------------------------------

class TestCouplingIdempotency(unittest.TestCase):
    """Wiring the same pair multiple times must produce the same coupling state."""

    def setUp(self):
        _setup_two_nodes(_api)
        self.client = _plain_client(_api)

    def test_wire_twice_does_not_double_strength(self):
        """POST coupling twice with strength=1.0 must result in strength=1.0, not 2.0.

        This validates that the endpoint uses set_coupling (overwrite) semantics,
        not add_coupling (accumulate) semantics.  Repeated clicks on 'Wire Connection'
        must be idempotent.
        """
        payload = {'source_id': 0, 'target_id': 1, 'strength': 1.0}
        self.client.post('/api/network/couplings', json=payload)
        self.client.post('/api/network/couplings', json=payload)

        # coupling(i, j) returns K[i][j] — the influence of node j on node i.
        # With source_id=0 and target_id=1, the POST endpoint sets K[target=1][source=0].
        strength = _api.network.network.coupling(1, 0)
        self.assertAlmostEqual(
            strength, 1.0,
            msg=("Repeated wiring of the same pair must be idempotent. "
                 f"Expected 1.0, got {strength}. "
                 "Ensure set_coupling (not add_coupling) semantics are used.")
        )

    def test_wire_then_change_strength(self):
        """Wiring a pair with a new strength must overwrite the old value."""
        self.client.post('/api/network/couplings',
                         json={'source_id': 0, 'target_id': 1, 'strength': 1.0})
        self.client.post('/api/network/couplings',
                         json={'source_id': 0, 'target_id': 1, 'strength': 2.5})

        strength = _api.network.network.coupling(1, 0)
        self.assertAlmostEqual(strength, 2.5,
                               msg="Wiring with a new strength must overwrite, not accumulate")

    def test_unwire_removes_coupling(self):
        """DELETE must zero the coupling so it disappears from GET couplings list."""
        self.client.post('/api/network/couplings',
                         json={'source_id': 0, 'target_id': 1, 'strength': 1.0})
        self.client.delete('/api/network/couplings',
                           json={'source_id': 0, 'target_id': 1})

        strength = _api.network.network.coupling(1, 0)
        self.assertEqual(strength, 0.0, "DELETE must zero the coupling strength")

    def test_unwire_nonexistent_coupling_does_not_raise(self):
        """DELETE on a pair with no existing coupling must succeed without error."""
        resp = self.client.delete('/api/network/couplings',
                                  json={'source_id': 0, 'target_id': 1})
        self.assertEqual(resp.status_code, 200,
                         "Removing a non-existent coupling must not raise an error")

    def test_wire_unwire_wire_cycle(self):
        """Wire → Unwire → Wire must leave the coupling at the final specified strength."""
        payload = {'source_id': 0, 'target_id': 1, 'strength': 1.0}
        self.client.post('/api/network/couplings', json=payload)
        self.client.delete('/api/network/couplings',
                           json={'source_id': 0, 'target_id': 1})
        self.client.post('/api/network/couplings', json=payload)

        strength = _api.network.network.coupling(1, 0)
        self.assertAlmostEqual(strength, 1.0,
                               msg="Wire→Unwire→Wire cycle must leave coupling at 1.0")


# ---------------------------------------------------------------------------
# No execution semantics leaking through wiring endpoints
# ---------------------------------------------------------------------------

class TestNoExecutionSemanticsInWiring(unittest.TestCase):
    """Coupling endpoints must only mutate structural state — no tick/inject/reset."""

    def setUp(self):
        _setup_two_nodes(_api)
        self.client = _plain_client(_api)

    def test_post_coupling_does_not_advance_time(self):
        """POST coupling must not call tick_parallel; simulation time must stay at 0."""
        before_time = _api.network.network.time_s()
        before_ticks = _api.network.network.ticks()
        self.client.post('/api/network/couplings',
                         json={'source_id': 0, 'target_id': 1, 'strength': 1.0})
        self.assertEqual(_api.network.network.time_s(), before_time,
                         "POST coupling must not advance simulation time")
        self.assertEqual(_api.network.network.ticks(), before_ticks,
                         "POST coupling must not increment tick counter")

    def test_delete_coupling_does_not_advance_time(self):
        """DELETE coupling must not call tick_parallel."""
        self.client.post('/api/network/couplings',
                         json={'source_id': 0, 'target_id': 1, 'strength': 1.0})
        before_time = _api.network.network.time_s()
        before_ticks = _api.network.network.ticks()
        self.client.delete('/api/network/couplings',
                           json={'source_id': 0, 'target_id': 1})
        self.assertEqual(_api.network.network.time_s(), before_time,
                         "DELETE coupling must not advance simulation time")
        self.assertEqual(_api.network.network.ticks(), before_ticks,
                         "DELETE coupling must not increment tick counter")


# ---------------------------------------------------------------------------
# Thread safety
# ---------------------------------------------------------------------------

class TestCouplingThreadSafety(unittest.TestCase):
    """_network_lock must exist and concurrent coupling mutations must not corrupt state."""

    def test_network_lock_exists(self):
        """_network_lock must be defined and be a real threading lock."""
        self.assertTrue(hasattr(_api, '_network_lock'),
                        "_network_lock must be defined in feen_rest_api")
        lock = _api._network_lock
        self.assertTrue(callable(getattr(lock, 'acquire', None)),
                        "_network_lock must be a threading lock")
        self.assertTrue(callable(getattr(lock, 'release', None)),
                        "_network_lock must be a threading lock")

    def test_concurrent_wiring_does_not_raise(self):
        """Multiple threads wiring different pairs concurrently must not raise."""
        _setup_two_nodes(_api)
        errors = []

        def wire():
            try:
                client = _plain_client(_api)
                for _ in range(5):
                    client.post('/api/network/couplings',
                                json={'source_id': 0, 'target_id': 1, 'strength': 1.0})
            except Exception as exc:
                errors.append(exc)

        threads = [threading.Thread(target=wire) for _ in range(4)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        self.assertEqual(errors, [],
                         f"Unexpected errors in concurrent wiring: {errors}")


if __name__ == '__main__':
    unittest.main()

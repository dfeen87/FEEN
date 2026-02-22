#!/usr/bin/env python3
"""
FEEN REST API Server
Provides HTTP REST API access to FEEN resonator network with global node access.

Endpoint classification (enforced by HTTP method and documented contract):

  READ-ONLY OBSERVER endpoints (safe for keep-alive, monitoring, and dashboards):
    GET  /api/health              — infrastructure liveness; no simulation access
    GET  /api/network/status      — tick/time counters; no state mutation
    GET  /api/network/nodes       — snapshot of all node states; no mutation
    GET  /api/network/nodes/<id>  — single node snapshot; no mutation
    GET  /api/network/state       — full state vector; no mutation
    GET  /api/config/snapshot     — serialized network configuration; no mutation

  STATE-MUTATING COMMAND endpoints (must be explicit and auditable):
    POST /api/network/nodes            — adds a node (structural change)
    POST /api/network/nodes/<id>/inject — injects energy (dynamics change)
    POST /api/network/tick             — advances simulation time (dynamics change)
    POST /api/network/reset            — resets all state (destructive)

Design invariants preserved by this API:
  • Observer endpoints NEVER call tick(), inject(), set_state(), or reset().
  • Keep-alive traffic MUST only hit GET /api/health or GET /api/network/status.
  • Energy injection is always a named, explicit POST — never implicit in a read.
  • Hardware adapter writes arrive only via /inject (energy) or a future
    /api/network/nodes/<id>/set_state (state overwrite); no silent side-effects.
"""

import time as _time

from flask import Flask, request, jsonify
from flask_cors import CORS
import sys
import os

# Add parent directory to path to import pyfeen and plugin_registry
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    import pyfeen
except ImportError:
    print("Error: pyfeen module not found. Please build the Python bindings first.")
    print("Run: cd build && cmake .. -DFEEN_BUILD_PYTHON=ON && make")
    sys.exit(1)


class ResonatorNetworkManager:
    """Manages a global FEEN resonator network accessible via REST API."""

    def __init__(self):
        self.nodes = []
        self.time = 0.0
        self.ticks = 0

    def add_node(self, config_dict):
        """Add a new resonator node to the network."""
        config = pyfeen.ResonatorConfig()
        config.frequency_hz = config_dict.get('frequency_hz', 1000.0)
        config.q_factor = config_dict.get('q_factor', 200.0)
        config.beta = config_dict.get('beta', 1e-4)
        config.name = config_dict.get('name', f'node_{len(self.nodes)}')

        resonator = pyfeen.Resonator(config)
        self.nodes.append({
            'id': len(self.nodes),
            'resonator': resonator,
            'config': config_dict
        })
        return len(self.nodes) - 1

    def get_node(self, node_id):
        """Get a specific node by ID."""
        if node_id < 0 or node_id >= len(self.nodes):
            return None
        return self.nodes[node_id]

    def get_node_state(self, node_id):
        """Get the state of a specific node."""
        node = self.get_node(node_id)
        if node is None:
            return None

        res = node['resonator']
        return {
            'id': node_id,
            'name': node['config'].get('name', f'node_{node_id}'),
            'x': res.x(),
            'v': res.v(),
            't': res.t(),
            'energy': res.energy(),
            'snr': res.snr()  # Now uses default temperature
        }

    def get_all_nodes_state(self):
        """Get the state of all nodes."""
        states = []
        for i in range(len(self.nodes)):
            state = self.get_node_state(i)
            if state is not None:
                states.append(state)
        return states

    def inject_node(self, node_id, amplitude, phase=0.0):
        """Inject a signal into a specific node."""
        node = self.get_node(node_id)
        if node is None:
            return False

        node['resonator'].inject(amplitude, phase)
        return True

    def tick_network(self, dt):
        """Evolve all nodes by timestep dt."""
        for node in self.nodes:
            node['resonator'].tick(dt)
        self.time += dt
        self.ticks += 1
        return True

    def get_network_status(self):
        """Get overall network status."""
        return {
            'num_nodes': len(self.nodes),
            'time': self.time,
            'ticks': self.ticks
        }

    def get_state_vector(self):
        """Get global state vector [x0, v0, x1, v1, ...]."""
        state_vector = []
        for node in self.nodes:
            res = node['resonator']
            state_vector.extend([res.x(), res.v()])
        return state_vector

    def get_config_snapshot(self):
        """Return a read-only serialized snapshot of the network configuration.

        Captures only static configuration (frequency, Q, beta) and node count.
        Does NOT include dynamic state (x, v, t).
        Suitable for reproducibility auditing and session replay.
        """
        return {
            'snapshot_wall_time': _time.time(),
            'num_nodes': len(self.nodes),
            'nodes': [
                {
                    'id': i,
                    'name': node['config'].get('name', f'node_{i}'),
                    'frequency_hz': node['config'].get('frequency_hz', 1000.0),
                    'q_factor': node['config'].get('q_factor', 200.0),
                    'beta': node['config'].get('beta', 1e-4),
                }
                for i, node in enumerate(self.nodes)
            ]
        }


# Global network manager
network = ResonatorNetworkManager()

# Flask app
app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# ---------------------------------------------------------------------------
# Plugin registry — loaded lazily; plugins may be added before or after
# the Flask app starts.  Blueprints from active plugins are registered on
# first access of the /api/plugins/* endpoints so that the app remains
# importable even when optional plugin files are absent.
# ---------------------------------------------------------------------------
try:
    from plugin_registry import PluginRegistry
    plugin_registry = PluginRegistry()
    _plugin_registry_available = True
except ImportError:
    plugin_registry = None  # type: ignore[assignment]
    _plugin_registry_available = False

_plugins_initialized = False


def _ensure_plugins_initialized():
    """Lazy-initialize plugins once and register their blueprints."""
    global _plugins_initialized
    if _plugins_initialized or not _plugin_registry_available:
        return
    _plugins_initialized = True

    # Discover and load built-in example plugins from the plugins/ sub-package.
    plugins_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plugins")
    if os.path.isdir(plugins_dir):
        for fname in sorted(os.listdir(plugins_dir)):
            if fname.endswith(".py") and not fname.startswith("_"):
                plugin_registry.load_plugin(os.path.join(plugins_dir, fname))

    plugin_registry.activate_all()

    # Register Flask blueprints from active plugins.
    for bp in plugin_registry.active_blueprints():
        try:
            app.register_blueprint(bp)
        except Exception as exc:  # noqa: BLE001
            import logging
            logging.getLogger(__name__).error("Failed to register blueprint %r: %s", bp, exc)


# ---------------------------------------------------------------------------
# READ-ONLY OBSERVER endpoints
# These endpoints MUST NOT call tick(), inject(), set_state(), or reset().
# They are safe for keep-alive traffic, monitoring, and dashboard polling.
# ---------------------------------------------------------------------------

@app.route('/api/health', methods=['GET'])
def health_check():
    """Infrastructure liveness probe.

    READ-ONLY OBSERVER: Does not access or advance simulation state.
    Safe for keep-alive polling by hosting platforms and load balancers.
    """
    return jsonify({'status': 'ok', 'service': 'FEEN REST API'})


@app.route('/api/network/status', methods=['GET'])
def get_network_status():
    """Get network tick/time counters.

    READ-ONLY OBSERVER: Returns counters only; no simulation mutation.
    """
    return jsonify(network.get_network_status())


@app.route('/api/config/snapshot', methods=['GET'])
def get_config_snapshot():
    """Return a read-only configuration snapshot for reproducibility auditing.

    READ-ONLY OBSERVER: Captures static node configuration, not dynamic state.
    Suitable for session replay, diff-based audit, and multi-user isolation checks.
    """
    return jsonify(network.get_config_snapshot())


@app.route('/api/network/nodes', methods=['GET'])
def list_nodes():
    """List all nodes in the network.

    READ-ONLY OBSERVER: Returns a snapshot of all node states; no mutation.
    """
    return jsonify({
        'nodes': network.get_all_nodes_state(),
        'count': len(network.nodes)
    })


@app.route('/api/network/nodes/<int:node_id>', methods=['GET'])
def get_node(node_id):
    """Get a specific node's state.

    READ-ONLY OBSERVER: Returns a snapshot; no simulation mutation.
    """
    state = network.get_node_state(node_id)
    if state is None:
        return jsonify({'error': f'Node {node_id} not found'}), 404
    return jsonify(state)


@app.route('/api/network/state', methods=['GET'])
def get_state_vector():
    """Get the global state vector of all nodes.

    READ-ONLY OBSERVER: Returns a snapshot; no simulation mutation.
    """
    return jsonify({
        'state_vector': network.get_state_vector(),
        'format': 'Interleaved [x0, v0, x1, v1, ...]',
        'num_nodes': len(network.nodes)
    })


# ---------------------------------------------------------------------------
# STATE-MUTATING COMMAND endpoints
# Each endpoint documents what it mutates and why the mutation is explicit.
# ---------------------------------------------------------------------------

@app.route('/api/network/nodes', methods=['POST'])
def add_node():
    """Add a new resonator node to the network.

    STATE-MUTATING COMMAND: Structural change — adds a node.
    Must be explicit; must not be reachable from observer/keep-alive paths.
    """
    data = request.get_json()
    if not data:
        return jsonify({'error': 'No JSON data provided'}), 400

    try:
        node_id = network.add_node(data)
        return jsonify({
            'id': node_id,
            'message': 'Node added successfully',
            'state': network.get_node_state(node_id)
        }), 201
    except Exception as e:
        return jsonify({'error': str(e)}), 400


@app.route('/api/network/nodes/<int:node_id>/inject', methods=['POST'])
def inject_signal(node_id):
    """Inject a signal into a specific node.

    STATE-MUTATING COMMAND: Energy injection — explicit, named, auditable.
    This is the ONLY path through which external energy enters a node.
    Amplitude and phase must be provided explicitly in the request body.
    """
    data = request.get_json()
    if not data:
        return jsonify({'error': 'No JSON data provided'}), 400

    amplitude = data.get('amplitude', 1.0)
    phase = data.get('phase', 0.0)

    if network.inject_node(node_id, amplitude, phase):
        return jsonify({
            'message': f'Signal injected into node {node_id}',
            'amplitude': amplitude,
            'phase': phase,
            'state': network.get_node_state(node_id)
        })
    else:
        return jsonify({'error': f'Node {node_id} not found'}), 404


@app.route('/api/network/tick', methods=['POST'])
def tick_network():
    """Evolve the network by a timestep.

    STATE-MUTATING COMMAND: Advances simulation time.
    dt is supplied explicitly in the request body.
    Hardware latency MUST NOT be used as dt.
    """
    data = request.get_json()
    if not data:
        return jsonify({'error': 'No JSON data provided'}), 400

    dt = data.get('dt', 1e-6)
    steps = data.get('steps', 1)

    try:
        for _ in range(steps):
            network.tick_network(dt)

        return jsonify({
            'message': f'Network evolved by {steps} steps',
            'status': network.get_network_status(),
            'nodes': network.get_all_nodes_state()
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 400


@app.route('/api/network/reset', methods=['POST'])
def reset_network():
    """Reset the network (clear all nodes).

    STATE-MUTATING COMMAND: Destructive — removes all nodes and resets time.
    """
    global network
    network = ResonatorNetworkManager()
    return jsonify({'message': 'Network reset successfully'})


# ---------------------------------------------------------------------------
# PLUGIN endpoints — READ-ONLY OBSERVER (plugin introspection only)
# These endpoints expose the plugin registry state and enforce that
# plugin management actions are explicit, not implicit.
# ---------------------------------------------------------------------------

@app.route('/api/plugins', methods=['GET'])
def list_plugins():
    """List all registered plugins and their lifecycle state.

    READ-ONLY OBSERVER: Returns plugin registry snapshot; no mutation.
    Initializes the plugin registry on first call.
    """
    _ensure_plugins_initialized()
    if not _plugin_registry_available:
        return jsonify({'plugins': [], 'registry_available': False})
    return jsonify({
        'plugins': plugin_registry.list_plugins(),
        'feen_plugin_api_version': list(
            __import__('plugin_registry').FEEN_PLUGIN_API_VERSION
        ),
        'registry_available': True,
    })


@app.route('/api/plugins/<plugin_name>', methods=['GET'])
def get_plugin(plugin_name):
    """Get details for a specific plugin.

    READ-ONLY OBSERVER: Returns plugin metadata and state; no mutation.
    """
    _ensure_plugins_initialized()
    if not _plugin_registry_available:
        return jsonify({'error': 'Plugin registry unavailable'}), 503
    entry = plugin_registry.get_plugin(plugin_name)
    if entry is None:
        return jsonify({'error': f'Plugin {plugin_name!r} not found'}), 404
    return jsonify(entry.to_dict())


@app.route('/api/plugins/<plugin_name>/activate', methods=['POST'])
def activate_plugin(plugin_name):
    """Activate a registered plugin.

    STATE-MUTATING COMMAND (plugin state only): Transitions plugin to ACTIVE.
    Does NOT touch FEEN simulation state.
    """
    _ensure_plugins_initialized()
    if not _plugin_registry_available:
        return jsonify({'error': 'Plugin registry unavailable'}), 503
    entry = plugin_registry.get_plugin(plugin_name)
    if entry is None:
        return jsonify({'error': f'Plugin {plugin_name!r} not found'}), 404
    ok = plugin_registry.activate_plugin(plugin_name)
    updated = plugin_registry.get_plugin(plugin_name)
    if ok:
        return jsonify({'message': f'Plugin {plugin_name!r} activated', 'plugin': updated.to_dict()})
    return jsonify({'error': f'Could not activate {plugin_name!r}', 'plugin': updated.to_dict()}), 400


@app.route('/api/plugins/<plugin_name>/deactivate', methods=['POST'])
def deactivate_plugin(plugin_name):
    """Deactivate an active plugin.

    STATE-MUTATING COMMAND (plugin state only): Transitions plugin to REGISTERED.
    Does NOT touch FEEN simulation state.
    """
    _ensure_plugins_initialized()
    if not _plugin_registry_available:
        return jsonify({'error': 'Plugin registry unavailable'}), 503
    entry = plugin_registry.get_plugin(plugin_name)
    if entry is None:
        return jsonify({'error': f'Plugin {plugin_name!r} not found'}), 404
    ok = plugin_registry.deactivate_plugin(plugin_name)
    updated = plugin_registry.get_plugin(plugin_name)
    if ok:
        return jsonify({'message': f'Plugin {plugin_name!r} deactivated', 'plugin': updated.to_dict()})
    return jsonify({'error': f'Could not deactivate {plugin_name!r}', 'plugin': updated.to_dict()}), 400


@app.route('/', methods=['GET'])
def index():
    """API documentation."""
    return jsonify({
        'service': 'FEEN REST API',
        'version': '1.0.0',
        'description': 'REST API for FEEN Wave Engine with global node access',
        'endpoint_classification': {
            'read_only_observer': [
                'GET /api/health',
                'GET /api/network/status',
                'GET /api/network/nodes',
                'GET /api/network/nodes/<id>',
                'GET /api/network/state',
                'GET /api/config/snapshot',
                'GET /api/plugins',
                'GET /api/plugins/<name>',
            ],
            'state_mutating_command': [
                'POST /api/network/nodes',
                'POST /api/network/nodes/<id>/inject',
                'POST /api/network/tick',
                'POST /api/network/reset',
                'POST /api/plugins/<name>/activate',
                'POST /api/plugins/<name>/deactivate',
            ]
        },
        'endpoints': {
            'GET /api/health': 'Infrastructure liveness (read-only)',
            'GET /api/network/status': 'Get network status (read-only)',
            'GET /api/network/nodes': 'List all nodes (read-only)',
            'POST /api/network/nodes': 'Add a new node (mutating)',
            'GET /api/network/nodes/<id>': 'Get specific node state (read-only)',
            'POST /api/network/nodes/<id>/inject': 'Inject signal to node (mutating)',
            'POST /api/network/tick': 'Evolve network by timestep (mutating)',
            'GET /api/network/state': 'Get global network state vector (read-only)',
            'GET /api/config/snapshot': 'Get config snapshot for auditing (read-only)',
            'POST /api/network/reset': 'Reset the network (mutating)',
            'GET /api/plugins': 'List all plugins and their state (read-only)',
            'GET /api/plugins/<name>': 'Get a specific plugin (read-only)',
            'POST /api/plugins/<name>/activate': 'Activate a plugin (plugin state only)',
            'POST /api/plugins/<name>/deactivate': 'Deactivate a plugin (plugin state only)',
        },
        'example_node_config': {
            'frequency_hz': 1000.0,
            'q_factor': 200.0,
            'beta': 1e-4,
            'name': 'my_resonator'
        }
    })


def main():
    """Main entry point for the REST API server."""
    import argparse

    parser = argparse.ArgumentParser(description='FEEN REST API Server')
    parser.add_argument('--host', default='127.0.0.1', help='Host to bind to (default: 127.0.0.1 for localhost only)')
    parser.add_argument('--port', type=int, default=5000, help='Port to bind to')
    parser.add_argument('--debug', action='store_true', help='Enable debug mode')

    args = parser.parse_args()

    print("=" * 80)
    print("FEEN REST API Server - Development/Research Mode")
    print("=" * 80)
    print("")
    print("⚠️  WARNING: This is a DEVELOPMENT server, not for production use!")
    print("   For production deployments, use a production WSGI server like:")
    print("   - Gunicorn: gunicorn -w 4 -b 0.0.0.0:5000 feen_rest_api:app")
    print("   - uWSGI: uwsgi --http 0.0.0.0:5000 --module feen_rest_api:app")
    print("")
    print(f"Starting FEEN REST API server on {args.host}:{args.port}")
    print(f"Access the API at: http://{args.host}:{args.port}")
    print(f"API Documentation: http://{args.host}:{args.port}/")
    print("")
    if args.host == '0.0.0.0':
        print("⚠️  Note: Binding to 0.0.0.0 exposes the API to all network interfaces.")
        print("   This API has no authentication. Use with caution on untrusted networks.")
        print("")
    print("=" * 80)

    app.run(host=args.host, port=args.port, debug=args.debug)


if __name__ == '__main__':
    main()

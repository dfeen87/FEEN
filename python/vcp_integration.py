import os
import math
import random
import time
import json
try:
    import requests
except ImportError:
    requests = None

try:
    import pyfeen
except ImportError:
    pyfeen = None

# Configuration
VCP_API_URL = os.environ.get('VCP_API_URL')

def get_vcp_network_view():
    """
    Returns a view of the VCP network state, including nodes, edges, and FEEN metrics.
    If VCP_API_URL is set, fetches from the external VCP coordinator.
    Otherwise, generates a simulated/dummy network for visualization.
    """
    nodes = []
    edges = []

    # 1. Fetch from VCP if configured
    if VCP_API_URL and requests:
        try:
            # Example fetch: nodes and couplings from VCP API
            # Note: This assumes VCP exposes similar endpoints or we adapt.
            # Given VCP is distributed, maybe we query a coordinator or known peer.
            # For this integration, we assume a single entry point.
            res_nodes = requests.get(f"{VCP_API_URL}/api/network/nodes", timeout=1.0)
            if res_nodes.ok:
                data = res_nodes.json()
                # Adapt VCP node format to our view format if needed
                # Assuming VCP returns similar structure as FEEN
                nodes = data.get('nodes', [])

            res_edges = requests.get(f"{VCP_API_URL}/api/network/couplings", timeout=1.0)
            if res_edges.ok:
                data = res_edges.json()
                edges = data.get('couplings', [])
        except Exception as e:
            print(f"VCP fetch failed: {e}")
            # Fallback to dummy data will happen below if empty

    # 2. Fallback to dummy data if fetch failed or no URL
    if not nodes:
        num_nodes = 6
        t = time.time()
        for i in range(num_nodes):
            phase = t * 2.0 + i
            x = math.sin(phase)
            v = math.cos(phase)

            nodes.append({
                'id': i + 1,
                'name': f'VCP Node {i + 1}',
                'type': 'feen_resonator',
                'state': {'x': x, 'v': v, 't': t},
                'status': 'active'
            })

        # Create random edges
        for i in range(num_nodes):
            target = (i + 1) % num_nodes
            strength = 0.5 + 0.5 * math.sin(t * 0.5 + i)
            edges.append({
                'source': nodes[i]['id'],
                'target': nodes[target]['id'],
                'strength': strength
            })

    # 3. Compute FEEN-powered metrics for all edges
    # We use pyfeen to compute physical properties of the connection.
    # Metrics:
    # - Resonance: Energy transfer efficiency / phase alignment
    # - Interference: Constructive vs Destructive
    # - Stability: Relative velocity / energy mismatch
    # - Delta v: Accumulated change (simulated increment)

    for edge in edges:
        # Find source and target nodes
        n1 = next((n for n in nodes if n['id'] == edge['source']), None)
        n2 = next((n for n in nodes if n['id'] == edge['target']), None)

        if n1 and n2:
            metrics = {}
            x1 = n1['state'].get('x', 0.0)
            v1 = n1['state'].get('v', 0.0)
            x2 = n2['state'].get('x', 0.0)
            v2 = n2['state'].get('v', 0.0)
            k = edge.get('strength', 0.0)

            if pyfeen:
                # Use FEEN physics engine
                # Create temporary resonators to compute potential/energy
                # We reuse a default config for metrics estimation
                cfg = pyfeen.ResonatorConfig()
                cfg.frequency_hz = 1000.0 # Assumption for metric baseline
                cfg.q_factor = 200.0

                r1 = pyfeen.Resonator(cfg)
                r1.set_state(x1, v1, 0.0)

                r2 = pyfeen.Resonator(cfg)
                r2.set_state(x2, v2, 0.0)

                # Resonance: SNR ratio or energy alignment?
                # Let's use phase alignment.
                # Phase is atan2(-v/omega, x) roughly.
                # Or just simple energy difference?
                e1 = r1.energy()
                e2 = r2.energy()
                # Resonance is high if energies are matched?
                # Or if they are in phase?
                # Let's use normalized energy difference as mismatch.
                resonance = 1.0 - (abs(e1 - e2) / (e1 + e2 + 1e-9))

                # Interference: Force = k*(x2 - x1)
                # Power = Force * v1
                # If Power > 0, energy is entering -> constructive?
                # If Power < 0, energy is leaving -> destructive (damping)?
                force = k * (x2 - x1)
                power = force * v1
                interference = power  # Positive = gain, Negative = loss

                # Stability: Based on switching time or barrier height?
                # If bistable, this matters. If monostable, less so.
                # Let's use relative velocity stability.
                stability = 1.0 / (1.0 + abs(v1 - v2))

                # Delta v: Use AileeMetric
                # We simulate one tick of delta v accumulation
                params = pyfeen.ailee.AileeParams()
                metric = pyfeen.ailee.AileeMetric(params)
                sample = pyfeen.ailee.AileeSample()
                sample.p_input = force * v1 # Work done by coupling
                sample.velocity = v1
                sample.mass = 1.0
                sample.dt = 0.001 # Simulated dt
                metric.integrate(sample)
                delta_v = metric.delta_v()

            else:
                # Fallback math
                resonance = 1.0 - min(1.0, abs(x1 - x2))
                interference = (x1 + x2) * 0.5
                stability = 1.0 / (1.0 + abs(v1 - v2))
                delta_v = 0.01 * (abs(x1) + abs(x2))

            metrics['resonance'] = float(resonance)
            metrics['interference'] = float(interference)
            metrics['stability'] = float(stability)
            metrics['delta_v'] = float(delta_v)

            edge['metrics'] = metrics

    return {
        'nodes': nodes,
        'edges': edges,
        'timestamp': time.time()
    }

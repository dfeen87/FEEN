#!/usr/bin/env python3
"""
Example client demonstrating FEEN REST API usage.
"""

import requests
import json
import time


class FEENClient:
    """Client for FEEN REST API."""
    
    def __init__(self, base_url='http://localhost:5000'):
        self.base_url = base_url
        
    def _get(self, endpoint):
        """Send GET request."""
        response = requests.get(f'{self.base_url}{endpoint}')
        response.raise_for_status()
        return response.json()
    
    def _post(self, endpoint, data=None):
        """Send POST request."""
        response = requests.post(
            f'{self.base_url}{endpoint}',
            json=data,
            headers={'Content-Type': 'application/json'}
        )
        response.raise_for_status()
        return response.json()
    
    def health_check(self):
        """Check API health."""
        return self._get('/api/health')
    
    def get_network_status(self):
        """Get network status."""
        return self._get('/api/network/status')
    
    def list_nodes(self):
        """List all nodes."""
        return self._get('/api/network/nodes')
    
    def add_node(self, frequency_hz=1000.0, q_factor=200.0, beta=1e-4, name=None):
        """Add a new node."""
        config = {
            'frequency_hz': frequency_hz,
            'q_factor': q_factor,
            'beta': beta
        }
        if name:
            config['name'] = name
        return self._post('/api/network/nodes', config)
    
    def get_node(self, node_id):
        """Get a specific node's state."""
        return self._get(f'/api/network/nodes/{node_id}')
    
    def inject_signal(self, node_id, amplitude=1.0, phase=0.0):
        """Inject a signal into a node."""
        return self._post(
            f'/api/network/nodes/{node_id}/inject',
            {'amplitude': amplitude, 'phase': phase}
        )
    
    def tick(self, dt=1e-6, steps=1):
        """Evolve the network."""
        return self._post('/api/network/tick', {'dt': dt, 'steps': steps})
    
    def get_state_vector(self):
        """Get global state vector."""
        return self._get('/api/network/state')
    
    def reset_network(self):
        """Reset the network."""
        return self._post('/api/network/reset')


def main():
    """Demonstrate REST API usage."""
    print("FEEN REST API Client Demo")
    print("=" * 50)
    
    client = FEENClient()
    
    # Health check
    print("\n1. Health Check:")
    print(json.dumps(client.health_check(), indent=2))
    
    # Reset network
    print("\n2. Reset Network:")
    print(json.dumps(client.reset_network(), indent=2))
    
    # Add nodes
    print("\n3. Adding 3 Resonator Nodes:")
    node1 = client.add_node(frequency_hz=1000.0, q_factor=200.0, beta=1e-4, name='osc_1kHz')
    print(f"   Node 1: {node1['id']} - {node1['state']['name']}")
    
    node2 = client.add_node(frequency_hz=1010.0, q_factor=200.0, beta=1e-4, name='osc_1010Hz')
    print(f"   Node 2: {node2['id']} - {node2['state']['name']}")
    
    node3 = client.add_node(frequency_hz=1020.0, q_factor=500.0, beta=-1e8, name='bistable')
    print(f"   Node 3: {node3['id']} - {node3['state']['name']} (bistable)")
    
    # Check network status
    print("\n4. Network Status:")
    status = client.get_network_status()
    print(json.dumps(status, indent=2))
    
    # Inject signals
    print("\n5. Injecting Signals:")
    client.inject_signal(0, amplitude=1.0, phase=0.0)
    print("   Injected 1.0 amplitude into node 0")
    
    client.inject_signal(1, amplitude=0.5, phase=0.0)
    print("   Injected 0.5 amplitude into node 1")
    
    # Get individual node states
    print("\n6. Node States After Injection:")
    for i in range(3):
        node = client.get_node(i)
        print(f"   Node {i}: x={node['x']:.6f}, v={node['v']:.6f}, E={node['energy']:.6f}")
    
    # Evolve network
    print("\n7. Evolving Network (1000 steps):")
    result = client.tick(dt=1e-6, steps=1000)
    print(f"   Time: {result['status']['time']:.6f} s")
    print(f"   Ticks: {result['status']['ticks']}")
    
    # Get updated states
    print("\n8. Node States After Evolution:")
    for i in range(3):
        node = client.get_node(i)
        print(f"   Node {i}: x={node['x']:.6f}, v={node['v']:.6f}, E={node['energy']:.6f}, SNR={node['snr']:.2f}")
    
    # Get global state vector
    print("\n9. Global State Vector:")
    state_vec = client.get_state_vector()
    print(f"   Format: {state_vec['format']}")
    print(f"   Vector: {state_vec['state_vector']}")
    
    # List all nodes
    print("\n10. All Nodes Summary:")
    nodes = client.list_nodes()
    print(f"   Total nodes: {nodes['count']}")
    for node in nodes['nodes']:
        print(f"   - {node['name']}: E={node['energy']:.6e} J, SNR={node['snr']:.2f}")
    
    print("\n" + "=" * 50)
    print("Demo completed successfully!")


if __name__ == '__main__':
    main()

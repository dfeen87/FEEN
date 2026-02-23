# FEEN REST API Documentation

## Overview

The FEEN REST API provides HTTP access to the FEEN Wave Engine's resonator network with global node access. This allows you to create, manage, and interact with phononic resonator networks over HTTP.

## Quick Start

### Installation

1. Build FEEN with Python bindings:
```bash
cd build
cmake .. -DFEEN_BUILD_PYTHON=ON
make
```

2. Install Python dependencies:
```bash
cd ../python
pip install -r requirements.txt
```

3. Start the REST API server:
```bash
python3 feen_rest_api.py
```

The server will start on `http://localhost:5000` by default.

### Command Line Options

```bash
python3 feen_rest_api.py --help

Options:
  --host HOST      Host to bind to (default: 127.0.0.1 for localhost only)
  --port PORT      Port to bind to (default: 5000)
  --debug          Enable debug mode
```

Example:
```bash
# Start on localhost (default, secure)
python3 feen_rest_api.py

# Bind to all interfaces (use with caution)
python3 feen_rest_api.py --host 0.0.0.0 --port 8080 --debug
```

## API Endpoints

### Health Check

**GET /api/health**

Check if the API is running.

**Response:**
```json
{
  "status": "ok",
  "service": "FEEN REST API"
}
```

### Network Status

**GET /api/network/status**

Get the current network status.

**Response:**
```json
{
  "num_nodes": 3,
  "time": 0.001,
  "ticks": 1000
}
```

### List All Nodes

**GET /api/network/nodes**

Get a list of all resonator nodes in the network.

**Response:**
```json
{
  "nodes": [
    {
      "id": 0,
      "name": "osc_1kHz",
      "x": 0.367879,
      "v": -2309.45,
      "t": 0.001,
      "energy": 0.135,
      "snr": 982345.2
    }
  ],
  "count": 1
}
```

### Add Node

**POST /api/network/nodes**

Add a new resonator node to the network.

**Request Body:**
```json
{
  "frequency_hz": 1000.0,
  "q_factor": 200.0,
  "beta": 1e-4,
  "name": "my_resonator"
}
```

**Parameters:**
- `frequency_hz` (required): Natural frequency in Hz
- `q_factor` (required): Quality factor
- `beta` (required): Nonlinearity coefficient (positive = monostable, negative = bistable)
- `name` (optional): Human-readable name

**Response:**
```json
{
  "id": 0,
  "message": "Node added successfully",
  "state": {
    "id": 0,
    "name": "my_resonator",
    "x": 0.0,
    "v": 0.0,
    "t": 0.0,
    "energy": 0.0,
    "snr": 0.0
  }
}
```

### Get Node State

**GET /api/network/nodes/{id}**

Get the state of a specific node.

**Response:**
```json
{
  "id": 0,
  "name": "my_resonator",
  "x": 0.367879,
  "v": -2309.45,
  "t": 0.001,
  "energy": 0.135,
  "snr": 982345.2
}
```

**State Fields:**
- `x`: Displacement from equilibrium (m)
- `v`: Velocity (m/s)
- `t`: Current time (s)
- `energy`: Total mechanical energy (J)
- `snr`: Signal-to-noise ratio

### Inject Signal

**POST /api/network/nodes/{id}/inject**

Inject a signal into a specific node.

**Request Body:**
```json
{
  "amplitude": 1.0,
  "phase": 0.0
}
```

**Parameters:**
- `amplitude` (optional, default: 1.0): Signal amplitude
- `phase` (optional, default: 0.0): Signal phase in radians

**Response:**
```json
{
  "message": "Signal injected into node 0",
  "state": {
    "id": 0,
    "name": "my_resonator",
    "x": 1.0,
    "v": 0.0,
    "t": 0.0,
    "energy": 0.5,
    "snr": 36347.2
  }
}
```

### Tick Network

**POST /api/network/tick**

Evolve the entire network by one or more timesteps.

**Request Body:**
```json
{
  "dt": 1e-6,
  "steps": 1000
}
```

**Parameters:**
- `dt` (optional, default: 1e-6): Timestep in seconds
- `steps` (optional, default: 1): Number of steps to evolve

**Response:**
```json
{
  "message": "Network evolved by 1000 steps",
  "status": {
    "num_nodes": 3,
    "time": 0.001,
    "ticks": 1000
  },
  "nodes": [...]
}
```

### Get State Vector

**GET /api/network/state**

Get the global state vector of all nodes in the network.

**Response:**
```json
{
  "state_vector": [0.367879, -2309.45, 0.5, -3141.59, 0.2, -1256.64],
  "format": "Interleaved [x0, v0, x1, v1, ...]",
  "num_nodes": 3
}
```

The state vector is in interleaved format: `[x0, v0, x1, v1, x2, v2, ...]`

### Reset Network

**POST /api/network/reset**

Reset the network, clearing all nodes.

**Response:**
```json
{
  "message": "Network reset successfully"
}
```

## Usage Examples

### Example 1: Create and Simulate a Monostable Resonator

```bash
# Add a 1 kHz resonator
curl -X POST http://localhost:5000/api/network/nodes \
  -H "Content-Type: application/json" \
  -d '{
    "frequency_hz": 1000.0,
    "q_factor": 200.0,
    "beta": 1e-4,
    "name": "osc_1kHz"
  }'

# Inject a signal
curl -X POST http://localhost:5000/api/network/nodes/0/inject \
  -H "Content-Type: application/json" \
  -d '{
    "amplitude": 1.0,
    "phase": 0.0
  }'

# Simulate for 1ms (1000 steps at 1μs)
curl -X POST http://localhost:5000/api/network/tick \
  -H "Content-Type: application/json" \
  -d '{
    "dt": 1e-6,
    "steps": 1000
  }'

# Check the state
curl http://localhost:5000/api/network/nodes/0
```

### Example 2: Frequency Multiplexing

```python
import requests

# Add 10 resonators at different frequencies
for i in range(10):
    freq = 1000.0 + i * 10.0  # 1000, 1010, 1020, ... Hz
    requests.post('http://localhost:5000/api/network/nodes', json={
        'frequency_hz': freq,
        'q_factor': 1000.0,
        'beta': 1e-4,
        'name': f'channel_{i}'
    })

# Inject signals with different amplitudes
for i in range(10):
    requests.post(f'http://localhost:5000/api/network/nodes/{i}/inject', json={
        'amplitude': 0.1 * (i + 1),
        'phase': 0.0
    })

# Evolve the network
requests.post('http://localhost:5000/api/network/tick', json={
    'dt': 1e-6,
    'steps': 10000
})

# Get global state
response = requests.get('http://localhost:5000/api/network/state')
state = response.json()
print(f"State vector: {state['state_vector']}")
```

### Example 3: Bistable Logic

```python
import requests

# Create a bistable resonator
requests.post('http://localhost:5000/api/network/nodes', json={
    'frequency_hz': 1000.0,
    'q_factor': 500.0,
    'beta': -1e8,  # Negative beta = bistable
    'name': 'bistable_bit'
})

# Set to HIGH state
requests.post('http://localhost:5000/api/network/nodes/0/inject', json={
    'amplitude': 0.01,  # Initial displacement to kick into HIGH well
    'phase': 0.0
})

# Let it settle
requests.post('http://localhost:5000/api/network/tick', json={
    'dt': 1e-6,
    'steps': 10000
})

# Read state
response = requests.get('http://localhost:5000/api/network/nodes/0')
print(f"Final state: {response.json()}")
```

## Global Node Access

The REST API provides **global node access** through:

1. **GET /api/network/state** - Access the complete state vector of all nodes simultaneously
2. **GET /api/network/nodes** - List all nodes with their current states
3. **POST /api/network/tick** - Evolve all nodes synchronously in a single operation

This enables:
- Parallel readout of entire network state
- Synchronized evolution of coupled resonator systems
- Batch operations on multiple nodes
- Real-time monitoring of network dynamics

## Python Client Library

Use the provided `FEENClient` class for easier interaction:

```python
from examples.rest_api_demo import FEENClient

client = FEENClient('http://localhost:5000')

# Add nodes
client.add_node(frequency_hz=1000.0, q_factor=200.0, beta=1e-4)
client.inject_signal(0, amplitude=1.0)
client.tick(dt=1e-6, steps=1000)

# Get state
state = client.get_node(0)
print(f"Energy: {state['energy']}")
```

## Error Handling

The API returns standard HTTP status codes:

- `200 OK`: Request successful
- `201 Created`: Node created successfully
- `400 Bad Request`: Invalid request data
- `404 Not Found`: Node not found
- `500 Internal Server Error`: Server error

Error responses include a JSON object with an `error` field:

```json
{
  "error": "Node 99 not found"
}
```

## Integration with FEEN

The REST API is built on top of FEEN's Python bindings (`pyfeen`) and provides HTTP access to:

- **Resonator class**: Individual Duffing oscillators
- **ResonatorConfig**: Configuration for resonator parameters
- **Network operations**: Global access to multiple resonators

All physics is computed using FEEN's validated RK4 integration and thermal noise models.

## Performance Considerations

- Each node tick operation involves RK4 integration (~120 ns per step)
- Network state operations scale linearly with number of nodes
- For high-performance applications, consider batching tick operations
- Use the `steps` parameter in `/api/network/tick` for efficient multi-step evolution

## Security Notes

This is a development/research API. For production use, consider:

- Rate limiting
- Input validation
- HTTPS encryption
- Network access controls

## See Also

- [FEEN Documentation](../docs/FEEN.md)
- [Python Bindings](pyfeen.cpp)
- [Examples](examples/)

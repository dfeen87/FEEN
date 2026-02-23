import sys
import os
from flask import Flask, send_from_directory, render_template, redirect

# Ensure we can import from python/ directory
python_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'python'))
if python_dir not in sys.path:
    sys.path.append(python_dir)

try:
    from feen_rest_api import app, network
except ImportError as e:
    print(f"Error: Could not import feen_rest_api from {python_dir}")
    print(f"Details: {e}")
    sys.exit(1)

# Set up template and static folders relative to this file
base_dir = os.path.dirname(os.path.abspath(__file__))
app.template_folder = os.path.join(base_dir, 'templates')
app.static_folder = os.path.join(base_dir, 'static')

@app.route('/')
def root():
    """Serve the homepage dashboard."""
    return render_template('dashboard.html')


@app.route('/dashboard')
def dashboard():
    """Alias for the homepage dashboard."""
    return render_template('dashboard.html')


@app.route('/simulation')
def simulation():
    """Serve the main simulation page."""
    return render_template('index.html')


@app.route('/node-graph')
def node_graph():
    """Serve the node-graph plugin visualization page."""
    return render_template('node_graph.html')


@app.route('/ailee-metric')
def ailee_metric():
    """Serve the AILEE Delta v Metric visualization page."""
    return render_template('ailee_metric.html')


@app.route('/coupling')
def coupling():
    """Serve the Node Coupling visualization page."""
    return render_template('coupling.html')


@app.route('/vcp-wiring')
def vcp_wiring():
    """Serve the authenticated VCP Wiring page."""
    return render_template('vcp_wiring.html')


@app.route('/vcp-connectivity')
def vcp_connectivity():
    """Serve the VCP Connectivity visualization page."""
    return render_template('vcp_connectivity.html')


@app.route('/hlv-lab')
def hlv_lab():
    """Serve the HLV Dynamics Lab page."""
    return render_template('hlv_lab.html')


@app.route('/docs')
def api_docs():
    """Serve the human-readable API documentation page."""
    return render_template('docs.html')

@app.route('/hardware')
def hardware_link():
    """Serve the Hardware Link Bluetooth bridge page."""
    return render_template('hardware.html')

# ── Hardware Link API ───────────────────────────────────────────────────────
# Minimal, deterministic Bluetooth bridge endpoints.
# In production these would delegate to a real BT adapter; here they maintain
# a simple in-process state dict so the page works without hardware present.

import threading as _threading
import time as _time

_hw_state = {
    'scanning': False,
    'scan_results': [],      # list of {name, addr, rssi}
    'paired': {},            # addr -> {name, addr, rssi}
    'streams': {},           # key -> {value, ts}
}
_hw_lock = _threading.Lock()

@app.route('/api/hardware/status')
def hw_status():
    from flask import jsonify
    with _hw_lock:
        return jsonify({
            'scanning': _hw_state['scanning'],
            'paired_count': len(_hw_state['paired']),
            'devices': list(_hw_state['paired'].values()),
        })

@app.route('/api/hardware/scan/start', methods=['POST'])
def hw_scan_start():
    from flask import jsonify
    with _hw_lock:
        _hw_state['scanning'] = True
        _hw_state['scan_results'] = []
    return jsonify({'ok': True})

@app.route('/api/hardware/scan/stop', methods=['POST'])
def hw_scan_stop():
    from flask import jsonify
    with _hw_lock:
        _hw_state['scanning'] = False
    return jsonify({'ok': True})

@app.route('/api/hardware/scan/results')
def hw_scan_results():
    from flask import jsonify
    with _hw_lock:
        return jsonify({'devices': list(_hw_state['scan_results'])})

@app.route('/api/hardware/pair', methods=['POST'])
def hw_pair():
    from flask import jsonify, request
    data = request.get_json(force=True) or {}
    addr = data.get('addr', '').strip()
    name = data.get('name', addr)
    if not addr:
        return jsonify({'ok': False, 'error': 'addr required'}), 400
    with _hw_lock:
        _hw_state['paired'][addr] = {'name': name, 'addr': addr, 'rssi': -65}
    return jsonify({'ok': True, 'addr': addr, 'rssi': -65})

@app.route('/api/hardware/unpair', methods=['POST'])
def hw_unpair():
    from flask import jsonify, request
    data = request.get_json(force=True) or {}
    addr = data.get('addr', '').strip()
    with _hw_lock:
        _hw_state['paired'].pop(addr, None)
        # Remove streams for this device
        for k in list(_hw_state['streams'].keys()):
            if k.startswith(addr + ':'):
                del _hw_state['streams'][k]
    return jsonify({'ok': True})

@app.route('/api/hardware/streams')
def hw_streams():
    from flask import jsonify
    import math
    t = _time.time()
    with _hw_lock:
        streams = {}
        for addr, dev in _hw_state['paired'].items():
            streams[addr + ':temp']    = {'value': round(20 + 5 * math.sin(t / 4), 2),  'ts': _time.strftime('%Y-%m-%dT%H:%M:%SZ', _time.gmtime(t))}
            streams[addr + ':accel_x'] = {'value': round(0.1 * math.sin(t / 1.2), 4),   'ts': _time.strftime('%Y-%m-%dT%H:%M:%SZ', _time.gmtime(t))}
            streams[addr + ':accel_y'] = {'value': round(0.1 * math.cos(t / 1.2), 4),   'ts': _time.strftime('%Y-%m-%dT%H:%M:%SZ', _time.gmtime(t))}
            streams[addr + ':rssi']    = {'value': dev['rssi'],                           'ts': _time.strftime('%Y-%m-%dT%H:%M:%SZ', _time.gmtime(t))}
        return jsonify({'streams': streams})

@app.route('/api/hardware/send', methods=['POST'])
def hw_send():
    from flask import jsonify, request
    data = request.get_json(force=True) or {}
    addr  = data.get('addr', '').strip()
    value = data.get('value', '')
    if not addr:
        return jsonify({'ok': False, 'error': 'addr required'}), 400
    with _hw_lock:
        if addr not in _hw_state['paired']:
            return jsonify({'ok': False, 'error': 'device not paired'}), 404
        # Record the last sent value as a stream entry for deterministic echo
        _hw_state['streams'][addr + ':tx'] = {
            'value': str(value),
            'ts': _time.strftime('%Y-%m-%dT%H:%M:%SZ', _time.gmtime()),
        }
    return jsonify({'ok': True, 'addr': addr, 'value': value})

@app.route('/static/<path:filename>')
def custom_static(filename):
    """Serve static files from web/static."""
    return send_from_directory(app.static_folder, filename)

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    # In production (Render), gunicorn will be used, so app.run is not called.
    # But for local dev:
    app.run(host='0.0.0.0', port=port)

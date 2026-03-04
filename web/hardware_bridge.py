"""
Hardware Link Bluetooth bridge — Flask Blueprint.

Minimal, deterministic Bluetooth bridge endpoints. In production these would
delegate to a real BT adapter; here they maintain a simple in-process state
dict so the page works without hardware present.
"""

import math
import time as _time
import threading as _threading

from flask import Blueprint, jsonify, request

hardware_bp = Blueprint('hardware', __name__)

_hw_state = {
    'scanning': False,
    'scan_results': [],   # list of {name, addr, rssi}
    'paired': {},         # addr -> {name, addr, rssi}
    'streams': {},        # key -> {value, ts}
}
_hw_lock = _threading.Lock()


@hardware_bp.route('/api/hardware/status')
def hw_status():
    with _hw_lock:
        return jsonify({
            'scanning': _hw_state['scanning'],
            'paired_count': len(_hw_state['paired']),
            'devices': list(_hw_state['paired'].values()),
        })


@hardware_bp.route('/api/hardware/scan/start', methods=['POST'])
def hw_scan_start():
    with _hw_lock:
        _hw_state['scanning'] = True
        _hw_state['scan_results'] = []
    return jsonify({'ok': True})


@hardware_bp.route('/api/hardware/scan/stop', methods=['POST'])
def hw_scan_stop():
    with _hw_lock:
        _hw_state['scanning'] = False
    return jsonify({'ok': True})


@hardware_bp.route('/api/hardware/scan/results')
def hw_scan_results():
    with _hw_lock:
        return jsonify({'devices': list(_hw_state['scan_results'])})


@hardware_bp.route('/api/hardware/pair', methods=['POST'])
def hw_pair():
    data = request.get_json(force=True) or {}
    addr = data.get('addr', '').strip()
    name = data.get('name', addr)
    if not addr:
        return jsonify({'ok': False, 'error': 'addr required'}), 400
    with _hw_lock:
        _hw_state['paired'][addr] = {'name': name, 'addr': addr, 'rssi': -65}
    return jsonify({'ok': True, 'addr': addr, 'rssi': -65})


@hardware_bp.route('/api/hardware/unpair', methods=['POST'])
def hw_unpair():
    data = request.get_json(force=True) or {}
    addr = data.get('addr', '').strip()
    with _hw_lock:
        _hw_state['paired'].pop(addr, None)
        for k in list(_hw_state['streams'].keys()):
            if k.startswith(addr + ':'):
                del _hw_state['streams'][k]
    return jsonify({'ok': True})


@hardware_bp.route('/api/hardware/streams')
def hw_streams():
    t = _time.time()
    with _hw_lock:
        streams = {}
        for addr, dev in _hw_state['paired'].items():
            ts = _time.strftime('%Y-%m-%dT%H:%M:%SZ', _time.gmtime(t))
            streams[addr + ':temp']    = {'value': round(20 + 5 * math.sin(t / 4), 2),  'ts': ts}
            streams[addr + ':accel_x'] = {'value': round(0.1 * math.sin(t / 1.2), 4),   'ts': ts}
            streams[addr + ':accel_y'] = {'value': round(0.1 * math.cos(t / 1.2), 4),   'ts': ts}
            streams[addr + ':rssi']    = {'value': dev['rssi'],                           'ts': ts}
        return jsonify({'streams': streams})


@hardware_bp.route('/api/hardware/send', methods=['POST'])
def hw_send():
    data = request.get_json(force=True) or {}
    addr  = data.get('addr', '').strip()
    value = data.get('value', '')
    if not addr:
        return jsonify({'ok': False, 'error': 'addr required'}), 400
    with _hw_lock:
        if addr not in _hw_state['paired']:
            return jsonify({'ok': False, 'error': 'device not paired'}), 404
        _hw_state['streams'][addr + ':tx'] = {
            'value': str(value),
            'ts': _time.strftime('%Y-%m-%dT%H:%M:%SZ', _time.gmtime()),
        }
    return jsonify({'ok': True, 'addr': addr, 'value': value})

"""
Hardware Monitor Plugin — FEEN Plugin Example
==============================================

Type: OBSERVER
Permissions: read-only (GET endpoints only)

Models a hardware-adjacent instrumentation plugin that:
  • Reads node states via GET /api/network/nodes (observer boundary preserved)
  • Computes derived metrics (SNR headroom, energy drift)
  • Exposes those metrics under /plugins/hardware_monitor/metrics
  • Does NOT write to simulation state — hardware *injection* must be done
    via an explicit external POST /api/network/nodes/<id>/inject call,
    which would be classified as a TOOL plugin action.

This plugin demonstrates how hardware-adjacent instrumentation remains
strictly read-only and can be ablated without affecting physics.
"""

import logging
import time
from typing import Any, Dict, List, Optional

from flask import Blueprint, jsonify

from plugin_registry import PluginManifest, PluginType

logger = logging.getLogger(__name__)

MANIFEST = PluginManifest(
    name="hardware_monitor",
    version=(1, 0, 0),
    plugin_type=PluginType.OBSERVER,
    description=(
        "Hardware-adjacent instrumentation observer. "
        "Tracks SNR headroom and energy drift per node."
    ),
    min_feen_api=(1, 0),
    max_feen_api=(1, 99),
)

# ---------------------------------------------------------------------------
# Metrics store (observer-side only)
# ---------------------------------------------------------------------------
_latest_metrics: Optional[Dict[str, Any]] = None

# ---------------------------------------------------------------------------
# Blueprint
# ---------------------------------------------------------------------------
_blueprint = Blueprint("hardware_monitor", __name__, url_prefix="/plugins/hardware_monitor")


@_blueprint.route("/metrics", methods=["GET"])
def get_metrics():
    """Return latest derived hardware metrics — read-only endpoint."""
    if _latest_metrics is None:
        return jsonify({"status": "no_data", "metrics": None})
    return jsonify(_latest_metrics)


@_blueprint.route("/info", methods=["GET"])
def info():
    """Return plugin metadata — read-only endpoint."""
    return jsonify(MANIFEST.to_dict())


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def update_metrics(nodes_snapshot: List[Dict[str, Any]]) -> None:
    """Compute and cache derived hardware metrics from a node snapshot.

    ``nodes_snapshot`` is the parsed JSON list from GET /api/network/nodes.
    Never mutates simulation state.
    """
    global _latest_metrics

    node_metrics = []
    for n in nodes_snapshot:
        snr = n.get("snr", 0.0)
        energy = n.get("energy", 0.0)
        node_metrics.append({
            "id": n.get("id"),
            "snr": snr,
            "snr_headroom_db": max(0.0, snr - 10.0),  # margin above MIN_READABLE_SNR
            "energy": energy,
        })

    _latest_metrics = {
        "wall_time": time.time(),
        "node_count": len(node_metrics),
        "nodes": node_metrics,
    }


# ---------------------------------------------------------------------------
# Lifecycle hooks
# ---------------------------------------------------------------------------

def get_blueprint():
    return _blueprint


def activate():
    logger.info("hardware_monitor plugin activated")


def deactivate():
    global _latest_metrics
    _latest_metrics = None
    logger.info("hardware_monitor plugin deactivated")


def unload():
    pass

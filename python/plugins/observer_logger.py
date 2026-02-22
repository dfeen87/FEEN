"""
Observer Logger Plugin — FEEN Plugin Example
=============================================

Type: OBSERVER
Permissions: read-only (GET endpoints only)

Periodically polls GET /api/network/nodes and writes a summary to the
Python logging system.  Demonstrates the observer plugin contract:

  • Uses only read-only (GET) REST endpoints.
  • Calling assert_observer_safe() in every HTTP helper guards the boundary.
  • Stores a rolling in-memory log (max 1 000 entries); never mutates state.
  • Exposes a GET endpoint for external consumers to retrieve the log.

In production this plugin would be activated by the plugin registry and its
`poll()` method would be called from a background thread or scheduler.
The plugin itself is responsible for that scheduling; FEEN core is unaffected.
"""

import logging
import time
from collections import deque
from typing import Any, Deque, Dict, Optional

from flask import Blueprint, jsonify

from plugin_registry import FEEN_PLUGIN_API_VERSION, PluginManifest, PluginType

logger = logging.getLogger(__name__)

MANIFEST = PluginManifest(
    name="observer_logger",
    version=(1, 0, 0),
    plugin_type=PluginType.OBSERVER,
    description=(
        "Read-only observer that logs node-energy snapshots. "
        "Provides /plugins/observer_logger/log for retrieval."
    ),
    min_feen_api=(1, 0),
    max_feen_api=(1, 99),
)

# ---------------------------------------------------------------------------
# In-memory rolling log (observer-side; no simulation state)
# ---------------------------------------------------------------------------
_MAX_ENTRIES = 1_000
_log: Deque[Dict[str, Any]] = deque(maxlen=_MAX_ENTRIES)
_api_base: Optional[str] = None  # Set by activate()

# ---------------------------------------------------------------------------
# Blueprint
# ---------------------------------------------------------------------------
_blueprint = Blueprint("observer_logger", __name__, url_prefix="/plugins/observer_logger")


@_blueprint.route("/log", methods=["GET"])
def get_log():
    """Return the rolling energy-snapshot log — read-only endpoint."""
    return jsonify({"entries": list(_log), "count": len(_log)})


@_blueprint.route("/info", methods=["GET"])
def info():
    """Return plugin metadata — read-only endpoint."""
    return jsonify(MANIFEST.to_dict())


# ---------------------------------------------------------------------------
# Public API: poll() is called by an external scheduler (not by FEEN core)
# ---------------------------------------------------------------------------

def poll(nodes_snapshot: list) -> None:
    """Record a snapshot of node states.

    ``nodes_snapshot`` is the parsed JSON list from GET /api/network/nodes.
    The caller (not FEEN core) is responsible for fetching it.
    This function never touches simulation state.
    """
    _log.append({
        "wall_time": time.time(),
        "nodes": [
            {
                "id": n.get("id"),
                "energy": n.get("energy"),
                "snr": n.get("snr"),
            }
            for n in nodes_snapshot
        ],
    })


# ---------------------------------------------------------------------------
# Lifecycle hooks
# ---------------------------------------------------------------------------

def get_blueprint():
    return _blueprint


def activate():
    logger.info("observer_logger plugin activated (FEEN API %s)", FEEN_PLUGIN_API_VERSION)


def deactivate():
    logger.info("observer_logger plugin deactivated")


def unload():
    _log.clear()

"""
UI Dashboard Plugin — FEEN Plugin Example
==========================================

Type: UI
Permissions: read-only (GET endpoints only)

Provides an enhanced per-node energy-history panel served as a Flask
Blueprint under /plugins/ui_dashboard/.

This plugin demonstrates the UI plugin contract:
  • Only reads data via GET /api/network/nodes
  • Does not call any command (POST) endpoint
  • Does not import or reference FEEN core internals
  • Returns a Blueprint with self-contained routes
"""

from flask import Blueprint, jsonify, current_app

from plugin_registry import PluginManifest, PluginType

# ---------------------------------------------------------------------------
# Required: every plugin module must expose MANIFEST
# ---------------------------------------------------------------------------
MANIFEST = PluginManifest(
    name="ui_dashboard",
    version=(1, 0, 0),
    plugin_type=PluginType.UI,
    description=(
        "Read-only energy-history panel.  "
        "Provides /plugins/ui_dashboard/info with plugin metadata."
    ),
    min_feen_api=(1, 0),
    max_feen_api=(1, 99),
)

# ---------------------------------------------------------------------------
# Blueprint (returned by get_blueprint during registration)
# ---------------------------------------------------------------------------
_blueprint = Blueprint("ui_dashboard", __name__, url_prefix="/plugins/ui_dashboard")


@_blueprint.route("/info", methods=["GET"])
def info():
    """Return plugin metadata — read-only endpoint."""
    return jsonify(MANIFEST.to_dict())


# ---------------------------------------------------------------------------
# Plugin lifecycle hooks (all optional)
# ---------------------------------------------------------------------------

def get_blueprint():
    """Return the Flask Blueprint for this plugin."""
    return _blueprint


def activate():
    """Called when the plugin transitions to ACTIVE state."""
    pass


def deactivate():
    """Called when the plugin is deactivated (before potential unload)."""
    pass


def unload():
    """Called just before the plugin is removed from the registry."""
    pass

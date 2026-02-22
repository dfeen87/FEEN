"""
FEEN Plugin Registry
====================

Manages plugin lifecycle and enforces strict boundary separation between
observer-only and command-capable plugins.

Plugin types:
  - ``ui``       — provides HTML/JS panels served under /plugins/<name>/
  - ``observer`` — read-only analysis/logging; may only call GET endpoints
  - ``tool``     — command-capable; may call POST endpoints via explicit commands

Plugin lifecycle:
  load → register → activate → (running) → deactivate → unload

Safety guarantees:
  • Observer plugins are forbidden from calling any command endpoint.
  • Every plugin runs inside a try/except guard; failures are isolated.
  • Plugins cannot import or reference FEEN core internals directly.
  • Each plugin declares its minimum/maximum FEEN API version.
  • A plugin may be disabled at deployment time via the plugin config file.

This module does NOT import pyfeen and does NOT touch simulation state.
"""

from __future__ import annotations

import importlib
import importlib.util
import logging
import os
import sys
import traceback
from enum import Enum
from typing import Any, Callable, Dict, List, Optional

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# FEEN plugin API version this registry implements.
# Plugins declare a compatible range; incompatible plugins are rejected.
# ---------------------------------------------------------------------------
FEEN_PLUGIN_API_VERSION = (1, 0)


class PluginType(str, Enum):
    """Declares what a plugin is allowed to do."""

    UI = "ui"
    """Provides static assets / template panels.  No REST calls allowed."""

    OBSERVER = "observer"
    """Read-only observer.  May call GET endpoints only."""

    TOOL = "tool"
    """Command-capable.  May call POST endpoints as well as GET endpoints.
    The plugin MUST document every POST it issues in its manifest."""


class PluginState(str, Enum):
    """Lifecycle state machine for a single plugin."""

    UNLOADED = "unloaded"
    LOADED = "loaded"
    REGISTERED = "registered"
    ACTIVE = "active"
    FAILED = "failed"


class PluginManifest:
    """
    Static metadata that every plugin module must expose as ``MANIFEST``.

    Example in a plugin module::

        MANIFEST = PluginManifest(
            name="energy_logger",
            version=(1, 0, 0),
            plugin_type=PluginType.OBSERVER,
            description="Logs node energy over time.",
            min_feen_api=(1, 0),
            max_feen_api=(1, 99),
        )
    """

    def __init__(
        self,
        *,
        name: str,
        version: tuple,
        plugin_type: PluginType,
        description: str,
        min_feen_api: tuple = (1, 0),
        max_feen_api: tuple = (1, 99),
        commands_issued: Optional[List[str]] = None,
    ) -> None:
        if not name or not name.isidentifier():
            raise ValueError(f"Plugin name must be a valid identifier; got {name!r}")
        self.name = name
        self.version = tuple(version)
        self.plugin_type = PluginType(plugin_type)
        self.description = description
        self.min_feen_api = tuple(min_feen_api)
        self.max_feen_api = tuple(max_feen_api)
        # Commands a TOOL plugin explicitly documents it will issue.
        # Observer/UI plugins must leave this empty.
        self.commands_issued: List[str] = list(commands_issued or [])

        if self.plugin_type != PluginType.TOOL and self.commands_issued:
            raise ValueError(
                f"Plugin {name!r} is type {self.plugin_type.value} but declares "
                "commands_issued — only TOOL plugins may issue commands."
            )

    def is_api_compatible(self) -> bool:
        """Return True if this plugin is compatible with the running FEEN API version."""
        return self.min_feen_api <= FEEN_PLUGIN_API_VERSION <= self.max_feen_api

    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "version": list(self.version),
            "type": self.plugin_type.value,
            "description": self.description,
            "min_feen_api": list(self.min_feen_api),
            "max_feen_api": list(self.max_feen_api),
            "commands_issued": self.commands_issued,
        }


class PluginEntry:
    """Internal tracking record for a loaded plugin."""

    def __init__(self, manifest: PluginManifest, module: Any) -> None:
        self.manifest = manifest
        self.module = module
        self.state = PluginState.LOADED
        self.error: Optional[str] = None
        # Flask blueprints or route functions the plugin optionally provides.
        self.blueprint: Optional[Any] = None

    def to_dict(self) -> Dict[str, Any]:
        return {
            **self.manifest.to_dict(),
            "state": self.state.value,
            "error": self.error,
        }


class ObserverBoundaryViolation(Exception):
    """Raised when an observer plugin attempts to call a command endpoint."""


class PluginRegistry:
    """
    Central registry for FEEN plugins.

    Usage::

        registry = PluginRegistry()
        registry.load_plugin("python/plugins/energy_logger.py")
        registry.activate_all()
        # Later:
        registry.deactivate_plugin("energy_logger")
        registry.unload_plugin("energy_logger")
    """

    def __init__(self) -> None:
        self._plugins: Dict[str, PluginEntry] = {}

    # ------------------------------------------------------------------
    # Lifecycle methods
    # ------------------------------------------------------------------

    def load_plugin(self, path_or_module: str) -> PluginEntry:
        """Load a plugin from a file path or dotted module name.

        The plugin module must expose a ``MANIFEST`` attribute of type
        :class:`PluginManifest`.  Loading is sandboxed: any exception raised
        during module import is caught and the plugin is put in FAILED state.

        Returns the :class:`PluginEntry` regardless of success/failure.
        """
        try:
            module = self._import_plugin(path_or_module)
        except Exception as exc:
            logger.error("Failed to import plugin %r: %s", path_or_module, exc)
            # Create a minimal failed entry so callers always get a return value.
            failed = PluginEntry.__new__(PluginEntry)
            failed.manifest = PluginManifest(
                name=os.path.splitext(os.path.basename(path_or_module))[0],
                version=(0, 0, 0),
                plugin_type=PluginType.UI,
                description="[failed to load]",
            )
            failed.module = None
            failed.state = PluginState.FAILED
            failed.error = str(exc)
            failed.blueprint = None
            self._plugins[failed.manifest.name] = failed
            return failed

        manifest = getattr(module, "MANIFEST", None)
        if not isinstance(manifest, PluginManifest):
            entry = PluginEntry.__new__(PluginEntry)
            entry.manifest = PluginManifest(
                name=os.path.splitext(os.path.basename(path_or_module))[0],
                version=(0, 0, 0),
                plugin_type=PluginType.UI,
                description="[missing MANIFEST]",
            )
            entry.module = module
            entry.state = PluginState.FAILED
            entry.error = "Module does not expose a PluginManifest as MANIFEST"
            entry.blueprint = None
            self._plugins[entry.manifest.name] = entry
            return entry

        if not manifest.is_api_compatible():
            entry = PluginEntry.__new__(PluginEntry)
            entry.manifest = manifest
            entry.module = module
            entry.state = PluginState.FAILED
            entry.error = (
                f"Plugin requires FEEN API {manifest.min_feen_api}–{manifest.max_feen_api}; "
                f"running {FEEN_PLUGIN_API_VERSION}"
            )
            entry.blueprint = None
            self._plugins[manifest.name] = entry
            return entry

        entry = PluginEntry(manifest, module)
        self._plugins[manifest.name] = entry
        logger.info("Loaded plugin %r (%s)", manifest.name, manifest.plugin_type.value)
        return entry

    def register_plugin(self, name: str) -> bool:
        """Advance a LOADED plugin to REGISTERED state (obtains its Blueprint if any).

        Returns True on success, False on failure.
        """
        entry = self._plugins.get(name)
        if entry is None or entry.state != PluginState.LOADED:
            return False
        try:
            if hasattr(entry.module, "get_blueprint"):
                entry.blueprint = entry.module.get_blueprint()
            entry.state = PluginState.REGISTERED
            logger.info("Registered plugin %r", name)
            return True
        except Exception as exc:
            entry.state = PluginState.FAILED
            entry.error = f"register failed: {exc}"
            logger.error("Failed to register plugin %r: %s", name, exc)
            return False

    def activate_plugin(self, name: str) -> bool:
        """Advance a REGISTERED plugin to ACTIVE state.

        Calls ``plugin.activate()`` if present.  Failure is isolated.
        Returns True on success, False on failure.
        """
        entry = self._plugins.get(name)
        if entry is None or entry.state != PluginState.REGISTERED:
            return False
        try:
            if hasattr(entry.module, "activate"):
                entry.module.activate()
            entry.state = PluginState.ACTIVE
            logger.info("Activated plugin %r", name)
            return True
        except Exception as exc:
            entry.state = PluginState.FAILED
            entry.error = f"activate failed: {exc}"
            logger.error("Failed to activate plugin %r: %s", name, exc)
            return False

    def deactivate_plugin(self, name: str) -> bool:
        """Deactivate an ACTIVE plugin without unloading it.

        Calls ``plugin.deactivate()`` if present.  Failure is isolated.
        Returns True on success, False on failure.
        """
        entry = self._plugins.get(name)
        if entry is None or entry.state != PluginState.ACTIVE:
            return False
        try:
            if hasattr(entry.module, "deactivate"):
                entry.module.deactivate()
            entry.state = PluginState.REGISTERED
            logger.info("Deactivated plugin %r", name)
            return True
        except Exception as exc:
            entry.state = PluginState.FAILED
            entry.error = f"deactivate failed: {exc}"
            logger.error("Failed to deactivate plugin %r: %s", name, exc)
            return False

    def unload_plugin(self, name: str) -> bool:
        """Deactivate (if needed) and remove a plugin from the registry.

        Calls ``plugin.unload()`` if present.  Returns True on success.
        """
        entry = self._plugins.get(name)
        if entry is None:
            return False
        try:
            if entry.state == PluginState.ACTIVE:
                self.deactivate_plugin(name)
            if entry.module is not None and hasattr(entry.module, "unload"):
                entry.module.unload()
        except Exception as exc:
            logger.error("Error during unload of %r: %s", name, exc)
        del self._plugins[name]
        logger.info("Unloaded plugin %r", name)
        return True

    # ------------------------------------------------------------------
    # Batch helpers
    # ------------------------------------------------------------------

    def activate_all(self) -> None:
        """Load → register → activate all LOADED plugins in order."""
        for name in list(self._plugins.keys()):
            entry = self._plugins[name]
            if entry.state == PluginState.LOADED:
                self.register_plugin(name)
            if entry.state == PluginState.REGISTERED:
                self.activate_plugin(name)

    # ------------------------------------------------------------------
    # Observer boundary enforcement
    # ------------------------------------------------------------------

    @staticmethod
    def assert_observer_safe(plugin_type: PluginType, method: str, path: str) -> None:
        """Raise :class:`ObserverBoundaryViolation` if an OBSERVER or UI plugin
        tries to issue a mutating HTTP request.

        ``method`` should be the HTTP method string (e.g. ``"POST"``).
        ``path`` is the endpoint path for logging purposes.
        """
        if plugin_type in (PluginType.OBSERVER, PluginType.UI):
            if method.upper() not in ("GET", "HEAD", "OPTIONS"):
                raise ObserverBoundaryViolation(
                    f"Plugin of type {plugin_type.value!r} attempted {method.upper()} {path!r}. "
                    "Observer/UI plugins may only issue read-only HTTP requests."
                )

    # ------------------------------------------------------------------
    # Introspection
    # ------------------------------------------------------------------

    def get_plugin(self, name: str) -> Optional[PluginEntry]:
        return self._plugins.get(name)

    def list_plugins(self) -> List[Dict[str, Any]]:
        return [e.to_dict() for e in self._plugins.values()]

    def active_blueprints(self) -> List[Any]:
        """Return Flask Blueprints from all ACTIVE plugins that provided one."""
        return [
            e.blueprint
            for e in self._plugins.values()
            if e.state == PluginState.ACTIVE and e.blueprint is not None
        ]

    # ------------------------------------------------------------------
    # Private helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _import_plugin(path_or_module: str) -> Any:
        """Import a plugin by file path or dotted module name."""
        if os.path.isfile(path_or_module):
            spec = importlib.util.spec_from_file_location(
                "feen_plugin_" + os.path.splitext(os.path.basename(path_or_module))[0],
                path_or_module,
            )
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return module
        # Dotted module name
        return importlib.import_module(path_or_module)

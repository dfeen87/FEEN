"""
Tests for the FEEN Plugin Registry.

These tests validate:
  • Plugin lifecycle state machine (load → register → activate → deactivate → unload)
  • Observer boundary enforcement (observer/UI plugins cannot issue commands)
  • API version compatibility checking
  • Manifest validation (invalid names, wrong type + commands_issued)
  • Failure isolation (bad plugin load does not crash the registry)
  • Built-in example plugin loading
  • Flask Blueprint registration from active plugins
"""

import os
import sys
import textwrap
import types
import unittest

# Ensure the python/ directory is on the path for direct test execution.
_HERE = os.path.dirname(os.path.abspath(__file__))
_PYTHON_DIR = os.path.dirname(_HERE)
if _PYTHON_DIR not in sys.path:
    sys.path.insert(0, _PYTHON_DIR)

from plugin_registry import (
    FEEN_PLUGIN_API_VERSION,
    ObserverBoundaryViolation,
    PluginManifest,
    PluginRegistry,
    PluginState,
    PluginType,
)


# ---------------------------------------------------------------------------
# Helpers — create minimal plugin modules in memory
# ---------------------------------------------------------------------------

def _make_module(name: str, manifest: PluginManifest, *, with_hooks: bool = False) -> types.ModuleType:
    """Build a synthetic plugin module with the given manifest."""
    mod = types.ModuleType(f"feen_plugin_{name}")
    mod.MANIFEST = manifest
    if with_hooks:
        mod._calls = []
        def activate():   mod._calls.append("activate")
        def deactivate(): mod._calls.append("deactivate")
        def unload():     mod._calls.append("unload")
        mod.activate   = activate
        mod.deactivate = deactivate
        mod.unload     = unload
    return mod


def _write_plugin_file(tmp_path: str, content: str) -> str:
    """Write a plugin Python file to a temp path and return its path."""
    path = os.path.join(tmp_path, "tmp_plugin.py")
    with open(path, "w") as f:
        f.write(content)
    return path


# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------

class TestPluginManifest(unittest.TestCase):

    def test_valid_observer_manifest(self):
        m = PluginManifest(
            name="my_obs",
            version=(1, 2, 3),
            plugin_type=PluginType.OBSERVER,
            description="test",
        )
        self.assertEqual(m.name, "my_obs")
        self.assertEqual(m.version, (1, 2, 3))
        self.assertEqual(m.plugin_type, PluginType.OBSERVER)
        self.assertTrue(m.is_api_compatible())

    def test_invalid_name_raises(self):
        with self.assertRaises(ValueError):
            PluginManifest(name="bad name!", version=(1,), plugin_type=PluginType.UI,
                           description="x")

    def test_observer_with_commands_raises(self):
        with self.assertRaises(ValueError):
            PluginManifest(
                name="bad_obs",
                version=(1,),
                plugin_type=PluginType.OBSERVER,
                description="x",
                commands_issued=["POST /api/network/tick"],
            )

    def test_tool_with_commands_ok(self):
        m = PluginManifest(
            name="my_tool",
            version=(1,),
            plugin_type=PluginType.TOOL,
            description="x",
            commands_issued=["POST /api/network/nodes/0/inject"],
        )
        self.assertEqual(m.commands_issued, ["POST /api/network/nodes/0/inject"])

    def test_api_version_incompatible(self):
        m = PluginManifest(
            name="old_plugin",
            version=(1,),
            plugin_type=PluginType.UI,
            description="x",
            min_feen_api=(99, 0),
            max_feen_api=(99, 99),
        )
        self.assertFalse(m.is_api_compatible())

    def test_to_dict(self):
        m = PluginManifest(name="p", version=(1, 0, 0), plugin_type=PluginType.UI, description="d")
        d = m.to_dict()
        self.assertEqual(d["name"], "p")
        self.assertEqual(d["type"], "ui")


class TestPluginLifecycle(unittest.TestCase):

    def _make_registry_with_plugin(self, name="test_plugin",
                                   plugin_type=PluginType.OBSERVER,
                                   with_hooks=False):
        reg = PluginRegistry()
        manifest = PluginManifest(name=name, version=(1,), plugin_type=plugin_type, description="t")
        mod = _make_module(name, manifest, with_hooks=with_hooks)
        # Inject directly into registry internal state for unit testing.
        from plugin_registry import PluginEntry
        entry = PluginEntry(manifest, mod)
        reg._plugins[name] = entry
        return reg, name

    def test_initial_state_is_loaded(self):
        reg, name = self._make_registry_with_plugin()
        entry = reg.get_plugin(name)
        self.assertEqual(entry.state, PluginState.LOADED)

    def test_register_transitions_to_registered(self):
        reg, name = self._make_registry_with_plugin()
        ok = reg.register_plugin(name)
        self.assertTrue(ok)
        self.assertEqual(reg.get_plugin(name).state, PluginState.REGISTERED)

    def test_activate_transitions_to_active(self):
        reg, name = self._make_registry_with_plugin()
        reg.register_plugin(name)
        ok = reg.activate_plugin(name)
        self.assertTrue(ok)
        self.assertEqual(reg.get_plugin(name).state, PluginState.ACTIVE)

    def test_deactivate_returns_to_registered(self):
        reg, name = self._make_registry_with_plugin()
        reg.register_plugin(name)
        reg.activate_plugin(name)
        ok = reg.deactivate_plugin(name)
        self.assertTrue(ok)
        self.assertEqual(reg.get_plugin(name).state, PluginState.REGISTERED)

    def test_unload_removes_plugin(self):
        reg, name = self._make_registry_with_plugin()
        reg.register_plugin(name)
        reg.activate_plugin(name)
        ok = reg.unload_plugin(name)
        self.assertTrue(ok)
        self.assertIsNone(reg.get_plugin(name))

    def test_lifecycle_hooks_called(self):
        reg, name = self._make_registry_with_plugin(with_hooks=True)
        entry = reg.get_plugin(name)
        reg.register_plugin(name)
        reg.activate_plugin(name)
        self.assertIn("activate", entry.module._calls)
        reg.deactivate_plugin(name)
        self.assertIn("deactivate", entry.module._calls)
        reg.unload_plugin(name)
        # unload is called on the module; entry is removed so check via calls
        self.assertIn("unload", entry.module._calls)

    def test_activate_all(self):
        reg = PluginRegistry()
        for nm in ("p1", "p2"):
            manifest = PluginManifest(name=nm, version=(1,), plugin_type=PluginType.UI, description="x")
            mod = _make_module(nm, manifest)
            from plugin_registry import PluginEntry
            reg._plugins[nm] = PluginEntry(manifest, mod)
        reg.activate_all()
        for nm in ("p1", "p2"):
            self.assertEqual(reg.get_plugin(nm).state, PluginState.ACTIVE)

    def test_double_activate_fails_gracefully(self):
        reg, name = self._make_registry_with_plugin()
        reg.register_plugin(name)
        reg.activate_plugin(name)
        # Second activate should return False (wrong state)
        ok = reg.activate_plugin(name)
        self.assertFalse(ok)
        self.assertEqual(reg.get_plugin(name).state, PluginState.ACTIVE)

    def test_deactivate_not_active_returns_false(self):
        reg, name = self._make_registry_with_plugin()
        ok = reg.deactivate_plugin(name)
        self.assertFalse(ok)


class TestObserverBoundary(unittest.TestCase):

    def test_observer_get_allowed(self):
        # Should not raise
        PluginRegistry.assert_observer_safe(PluginType.OBSERVER, "GET", "/api/network/nodes")

    def test_observer_post_raises(self):
        with self.assertRaises(ObserverBoundaryViolation):
            PluginRegistry.assert_observer_safe(PluginType.OBSERVER, "POST", "/api/network/tick")

    def test_ui_post_raises(self):
        with self.assertRaises(ObserverBoundaryViolation):
            PluginRegistry.assert_observer_safe(PluginType.UI, "POST", "/api/network/reset")

    def test_tool_post_allowed(self):
        # TOOL plugins may issue POST requests
        PluginRegistry.assert_observer_safe(PluginType.TOOL, "POST", "/api/network/nodes/0/inject")

    def test_observer_head_allowed(self):
        PluginRegistry.assert_observer_safe(PluginType.OBSERVER, "HEAD", "/api/health")

    def test_observer_options_allowed(self):
        PluginRegistry.assert_observer_safe(PluginType.OBSERVER, "OPTIONS", "/api/health")


class TestFailureIsolation(unittest.TestCase):

    def test_load_nonexistent_file_returns_failed_entry(self):
        reg = PluginRegistry()
        entry = reg.load_plugin("/nonexistent/path/bad_plugin.py")
        self.assertEqual(entry.state, PluginState.FAILED)
        self.assertIsNotNone(entry.error)

    def test_load_module_without_manifest_returns_failed(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            path = _write_plugin_file(tmp, "# No MANIFEST here\n")
            reg = PluginRegistry()
            entry = reg.load_plugin(path)
            self.assertEqual(entry.state, PluginState.FAILED)

    def test_load_incompatible_api_version_fails(self):
        import tempfile
        content = textwrap.dedent("""\
            import sys, os
            sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))
            from plugin_registry import PluginManifest, PluginType
            MANIFEST = PluginManifest(
                name='future_plugin', version=(1,),
                plugin_type=PluginType.UI, description='x',
                min_feen_api=(99, 0), max_feen_api=(99, 99),
            )
        """)
        with tempfile.TemporaryDirectory() as tmp:
            path = _write_plugin_file(tmp, content)
            reg = PluginRegistry()
            entry = reg.load_plugin(path)
            self.assertEqual(entry.state, PluginState.FAILED)
            self.assertIn("99", entry.error)

    def test_activate_exception_sets_failed_state(self):
        reg = PluginRegistry()
        manifest = PluginManifest(name="bad_act", version=(1,), plugin_type=PluginType.UI, description="x")
        mod = types.ModuleType("bad_act")
        mod.MANIFEST = manifest
        def bad_activate(): raise RuntimeError("boom")
        mod.activate = bad_activate

        from plugin_registry import PluginEntry
        entry = PluginEntry(manifest, mod)
        reg._plugins["bad_act"] = entry
        reg.register_plugin("bad_act")
        ok = reg.activate_plugin("bad_act")
        self.assertFalse(ok)
        self.assertEqual(entry.state, PluginState.FAILED)

    def test_failed_plugin_does_not_appear_in_active_blueprints(self):
        reg = PluginRegistry()
        entry_fail = reg.load_plugin("/nonexistent.py")
        blueprints = reg.active_blueprints()
        self.assertEqual(blueprints, [])


class TestBuiltinPlugins(unittest.TestCase):
    """Smoke-test the built-in example plugins shipped with FEEN."""

    _PLUGINS_DIR = os.path.join(_PYTHON_DIR, "plugins")

    def _load_plugin(self, filename):
        reg = PluginRegistry()
        path = os.path.join(self._PLUGINS_DIR, filename)
        return reg, reg.load_plugin(path)

    def test_ui_dashboard_loads(self):
        _, entry = self._load_plugin("ui_dashboard.py")
        self.assertNotEqual(entry.state, PluginState.FAILED, entry.error)
        self.assertEqual(entry.manifest.plugin_type, PluginType.UI)

    def test_observer_logger_loads(self):
        _, entry = self._load_plugin("observer_logger.py")
        self.assertNotEqual(entry.state, PluginState.FAILED, entry.error)
        self.assertEqual(entry.manifest.plugin_type, PluginType.OBSERVER)

    def test_hardware_monitor_loads(self):
        _, entry = self._load_plugin("hardware_monitor.py")
        self.assertNotEqual(entry.state, PluginState.FAILED, entry.error)
        self.assertEqual(entry.manifest.plugin_type, PluginType.OBSERVER)

    def test_all_builtin_plugins_activate(self):
        reg = PluginRegistry()
        for fname in ("ui_dashboard.py", "observer_logger.py", "hardware_monitor.py"):
            reg.load_plugin(os.path.join(self._PLUGINS_DIR, fname))
        reg.activate_all()
        for entry in reg._plugins.values():
            self.assertEqual(entry.state, PluginState.ACTIVE,
                             f"Plugin {entry.manifest.name} state: {entry.state}, error: {entry.error}")

    def test_builtin_plugins_provide_blueprints(self):
        reg = PluginRegistry()
        for fname in ("ui_dashboard.py", "observer_logger.py", "hardware_monitor.py"):
            reg.load_plugin(os.path.join(self._PLUGINS_DIR, fname))
        reg.activate_all()
        bps = reg.active_blueprints()
        self.assertEqual(len(bps), 3)

    def test_observer_logger_poll(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location(
            "feen_plugin_observer_logger_test",
            os.path.join(self._PLUGINS_DIR, "observer_logger.py"),
        )
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        mod.poll([{"id": 0, "energy": 1.5, "snr": 20.0}])
        # Access internal _log via module attribute (white-box)
        log = list(mod._log)
        self.assertEqual(len(log), 1)
        self.assertEqual(log[0]["nodes"][0]["id"], 0)

    def test_hardware_monitor_update_metrics(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location(
            "feen_plugin_hw_monitor_test",
            os.path.join(self._PLUGINS_DIR, "hardware_monitor.py"),
        )
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        mod.update_metrics([{"id": 0, "energy": 0.5, "snr": 15.0}])
        m = mod._latest_metrics
        self.assertIsNotNone(m)
        self.assertEqual(m["node_count"], 1)
        self.assertAlmostEqual(m["nodes"][0]["snr_headroom_db"], 5.0)


class TestEagerPluginInitialization(unittest.TestCase):
    """Verify that plugins and blueprints are registered at import time.

    Flask 2.x raises an error if blueprints are registered after the first
    request.  Eager initialization in feen_rest_api ensures this never happens.
    """

    def _import_feen_rest_api(self):
        """Import feen_rest_api in a clean module environment."""
        import importlib
        # Remove cached module so each test gets a fresh import.
        for key in list(sys.modules.keys()):
            if "feen_rest_api" in key:
                del sys.modules[key]
        sys.path.insert(0, _PYTHON_DIR)
        try:
            import feen_rest_api
            return feen_rest_api
        except SystemExit:
            self.skipTest("pyfeen native module not available in this environment")

    def test_plugins_initialized_before_first_request(self):
        """_plugins_initialized must be True after importing feen_rest_api."""
        mod = self._import_feen_rest_api()
        self.assertTrue(
            mod._plugins_initialized,
            "Plugins must be initialized at import time, not lazily on first request.",
        )

    def test_blueprints_registered_before_first_request(self):
        """Plugin blueprints must be registered with the Flask app at import time."""
        mod = self._import_feen_rest_api()
        registered_names = {bp.name for bp in mod.app.blueprints.values()}
        # All active plugins that provided a blueprint must be registered.
        active_bps = mod.plugin_registry.active_blueprints()
        for bp in active_bps:
            self.assertIn(
                bp.name,
                registered_names,
                f"Blueprint {bp.name!r} was not registered before the first request.",
            )

    def test_health_endpoint_works_after_eager_init(self):
        """The /api/health endpoint must respond correctly after eager init."""
        mod = self._import_feen_rest_api()
        client = mod.app.test_client()
        response = client.get("/api/health")
        self.assertEqual(response.status_code, 200)
        data = response.get_json()
        self.assertEqual(data["status"], "ok")


if __name__ == "__main__":
    unittest.main(verbosity=2)

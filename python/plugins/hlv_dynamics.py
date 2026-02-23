"""
HLV Dynamics Lab Plugin — FEEN Physics + Observer Plugin
=========================================================

Type: TOOL  (manages its own simulation state; exposes POST endpoints)
Specification: docs/HLV.md — Marcel Krüger & Don Feeney

Implements the full Phase-1 through Phase-4 HLV test plan:

  Physics plugins (ẋ = F(t, x; G, θ)):
    P1 — Kuramoto baseline
    P2 — Exponential memory kernel (non-Markovian extension)
    P3 — Phase-offset (chiral) coupling

  Observer modules (y(t) = O(t, x(t); G, θ)):
    O1 — Synchronization order parameter R(t), ψ(t), σ_θ(t)
    O2 — Deterministic instability functional ΔΦ(t)

  Artifacts (per-run bundle):
    config.json, metrics.csv, events.jsonl, hash.txt

Architecture:
  • All simulation state is fully contained in this plugin.
  • The FEEN C++ integrator is not used; Euler/RK4 runs in pure Python/NumPy.
  • Observer hooks are called after each integration step.
  • The plugin exposes a Flask Blueprint at /api/hlv/*.
  • Thread safety: a single lock guards the active simulation state.
"""

from __future__ import annotations

import csv
import hashlib
import io
import json
import logging
import math
import threading
import time as _time
from typing import Any, Dict, List, Optional, Tuple

import numpy as np
from flask import Blueprint, jsonify, request

from plugin_registry import FEEN_PLUGIN_API_VERSION, PluginManifest, PluginType

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Plugin manifest
# ---------------------------------------------------------------------------

MANIFEST = PluginManifest(
    name="hlv_dynamics",
    version=(1, 0, 0),
    plugin_type=PluginType.TOOL,
    description=(
        "HLV Dynamics Lab: Kuramoto phase oscillator physics plugins (P1/P2/P3) "
        "and observer modules (O1/O2) for structured phase-memory dynamics experiments. "
        "Implements the full HLV.md test plan including κ-sweeps, artifact bundles, "
        "and null-test verification."
    ),
    min_feen_api=(1, 0),
    max_feen_api=(1, 99),
    commands_issued=[
        "POST /api/hlv/run",
        "POST /api/hlv/sweep",
        "POST /api/hlv/inject",
    ],
)

# ---------------------------------------------------------------------------
# Graph layer — topology builders (Aᵢⱼ, φᵢⱼ)
# ---------------------------------------------------------------------------

def build_ring(N: int, weight: float = 1.0) -> np.ndarray:
    """Undirected ring graph: each node connects to its two nearest neighbours."""
    A = np.zeros((N, N))
    for i in range(N):
        A[i, (i + 1) % N] = weight
        A[i, (i - 1) % N] = weight
    return A


def build_small_world(N: int, k: int = 4, beta: float = 0.1,
                      rng: Optional[np.random.Generator] = None) -> np.ndarray:
    """Watts-Strogatz small-world graph (unweighted)."""
    if rng is None:
        rng = np.random.default_rng(0)
    A = np.zeros((N, N))
    # Start with a ring lattice of degree k
    for i in range(N):
        for d in range(1, k // 2 + 1):
            j = (i + d) % N
            A[i, j] = 1.0
            A[j, i] = 1.0
    # Rewire with probability beta
    for i in range(N):
        for d in range(1, k // 2 + 1):
            if rng.random() < beta:
                j_old = (i + d) % N
                candidates = [m for m in range(N) if m != i and A[i, m] == 0]
                if candidates:
                    j_new = rng.choice(candidates)
                    A[i, j_old] = 0.0
                    A[j_old, i] = 0.0
                    A[i, j_new] = 1.0
                    A[j_new, i] = 1.0
    return A


def build_erdos_renyi(N: int, p: float = 0.2,
                      rng: Optional[np.random.Generator] = None) -> np.ndarray:
    """Erdős–Rényi random graph G(N, p)."""
    if rng is None:
        rng = np.random.default_rng(0)
    A = np.zeros((N, N))
    for i in range(N):
        for j in range(i + 1, N):
            if rng.random() < p:
                A[i, j] = 1.0
                A[j, i] = 1.0
    return A


def build_phase_offsets_ring(N: int, phi0: float,
                              mode: str = "chiral") -> np.ndarray:
    """Build phase-offset matrix φᵢⱼ for a ring.

    mode='chiral'  : forward edges +phi0, backward edges -phi0
    mode='random'  : uniform in [-phi0, phi0] (requires phi0 as half-range)
    mode='zero'    : all zeros (baseline)
    """
    phi = np.zeros((N, N))
    if mode == "chiral":
        for i in range(N):
            phi[i, (i + 1) % N] = +phi0
            phi[i, (i - 1) % N] = -phi0
    elif mode == "random":
        rng = np.random.default_rng(0)
        for i in range(N):
            for j in range(N):
                if i != j:
                    phi[i, j] = rng.uniform(-phi0, phi0)
    return phi


def build_graph(cfg: Dict[str, Any]) -> Tuple[np.ndarray, np.ndarray]:
    """Build (A, phi) matrices from a topology config dict."""
    N = int(cfg.get("N", 32))
    topo = cfg.get("topology", "ring")
    topo_seed = cfg.get("topo_seed", 0)
    rng = np.random.default_rng(topo_seed)

    if topo == "ring":
        A = build_ring(N)
    elif topo == "fully_connected":
        # All-to-all coupling (mean-field Kuramoto model)
        A = (np.ones((N, N)) - np.eye(N)) / (N - 1)
    elif topo == "small_world":
        k = int(cfg.get("sw_k", 4))
        beta = float(cfg.get("sw_beta", 0.1))
        A = build_small_world(N, k=k, beta=beta, rng=rng)
    elif topo == "erdos_renyi":
        p = float(cfg.get("er_p", 0.2))
        A = build_erdos_renyi(N, p=p, rng=rng)
    else:
        A = build_ring(N)

    # Phase offsets (P3)
    phi0 = float(cfg.get("phi0", 0.0))
    offset_mode = cfg.get("offset_mode", "chiral")
    phi = build_phase_offsets_ring(N, phi0, mode=offset_mode)

    return A, phi


# ---------------------------------------------------------------------------
# Frequency distribution samplers
# ---------------------------------------------------------------------------

def sample_frequencies(N: int, dist_cfg: Dict[str, Any],
                        rng: np.random.Generator) -> np.ndarray:
    """Sample natural frequencies ωᵢ from the specified distribution."""
    dist = dist_cfg.get("type", "lorentzian")
    if dist == "lorentzian":
        gamma = float(dist_cfg.get("gamma", 0.5))
        # Cauchy distribution: scale = gamma
        return rng.standard_cauchy(N) * gamma
    elif dist == "gaussian":
        mean = float(dist_cfg.get("mean", 0.0))
        std = float(dist_cfg.get("std", 0.5))
        return rng.normal(mean, std, N)
    elif dist == "uniform":
        low = float(dist_cfg.get("low", -1.0))
        high = float(dist_cfg.get("high", 1.0))
        return rng.uniform(low, high, N)
    elif dist == "constant":
        return np.zeros(N)
    else:
        return rng.standard_cauchy(N) * 0.5


# ---------------------------------------------------------------------------
# Physics plugins — ẋ = F(t, x; G, θ)
# ---------------------------------------------------------------------------

def physics_p1(theta: np.ndarray, omega: np.ndarray,
               A: np.ndarray, phi: np.ndarray,
               params: Dict[str, Any],
               rng: np.random.Generator) -> np.ndarray:
    """P1: Kuramoto baseline.

    θ̇ᵢ = ωᵢ + κ Σⱼ Aᵢⱼ sin(θⱼ − θᵢ) + σ ξᵢ(t)
    """
    kappa = float(params.get("kappa", 1.0))
    sigma = float(params.get("sigma", 0.0))
    # Phase difference matrix: diff[i,j] = θⱼ − θᵢ
    diff = theta[np.newaxis, :] - theta[:, np.newaxis]
    coupling = np.sum(A * np.sin(diff), axis=1)
    dtheta = omega + kappa * coupling
    if sigma > 0.0:
        dtheta += sigma * rng.standard_normal(len(theta))
    return dtheta


def physics_p2(theta: np.ndarray, omega: np.ndarray, memory: np.ndarray,
               A: np.ndarray, phi: np.ndarray,
               params: Dict[str, Any],
               rng: np.random.Generator) -> Tuple[np.ndarray, np.ndarray]:
    """P2: Exponential memory kernel.

    θ̇ᵢ = ωᵢ + κ Σⱼ Aᵢⱼ sin(θⱼ − θᵢ) + η mᵢ + σ ξᵢ(t)
    ṁᵢ = −mᵢ/τₘ + Σⱼ Aᵢⱼ sin(θⱼ − θᵢ)
    """
    kappa = float(params.get("kappa", 1.0))
    eta = float(params.get("eta", 0.5))
    tau_m = float(params.get("tau_m", 5.0))
    sigma = float(params.get("sigma", 0.0))

    diff = theta[np.newaxis, :] - theta[:, np.newaxis]
    sin_diff_sum = np.sum(A * np.sin(diff), axis=1)

    dtheta = omega + kappa * sin_diff_sum + eta * memory
    if sigma > 0.0:
        dtheta += sigma * rng.standard_normal(len(theta))

    dmemory = -memory / tau_m + sin_diff_sum

    return dtheta, dmemory


def physics_p3(theta: np.ndarray, omega: np.ndarray,
               A: np.ndarray, phi: np.ndarray,
               params: Dict[str, Any],
               rng: np.random.Generator) -> np.ndarray:
    """P3: Phase-offset (chiral) coupling.

    θ̇ᵢ = ωᵢ + κ Σⱼ Aᵢⱼ sin(θⱼ − θᵢ + φᵢⱼ) + σ ξᵢ(t)
    """
    kappa = float(params.get("kappa", 1.0))
    sigma = float(params.get("sigma", 0.0))

    diff = theta[np.newaxis, :] - theta[:, np.newaxis]  # diff[i,j] = θⱼ − θᵢ
    coupling = np.sum(A * np.sin(diff + phi), axis=1)
    dtheta = omega + kappa * coupling
    if sigma > 0.0:
        dtheta += sigma * rng.standard_normal(len(theta))
    return dtheta


# ---------------------------------------------------------------------------
# Observer modules — y(t) = O(t, x(t); G, θ)
# ---------------------------------------------------------------------------

def observer_o1(theta: np.ndarray) -> Dict[str, float]:
    """O1: Synchronisation order parameter.

    R(t) e^{iψ(t)} = (1/N) Σⱼ e^{iθⱼ}
    """
    z = np.mean(np.exp(1j * theta))
    R = float(abs(z))
    psi = float(np.angle(z))
    # Circular standard deviation
    sigma_theta = float(np.sqrt(max(-2.0 * math.log(max(R, 1e-12)), 0.0)))
    return {"R": R, "psi": psi, "sigma_theta": sigma_theta}


def observer_o2(history_R: List[float], window: int = 10) -> float:
    """O2: ΔΦ(t) — deterministic instability functional.

    Computes the rate of change of the order-parameter R over a baseline window.
    ΔΦ(t) = |R(t) − ⟨R⟩_baseline| where baseline is the mean over the
    previous `window` samples, normalised by the baseline mean.
    """
    n = len(history_R)
    if n < window + 1:
        return 0.0
    baseline = float(np.mean(history_R[n - window - 1: n - 1]))
    if baseline < 1e-9:
        return 0.0
    delta = abs(history_R[-1] - baseline) / baseline
    return float(delta)


# ---------------------------------------------------------------------------
# Integrator — Euler (deterministic, fast) and RK4 (accuracy)
# ---------------------------------------------------------------------------

def _rk4_step_p1(theta, omega, A, phi, params, dt):
    """RK4 integration step for P1 (noiseless)."""
    _rng_dummy = np.random.default_rng(0)  # no noise in RK4 steps

    def f(th):
        return physics_p1(th, omega, A, phi, {**params, "sigma": 0.0}, _rng_dummy)

    k1 = f(theta)
    k2 = f(theta + 0.5 * dt * k1)
    k3 = f(theta + 0.5 * dt * k2)
    k4 = f(theta + dt * k3)
    return theta + (dt / 6.0) * (k1 + 2 * k2 + 2 * k3 + k4)


# ---------------------------------------------------------------------------
# Core simulation runner
# ---------------------------------------------------------------------------

def run_simulation(cfg: Dict[str, Any],
                   inject_events: Optional[List[Dict]] = None
                   ) -> Dict[str, Any]:
    """Run a single Kuramoto simulation and return metrics + artifacts.

    Parameters
    ----------
    cfg : dict
        Full configuration (plugin, topology, parameters, sim settings).
        May include 'inject_events' key.
    inject_events : list of dicts, optional
        Pre-scheduled perturbation events (overrides cfg['inject_events']):
        {'time': float, 'type': 'phase_kick'|'omega_kick', 'node': int|'all',
         'amplitude': float, 'duration': float (omega_kick only)}

    Returns
    -------
    dict with keys:
        'config', 'metrics', 'events', 'summary'
    """
    # inject_events from parameter takes priority; fall back to cfg key
    if inject_events is None:
        inject_events = cfg.get("inject_events") or []
    # ── Configuration ──────────────────────────────────────────────────────
    plugin = cfg.get("plugin", "P1")
    N = int(cfg.get("N", 32))
    dt = float(cfg.get("dt", 0.05))
    t_end = float(cfg.get("t_end", 50.0))
    integrator = cfg.get("integrator", "euler")
    seed = int(cfg.get("seed", 42))
    freq_dist = cfg.get("freq_dist", {"type": "lorentzian", "gamma": 0.5})
    observers_enabled = cfg.get("observers", ["O1"])

    params = {
        "kappa": float(cfg.get("kappa", 1.0)),
        "sigma": float(cfg.get("sigma", 0.0)),
        "eta": float(cfg.get("eta", 0.5)),
        "tau_m": float(cfg.get("tau_m", 5.0)),
    }

    # ── Initialise RNG and state ────────────────────────────────────────────
    rng = np.random.default_rng(seed)
    omega = sample_frequencies(N, freq_dist, rng)
    theta = rng.uniform(-math.pi, math.pi, N)  # random initial phases
    memory = np.zeros(N)  # P2 memory variable

    # ── Graph layer ─────────────────────────────────────────────────────────
    A, phi_offsets = build_graph(cfg)

    # ── Omega-kick state (for omega_kick perturbations) ─────────────────────
    omega_kick_ends: Dict[int, float] = {}  # node → end_time
    omega_original = omega.copy()

    # ── Logging setup ────────────────────────────────────────────────────────
    metrics_rows: List[Dict[str, float]] = []
    events_log: List[Dict[str, Any]] = []
    history_R: List[float] = []

    # ── Pre-scheduled events queue ────────────────────────────────────────────
    pending_events = sorted((inject_events or []), key=lambda e: e.get("time", 0))
    event_idx = 0

    # ── Integration loop ──────────────────────────────────────────────────────
    t = 0.0
    n_steps = int(round(t_end / dt))

    for step in range(n_steps):
        t = step * dt

        # ── Apply scheduled perturbations at this timestep ────────────────
        while event_idx < len(pending_events):
            ev = pending_events[event_idx]
            ev_t = float(ev.get("time", 0.0))
            if ev_t > t + dt * 0.5:
                break
            ev_type = ev.get("type", "phase_kick")
            amp = float(ev.get("amplitude", 0.1))
            node_spec = ev.get("node", "all")

            nodes_to_kick = list(range(N)) if node_spec == "all" else [int(node_spec)]

            if ev_type == "phase_kick":
                for ni in nodes_to_kick:
                    theta[ni] += amp
                events_log.append({
                    "t": t, "type": "phase_kick",
                    "nodes": nodes_to_kick, "amplitude": amp
                })
            elif ev_type == "omega_kick":
                dur = float(ev.get("duration", 1.0))
                for ni in nodes_to_kick:
                    omega[ni] = omega_original[ni] + amp
                    omega_kick_ends[ni] = t + dur
                events_log.append({
                    "t": t, "type": "omega_kick",
                    "nodes": nodes_to_kick, "amplitude": amp, "duration": dur
                })

            event_idx += 1

        # ── Expire omega kicks ────────────────────────────────────────────
        for ni in list(omega_kick_ends.keys()):
            if omega_kick_ends[ni] <= t:
                omega[ni] = omega_original[ni]
                del omega_kick_ends[ni]

        # ── Physics step ──────────────────────────────────────────────────
        if plugin == "P1":
            if integrator == "rk4" and params["sigma"] == 0.0:
                theta = _rk4_step_p1(theta, omega, A, phi_offsets, params, dt)
            else:
                dtheta = physics_p1(theta, omega, A, phi_offsets, params, rng)
                theta = theta + dt * dtheta
        elif plugin == "P2":
            dtheta, dmemory = physics_p2(theta, omega, memory, A, phi_offsets, params, rng)
            theta = theta + dt * dtheta
            memory = memory + dt * dmemory
        elif plugin == "P3":
            dtheta = physics_p3(theta, omega, A, phi_offsets, params, rng)
            theta = theta + dt * dtheta
        else:
            # Default to P1
            dtheta = physics_p1(theta, omega, A, phi_offsets, params, rng)
            theta = theta + dt * dtheta

        # ── Wrap phases to [−π, π] ────────────────────────────────────────
        theta = (theta + math.pi) % (2 * math.pi) - math.pi

        # ── Observers ─────────────────────────────────────────────────────
        row: Dict[str, float] = {"t": round(t + dt, 8)}

        if "O1" in observers_enabled:
            o1 = observer_o1(theta)
            row.update(o1)
            history_R.append(o1["R"])
        else:
            row["R"] = 0.0
            row["psi"] = 0.0
            row["sigma_theta"] = 0.0
            history_R.append(0.0)

        if "O2" in observers_enabled:
            row["delta_phi"] = observer_o2(history_R)
        else:
            row["delta_phi"] = 0.0

        metrics_rows.append(row)

    # ── Summary statistics (final 20% window) ─────────────────────────────
    final_window_start = int(0.8 * len(metrics_rows))
    final_R = [r["R"] for r in metrics_rows[final_window_start:]]
    mean_R = float(np.mean(final_R)) if final_R else 0.0
    se_R = float(np.std(final_R) / math.sqrt(max(len(final_R), 1)))

    # Settling time: first time R > 0.9 (or None if never reached)
    settling_time = None
    for row in metrics_rows:
        if row["R"] > 0.9:
            settling_time = row["t"]
            break

    summary = {
        "mean_R_final": mean_R,
        "se_R_final": se_R,
        "settling_time": settling_time,
        "n_steps": n_steps,
        "t_end_actual": round(n_steps * dt, 8),
        "N": N,
        "plugin": plugin,
        "kappa": params["kappa"],
        "seed": seed,
    }

    return {
        "config": cfg,
        "metrics": metrics_rows,
        "events": events_log,
        "summary": summary,
    }


# ---------------------------------------------------------------------------
# Sweep protocol — κ sweep (E1 / Phase 1)
# ---------------------------------------------------------------------------

def run_kappa_sweep(sweep_cfg: Dict[str, Any]) -> Dict[str, Any]:
    """Execute the E1 κ-sweep protocol (HLV.md §F.E1).

    For each κ in the sweep range, runs `num_seeds` independent simulations
    and reports mean ± SE of the final-window order parameter R.
    """
    kappa_min = float(sweep_cfg.get("kappa_min", 0.0))
    kappa_max = float(sweep_cfg.get("kappa_max", 6.0))
    kappa_step = float(sweep_cfg.get("kappa_step", 0.2))
    num_seeds = int(sweep_cfg.get("num_seeds", 10))
    seed_base = int(sweep_cfg.get("seed_base", 0))

    kappa_values = []
    k = kappa_min
    while k <= kappa_max + kappa_step * 0.5:
        kappa_values.append(round(k, 8))
        k += kappa_step

    sweep_results = []
    for kappa in kappa_values:
        seed_results = []
        for s in range(num_seeds):
            sim_cfg = {**sweep_cfg, "kappa": kappa, "seed": seed_base + s}
            result = run_simulation(sim_cfg)
            seed_results.append(result["summary"])

        mean_R = float(np.mean([r["mean_R_final"] for r in seed_results]))
        se_R = float(np.std([r["mean_R_final"] for r in seed_results])
                     / math.sqrt(num_seeds))
        settling_times = [r["settling_time"] for r in seed_results
                          if r["settling_time"] is not None]
        mean_settle = float(np.mean(settling_times)) if settling_times else None

        sweep_results.append({
            "kappa": kappa,
            "mean_R": mean_R,
            "se_R": se_R,
            "mean_settling_time": mean_settle,
            "n_locked": len(settling_times),
            "num_seeds": num_seeds,
        })

    return {
        "sweep_config": sweep_cfg,
        "results": sweep_results,
        "kappa_values": kappa_values,
    }


# ---------------------------------------------------------------------------
# Artifact bundle — config.json + metrics.csv + events.jsonl + hash.txt
# ---------------------------------------------------------------------------

def build_artifact_bundle(result: Dict[str, Any]) -> Dict[str, str]:
    """Build the HLV artifact bundle as a dict of {filename: content_string}.

    Bundle specification (HLV.md Appendix A.2):
        config.json   — full run configuration
        metrics.csv   — t, R, ψ, σ_θ, ΔΦ time series
        events.jsonl  — perturbation event log
        hash.txt      — SHA-256 over config + metrics + events
    """
    config_str = json.dumps(result["config"], indent=2, sort_keys=True)

    # metrics.csv
    metrics = result.get("metrics", [])
    csv_buf = io.StringIO()
    if metrics:
        fieldnames = ["t", "R", "psi", "sigma_theta", "delta_phi"]
        writer = csv.DictWriter(csv_buf, fieldnames=fieldnames,
                                extrasaction="ignore")
        writer.writeheader()
        for row in metrics:
            writer.writerow({
                "t": row.get("t", 0.0),
                "R": row.get("R", 0.0),
                "psi": row.get("psi", 0.0),
                "sigma_theta": row.get("sigma_theta", 0.0),
                "delta_phi": row.get("delta_phi", 0.0),
            })
    metrics_str = csv_buf.getvalue()

    # events.jsonl
    events_str = "\n".join(json.dumps(ev) for ev in result.get("events", []))

    # hash.txt — SHA-256 over config + metrics + events
    hasher = hashlib.sha256()
    hasher.update(config_str.encode())
    hasher.update(metrics_str.encode())
    hasher.update(events_str.encode())
    hash_str = hasher.hexdigest()

    return {
        "config.json": config_str,
        "metrics.csv": metrics_str,
        "events.jsonl": events_str,
        "hash.txt": hash_str,
    }


# ---------------------------------------------------------------------------
# Plugin-level state (thread-safe)
# ---------------------------------------------------------------------------

_lock = threading.Lock()
_latest_result: Optional[Dict[str, Any]] = None
_latest_sweep: Optional[Dict[str, Any]] = None
_run_status: Dict[str, Any] = {"state": "idle", "last_run_at": None}


# ---------------------------------------------------------------------------
# Flask Blueprint
# ---------------------------------------------------------------------------

_blueprint = Blueprint("hlv_dynamics", __name__, url_prefix="/api/hlv")


@_blueprint.route("/status", methods=["GET"])
def status():
    """Return current HLV plugin status — read-only observer."""
    with _lock:
        return jsonify({
            "plugin": MANIFEST.name,
            "version": list(MANIFEST.version),
            "feen_api": list(FEEN_PLUGIN_API_VERSION),
            "run_status": _run_status,
            "has_result": _latest_result is not None,
            "has_sweep": _latest_sweep is not None,
        })


@_blueprint.route("/run", methods=["POST"])
def run_endpoint():
    """Run a single HLV simulation.

    Request body (JSON):
        plugin       : "P1" | "P2" | "P3"          (default "P1")
        N            : int                            (default 32)
        topology     : "ring" | "small_world" | "erdos_renyi"
        kappa        : float                          (default 1.0)
        sigma        : float   noise amplitude        (default 0.0)
        eta          : float   memory coupling (P2)  (default 0.5)
        tau_m        : float   memory timescale (P2) (default 5.0)
        phi0         : float   phase offset (P3)     (default 0.0)
        seed         : int                            (default 42)
        dt           : float                          (default 0.05)
        t_end        : float                          (default 50.0)
        integrator   : "euler" | "rk4"               (default "euler")
        observers    : list of "O1" and/or "O2"      (default ["O1"])
        freq_dist    : {type, gamma/std/...}
        inject_events: list of perturbation events
    """
    global _latest_result, _run_status
    data = request.get_json(silent=True) or {}

    # Set observer default
    if "observers" not in data:
        data["observers"] = ["O1"]

    try:
        with _lock:
            _run_status = {"state": "running", "last_run_at": _time.time()}

        result = run_simulation(data, inject_events=data.get("inject_events"))

        with _lock:
            _latest_result = result
            _run_status = {"state": "idle", "last_run_at": _time.time()}

        # Return summary + first/last 20 rows (avoid huge responses)
        metrics = result["metrics"]
        sample_metrics = (metrics[:20] + metrics[-20:]) if len(metrics) > 40 else metrics

        return jsonify({
            "summary": result["summary"],
            "metrics_sample": sample_metrics,
            "metrics_count": len(metrics),
            "events": result["events"],
        })
    except Exception as exc:
        with _lock:
            _run_status = {"state": "error", "error": str(exc)}
        logger.exception("HLV run failed")
        return jsonify({"error": str(exc)}), 500


@_blueprint.route("/sweep", methods=["POST"])
def sweep_endpoint():
    """Execute a κ-sweep (E1 protocol, HLV.md §F.E1).

    Request body (JSON):
        All fields from /run, plus:
        kappa_min    : float   (default 0.0)
        kappa_max    : float   (default 6.0)
        kappa_step   : float   (default 0.2)
        num_seeds    : int     (default 10)
        seed_base    : int     (default 0)
    """
    global _latest_sweep, _run_status
    data = request.get_json(silent=True) or {}

    if "observers" not in data:
        data["observers"] = ["O1"]

    try:
        with _lock:
            _run_status = {"state": "sweeping", "last_run_at": _time.time()}

        sweep = run_kappa_sweep(data)

        with _lock:
            _latest_sweep = sweep
            _run_status = {"state": "idle", "last_run_at": _time.time()}

        return jsonify(sweep)
    except Exception as exc:
        with _lock:
            _run_status = {"state": "error", "error": str(exc)}
        logger.exception("HLV sweep failed")
        return jsonify({"error": str(exc)}), 500


@_blueprint.route("/inject", methods=["POST"])
def inject_endpoint():
    """Inject a perturbation into the next simulation run.

    This adds a perturbation event to the inject_events list of the run config.
    The perturbation is applied when the next /run call includes inject_events.

    This endpoint is a passthrough helper — the actual injection happens in /run.

    Request body (JSON):
        node      : int or "all"
        type      : "phase_kick" | "omega_kick"
        amplitude : float
        time      : float  (simulation time at which to apply the kick)
        duration  : float  (omega_kick only; default 1.0)
    """
    data = request.get_json(silent=True) or {}
    # Validate and echo the event spec
    ev = {
        "node": data.get("node", "all"),
        "type": data.get("type", "phase_kick"),
        "amplitude": float(data.get("amplitude", 0.1)),
        "time": float(data.get("time", 0.0)),
    }
    if ev["type"] == "omega_kick":
        ev["duration"] = float(data.get("duration", 1.0))

    return jsonify({
        "message": "Perturbation event spec ready — include in inject_events of POST /api/hlv/run",
        "event": ev,
    })


@_blueprint.route("/results", methods=["GET"])
def results_endpoint():
    """Return the latest simulation results — read-only observer."""
    with _lock:
        if _latest_result is None:
            return jsonify({"error": "No results yet. Run a simulation first."}), 404
        metrics = _latest_result["metrics"]
        sample = (metrics[:50] + metrics[-50:]) if len(metrics) > 100 else metrics
        return jsonify({
            "summary": _latest_result["summary"],
            "metrics_sample": sample,
            "metrics_count": len(metrics),
            "events": _latest_result["events"],
        })


@_blueprint.route("/results/full", methods=["GET"])
def results_full_endpoint():
    """Return the full metrics time series — read-only observer."""
    with _lock:
        if _latest_result is None:
            return jsonify({"error": "No results yet."}), 404
        return jsonify({
            "summary": _latest_result["summary"],
            "metrics": _latest_result["metrics"],
            "events": _latest_result["events"],
        })


@_blueprint.route("/artifacts", methods=["GET"])
def artifacts_endpoint():
    """Return the artifact bundle (config, metrics CSV, events, hash) as JSON.

    READ-ONLY OBSERVER: Returns the most recent run's artifact bundle.
    """
    with _lock:
        if _latest_result is None:
            return jsonify({"error": "No results yet."}), 404
        bundle = build_artifact_bundle(_latest_result)
    return jsonify(bundle)


@_blueprint.route("/sweep/results", methods=["GET"])
def sweep_results_endpoint():
    """Return the latest sweep results — read-only observer."""
    with _lock:
        if _latest_sweep is None:
            return jsonify({"error": "No sweep results yet."}), 404
        return jsonify(_latest_sweep)


# ---------------------------------------------------------------------------
# Lifecycle hooks
# ---------------------------------------------------------------------------

def get_blueprint():
    return _blueprint


def activate():
    logger.info("hlv_dynamics plugin activated (FEEN API %s)", FEEN_PLUGIN_API_VERSION)


def deactivate():
    logger.info("hlv_dynamics plugin deactivated")


def unload():
    global _latest_result, _latest_sweep
    with _lock:
        _latest_result = None
        _latest_sweep = None

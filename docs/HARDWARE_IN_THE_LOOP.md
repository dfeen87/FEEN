# FEEN Hardware-in-the-Loop Integration Strategy

**Version 1.0** | Architecture-first design document

---

## Overview

This document defines the hardware-in-the-loop (HIL) integration strategy for FEEN,
covering the hardware adapter abstraction, interface boundaries, REST contract,
invariant preservation, phased rollout, safe hosted operation, and multi-user session
management.

All recommendations are architecture-first and falsifiable. No FEEN core physics are
modified. No feedback paths from observers to dynamics are introduced.

---

## 1. Hardware Adapter Abstraction

### 1.1 Concept

The hardware adapter is an **ablatable, stateless bridge** between physical sensors/
actuators and FEEN simulation state. It lives entirely outside the FEEN core and can
be removed without changing any physics computation.

```
Physical Sensor
      │  raw ADC voltage (V)
      ▼
  [CalibrationParams]
  scale_x, offset_x  →  displacement x [m]
  scale_v, offset_v  →  velocity v [m/s]
  latency_s          →  recorded in SensorSample (informational only)
      │
      ▼
  HardwareAdapter::read_sensor_sample(transducer_id, sim_t)
      │  returns SensorSample { x, v, sample_time_s, latency_s }
      │
      ▼
  HardwareAdapter::apply_to_resonator(resonator, sample)
      │  calls resonator.set_state(x, v, sample_time_s)
      │
      ▼
  [FEEN Resonator — owns all simulation state]
      │
  FEEN physics tick (RK4, dt supplied by scheduler)
      │
      ▼
  resonator.x()  →  compute_actuator_command(x)  →  write_transducer_voltage
      │
      ▼
  Physical Actuator
```

### 1.2 Calibration

All sensor scaling, offset correction, and unit conversion lives in
`CalibrationParams` (see `include/feen/hardware/hardware_adapter.h`).  Physical
parameters (ω₀, γ, β) in `ResonatorConfig` are immutable after construction.

| Parameter         | Role                                          |
|-------------------|-----------------------------------------------|
| `scale_x`         | Sensor voltage → displacement [m/V]           |
| `scale_v`         | Sensor voltage → velocity [(m/s)/V]           |
| `offset_x`        | Zero-point correction for displacement [m]    |
| `offset_v`        | Zero-point correction for velocity [m/s]      |
| `latency_s`       | Known pipeline latency [s] — informational    |
| `actuator_scale`  | FEEN displacement → actuator voltage [V/m]    |
| `actuator_offset` | Actuator voltage zero-offset [V]              |

### 1.3 Latency Handling

Measured pipeline latency (`latency_s`) is recorded in every `SensorSample` for
logging and audit purposes. It is **never** added to or subtracted from the
integrator time variable `t`. Incorporating latency into `dt` would silently distort
the physics. If latency compensation is required for a specific application, it must
be implemented as an explicit, named predictor at the adapter call site — not hidden
inside the adapter itself.

### 1.4 Ablatability

The adapter has zero compile-time dependency from FEEN core. Removing all adapter
calls from a simulation loop leaves `tick()`, `inject()`, and `set_state()` behavior
identical. This is enforced structurally: `hardware_adapter.h` depends on
`resonator.h` and `fpga_driver.h`, but neither of those depend on
`hardware_adapter.h`.

---

## 2. Interface Boundary Definition

```
┌─────────────────────────────────────────────────────────────────────────┐
│ LAYER 4 — Frontend Dashboard (web/)                                     │
│   • Strictly I/O: reads GET endpoints, sends POST commands              │
│   • No physics knowledge; no direct Resonator references                │
│   • Polls read-only endpoints only for visualization                    │
│   • User commands (inject, tick, add node) go through REST POST only   │
└──────────────────────┬──────────────────────────────────────────────────┘
                       │ HTTP (GET/POST)
┌──────────────────────▼──────────────────────────────────────────────────┐
│ LAYER 3 — REST API (python/feen_rest_api.py)                            │
│   • Strictly I/O: routes HTTP to ResonatorNetworkManager methods        │
│   • Owns no simulation state independently                              │
│   • Enforces read/write split: GET = read-only, POST = mutating         │
│   • Does not interpret physics; just serializes/deserializes            │
└──────────────────────┬──────────────────────────────────────────────────┘
                       │ Python method calls
┌──────────────────────▼──────────────────────────────────────────────────┐
│ LAYER 2 — Hardware Adapter (include/feen/hardware/hardware_adapter.h)   │
│   • Strictly I/O: converts sensor readings → set_state(), x() → DAC    │
│   • Owns no simulation state                                            │
│   • Calibration lives here, not in Resonator                           │
│   • Ablatable: FEEN core is unaffected by its presence or absence       │
└──────────────────────┬──────────────────────────────────────────────────┘
                       │ set_state() / inject() / x() / v()
┌──────────────────────▼──────────────────────────────────────────────────┐
│ LAYER 1 — FEEN Core (include/feen/)                                     │
│   • OWNS simulation state                                               │
│   • Physical dynamics: resonator.h, network.h                          │
│   • Numerical integration: sim/integrators.h, sim/scheduler.h          │
│   • Observer layer (no write-back): spiral_time/                        │
│   • All physics parameters are immutable after construction             │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.1 State Ownership Rule

**FEEN core owns all simulation state.** Layers 2, 3, and 4 are strictly I/O.

- Layer 2 (Hardware Adapter): writes state via `set_state()` or `inject()` only;
  reads state via `x()`, `v()`, `t()` only.
- Layer 3 (REST API): serializes state snapshots; never stores state between requests.
- Layer 4 (Dashboard): renders snapshots; has no object references into FEEN.

### 2.2 Spiral-Time Observer Position

The `SpiralTimeObserver` lives inside Layer 1 as a read-only observer. It reads
phases and amplitudes from Resonator objects and produces `SpiralTimeState`. It has
no write path into any Resonator, integrator, or coupling matrix. The REST API and
dashboard may read `SpiralTimeState` fields, but they must never feed them back as
drive terms, calibration corrections, or injection targets.

---

## 3. Safe REST Contract

### 3.1 Endpoint Classification

#### Read-Only Observer Endpoints

These endpoints **must never** call `tick()`, `inject()`, `set_state()`, or `reset()`.
They are safe for keep-alive polling, monitoring tools, and dashboard visualization.

| Endpoint                    | What it returns                            |
|-----------------------------|--------------------------------------------|
| `GET /api/health`           | Service liveness (no simulation access)    |
| `GET /api/network/status`   | Tick counter + simulation time (counters)  |
| `GET /api/network/nodes`    | Snapshot of all node states                |
| `GET /api/network/nodes/N`  | Snapshot of node N                         |
| `GET /api/network/state`    | Full state vector [x₀, v₀, x₁, v₁, …]    |
| `GET /api/config/snapshot`  | Static configuration for audit/replay      |

#### State-Mutating Command Endpoints

These endpoints change simulation state. Each mutation is **explicit, named, and
auditable** — the request body must declare every parameter that affects dynamics.

| Endpoint                           | What it mutates                          |
|------------------------------------|------------------------------------------|
| `POST /api/network/nodes`          | Adds a resonator node (structural)       |
| `POST /api/network/nodes/N/inject` | Injects energy into node N               |
| `POST /api/network/tick`           | Advances simulation time by `dt × steps` |
| `POST /api/network/reset`          | Clears all nodes and resets time         |

### 3.2 Preventing Observer/Visualization Interference

- The dashboard polls only GET endpoints. It never issues a POST as a side-effect
  of rendering a chart or displaying a status value.
- Keep-alive requests (see §6) hit only `GET /api/health` or
  `GET /api/network/status`. These endpoints do not touch any Resonator object.
- Rate-limiting on POST endpoints is the infrastructure-level mechanism for
  preventing accidental or abusive energy injection from untrusted clients.

### 3.3 Injection Auditability

Every call to `POST /api/network/nodes/N/inject` must include `amplitude` and
`phase` in the request body. The response echoes these values alongside the
post-injection state. This makes every energy injection traceable in access logs:
timestamp, node, amplitude, phase, resulting state.

For hardware-in-the-loop use, the adapter's `drive_actuator()` path (FEEN → DAC)
does not use `/inject`; it uses the Transducer model directly. The `/inject`
endpoint is reserved for explicit, user-initiated pulse injection.

### 3.4 Authentication Recommendation (JWT)

For hosted deployments, all POST endpoints should require a JWT Bearer token with
at minimum the following claims:

```
{
  "sub": "<user_or_service_id>",
  "role": "operator",          // "operator" = can POST; "observer" = GET only
  "exp": <unix_timestamp>,
  "session_id": "<uuid>"       // for per-session audit trail
}
```

GET endpoints may remain public or require a lower-privilege `observer` role.
Keep-alive infrastructure requests use a dedicated `keepalive` service account
with `role: observer` and a long-lived token that is rotated out-of-band.

---

## 4. FEEN Invariants That Must Never Be Violated

### 4.1 Energy Dissipation (Thermodynamic Consistency)

The damping term `−2γv` in the Duffing equation ensures energy is removed from the
system over time. This invariant must hold even after `set_state()` overwrites the
resonator with a sensor reading. **The hardware adapter must not suppress or
compensate damping.** If a sensor reading happens to show higher displacement than
the current FEEN state, calling `set_state()` with that value is correct; the
subsequent `tick()` will apply the correct damping to the new initial condition.

*Falsification criterion*: After any sequence of `set_state()` + `tick()` calls with
no `inject()`, total energy must be ≤ the energy at the first `set_state()`.

### 4.2 Observer Separation

`SpiralTimeObserver` reads phases and amplitudes from FEEN state but has **no write
path** back to any Resonator, CouplingMatrix, or integrator. This must remain true
even when the observer's output is displayed on the dashboard or transmitted over the
REST API. Displaying `chi` or `phi` in a chart does not constitute feeding them back.

*Falsification criterion*: Removing all `SpiralTimeObserver::update()` calls leaves
all Resonator trajectories bit-for-bit identical.

### 4.3 Determinism

Given identical initial state and the same sequence of `tick(dt)` calls with the same
`dt`, FEEN produces identical trajectories. The hardware adapter must not introduce
non-determinism into this sequence:
- `dt` must always be a fixed, explicitly supplied value — never derived from
  wall-clock intervals, sensor poll latency, or network round-trip time.
- `set_state()` writes are deterministic: same sensor reading → same state.

*Falsification criterion*: Two simulation runs with identical initial state and tick
sequences must produce identical state vectors at every step.

### 4.4 Ablatability

Every external layer (hardware adapter, REST API, dashboard, Spiral-Time) can be
removed independently without breaking FEEN core. This is an architectural invariant,
not a runtime property. It is enforced by the dependency graph: FEEN core headers
include only C++ standard library headers, not adapter, REST, or web headers.

*Falsification criterion*: `#include <feen/resonator.h>` compiles in a project with
no adapter, no pybind, no Flask, no web code present.

### 4.5 Physics Parameter Immutability

`ResonatorConfig` fields (frequency_hz, q_factor, beta, harmonics) are set at
construction and are not modified at runtime. The hardware adapter does not modify
config; it only modifies dynamic state via `set_state()`. Calibration coefficients
(scale, offset) live in `CalibrationParams`, not in `ResonatorConfig`.

*Falsification criterion*: After any sequence of `set_state()`, `inject()`, `tick()`,
and adapter calls, `resonator.frequency_hz()`, `resonator.q_factor()`, and
`resonator.gamma()` return the values set at construction.

---

## 5. Phased Integration Path

### Phase 1 — Single Physical Resonator, 1:1 Mapping (Local)

**Goal**: Validate the adapter layer with one physical MEMS/NEMS device mapped to one
FEEN `Resonator` node.

**Architecture**:

```
[Physical MEMS Device]
      │  FPGA ADC (single channel)
      ▼
[MEMSCalibration::extract_parameters()]  →  ResonatorConfig
      │
      ▼
[Resonator r(cfg)]
      │
[HardwareAdapter adapter(fpga, cal)]
      │
Loop:
  s = adapter.read_sensor_sample(tid, r.t())
  adapter.apply_to_resonator(r, s)
  r.tick(dt)                               // dt fixed by scheduler
  adapter.drive_actuator(r, tid)
```

**Validation criteria**:
- Resonator energy dissipates at the measured Q-factor rate.
- Physical device response matches FEEN trajectory under identical drive.
- Removing the adapter restores purely simulated behavior.

**Failure modes**:
- Sensor noise causes `set_state()` to inject large step discontinuities.
  *Prevention*: apply a low-pass filter at the adapter (before `set_state()`),
  never inside Resonator.
- Latency causes the actuator to drive with a stale `x()`.
  *Prevention*: record latency; decide at the call site whether to use a
  predictor; never hide it in `dt`.

---

### Phase 2 — Hybrid Simulated + Physical Networks (Local)

**Goal**: Mix physical nodes (hardware-adapter-driven) and simulated nodes (pure FEEN)
in one `ResonatorNetwork`.

**Architecture**:

```
[ResonatorNetwork]
  node 0 — physical (set_state each step from HardwareAdapter)
  node 1 — physical (set_state each step from HardwareAdapter)
  node 2 — simulated (no adapter; evolves by RK4 alone)
  node 3 — simulated

[tick_parallel(dt)] — all nodes advance synchronously
  Physical nodes: adapter writes set_state before tick
  Simulated nodes: tick receives only internal coupling forces
```

**Coupling**: The `CouplingMatrix` is populated with spring coefficients for all node
pairs, regardless of physical/simulated status. The adapter writes only affect the
pre-tick state; the coupling forces are computed from the synchronized snapshot as
usual.

**Validation criteria**:
- Simulated nodes respond to physical node displacement via coupling.
- Physical nodes are unaffected by simulated node errors (because their state
  is overwritten by `set_state()` before each tick).

**Failure modes**:
- Physical and simulated clocks drift: physical node state is always overwritten
  by `set_state(sample_time_s)`, but `sample_time_s` must be set to the FEEN
  network's own `time_s()` to keep timestamps consistent.
- Adapter dropout (hardware unreachable): the last known `set_state()` values
  remain until the next successful read. This is safe but degrades accuracy.
  *Prevention*: detect dropout; switch affected node to simulated mode.

---

### Phase 3 — Remote Hardware Access via REST + Dashboard (Hosted)

**Goal**: Allow a remote user to interact with a hardware-backed FEEN instance via the
REST API and dashboard.

**Architecture**:

```
[Physical Lab Hardware]
      │  (local machine, direct FPGA connection)
      ▼
[FEEN Core + HardwareAdapter + Simulation Loop]
      │  (local process)
      ▼
[REST API — python/feen_rest_api.py]
      │  Gunicorn, HTTPS, JWT authentication
      ▼
[Internet / hosted platform (e.g., Render)]
      │
      ▼
[Frontend Dashboard — web/]
      │  User browser: observe + command
```

**Authentication**: All POST endpoints require JWT with `role: operator`. GET
endpoints require JWT with `role: observer` (or public, depending on policy).

**Simulation loop ownership**: The simulation loop runs **locally** on the lab
machine, not on the hosted platform. The hosted REST API is a thin routing layer. This
ensures the hardware adapter can maintain its FPGA connection and deterministic `dt`
without depending on hosted-platform latency.

**Failure modes**:
- Network interruption stops POST /api/network/tick: simulation pauses gracefully
  (no spurious ticks from keep-alive; see §6).
- Dashboard polls GET endpoints during network hiccup: returns last snapshot; no
  phantom energy injection.

---

## 6. Safe Operational Strategy for Hosted Instances

Hosted platforms (e.g., Render free tier) idle inactive instances after a period of
inactivity. The keep-alive strategy must prevent idling **without influencing physics**.

### 6.1 Keep-Alive Target Endpoints

Use only:
- `GET /api/health` — no simulation object access; returns static JSON.
- `GET /api/network/status` — reads tick/time counters; no mutation.

**Never use** a POST endpoint for keep-alive. A POST to `/tick`, `/inject`, or
`/reset` would advance or corrupt the simulation state.

### 6.2 Keep-Alive Request Properties

- **Authentication**: Use a dedicated service account JWT with `role: observer` and
  `sub: keepalive-agent`. This token should be:
  - Long-lived (rotate monthly or per deployment).
  - Narrowly scoped: the `role: observer` claim prevents it from ever being accepted
    on a POST endpoint.
- **Frequency**: Once every 5–10 minutes is sufficient to prevent idling on most
  platforms. This is infrastructure-level activity; it does not interact with
  simulation state in any way.
- **Payload**: `GET /api/health` returns `{"status": "ok", "service": "FEEN REST API"}`
  and accesses no Resonator objects.

### 6.3 Separation Principle

Keep-alive traffic is **infrastructure-level activity, outside the model**. The
simulation does not observe, record, or react to keep-alive requests. The tick counter
does not increment; no energy is injected; no state is read or written. This is
guaranteed by the architecture: `GET /api/health` returns a hardcoded JSON object.

### 6.4 Failure Mode: Accidental State Mutation in Keep-Alive

*Risk*: An operator mistakenly configures a keep-alive agent to POST to `/tick`.

*Prevention*:
1. The keep-alive service account token carries `role: observer`, which is rejected
   by all POST endpoints.
2. POST endpoints are documented as STATE-MUTATING COMMAND in both code and this
   document; their docstrings explicitly warn against keep-alive use.
3. The distinction is enforced by HTTP method (GET vs POST), not by URL pattern
   alone, so a misconfigured URL cannot accidentally reach a mutating endpoint via GET.

---

## 7. Multi-User / Session Management

### 7.1 Default: Single Shared Simulation

The default deployment mode is one shared `ResonatorNetwork` per server process. This
is appropriate for:
- Live demo with one presenter and a read-only audience.
- Hardware-in-the-loop experiments where a single physical device is shared.

**Cross-user interference** is prevented by:
- POST endpoint authentication (JWT with `role: operator` required).
- Only designated operators can issue tick, inject, add-node, or reset commands.
- Observer-role clients (dashboard viewers) can only read state.

### 7.2 Per-Session Isolation (Optional)

For independent user experiments, each authenticated session creates its own
`ResonatorNetworkManager` instance, identified by a `session_id` claim in the JWT.
All endpoints are scoped to the session: `/api/sessions/<session_id>/network/...`.

**Tradeoffs**:
- Isolation: guaranteed — no cross-session state leakage.
- Cost: one Python object graph per session; acceptable for small node counts.
- Hardware binding: a session can be configured to use real hardware (via adapter) or
  pure simulation. Hardware sessions are limited to one at a time per physical device.

### 7.3 Reproducibility: Configuration Snapshots

`GET /api/config/snapshot` returns the static configuration of all nodes
(frequency, Q, beta, name) without dynamic state. This snapshot can be used to:
- Replay an experiment by POSTing the same node configs to a fresh instance.
- Diff two session configs to confirm they are equivalent.
- Archive the configuration used for a published result.

Dynamic state snapshots (x, v, t for each node) are available from
`GET /api/network/nodes`. Combining both snapshots provides full reproducibility.

### 7.4 Failure Mode: Concurrent POST Commands

*Risk*: Two operators simultaneously POST to `/tick`, causing double-advance of the
simulation clock.

*Prevention*:
- For shared-simulation deployments, the application layer should use a threading
  lock around the simulation loop. Flask's `threaded=True` (default in Gunicorn) runs
  each request in a thread; the `ResonatorNetworkManager.tick_network()` method is not
  thread-safe without a lock.
- For per-session isolation, sessions are independent and concurrent commands on
  different sessions do not interfere.

---

## Recommended Default Deployment Mode

**For the live demo hosted on Render (or equivalent):**

```
Single shared simulation (no per-session isolation)
  • One ResonatorNetwork, maintained in the server process.
  • GET endpoints: public (no auth required for read-only observation).
  • POST endpoints: require JWT with role: operator.
  • Keep-alive: GET /api/health every 5 minutes, role: observer JWT.
  • Dashboard: polls GET /api/network/nodes every 1–2 seconds for visualization.
  • Hardware adapter: not active in the hosted demo (pure simulation).
  • Simulation loop: driven by POST /api/network/tick from a trusted client
    (or a background thread in the server process).
```

**Why this is safe:**
- Observers (dashboard users) cannot mutate state — they only read.
- Energy injection requires a deliberate POST with explicit amplitude and phase.
- Keep-alive does not advance simulation time.
- The hardware adapter is absent; FEEN core physics are unchanged.
- The configuration snapshot makes every demo session reproducible.

---

## See Also

- [`include/feen/hardware/hardware_adapter.h`](../include/feen/hardware/hardware_adapter.h) — C++ adapter interface
- [`include/feen/hardware/fpga_driver.h`](../include/feen/hardware/fpga_driver.h) — FPGA I/O abstraction
- [`include/feen/hardware/mems_calibration.h`](../include/feen/hardware/mems_calibration.h) — parameter extraction from hardware
- [`tests/test_hardware_adapter.cpp`](../tests/test_hardware_adapter.cpp) — invariant validation tests
- [`python/feen_rest_api.py`](../python/feen_rest_api.py) — REST API with endpoint classification
- [`docs/REST_API.md`](REST_API.md) — REST endpoint reference
- [`docs/SPIRAL_TIME.md`](SPIRAL_TIME.md) — Spiral-Time observer design

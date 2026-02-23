/**
 * FEEN Node Graph — client-side rendering
 *
 * Renders FEEN resonator nodes and plugin-provided nodes as a visual graph
 * on an HTML5 canvas.  All physics information is fetched read-only from
 * GET /api/network/nodes and GET /api/plugins.  Graph interactions that
 * trigger physics changes (inject, tick) send explicit POST requests and are
 * clearly labelled in the UI — there are no implicit side-effects.
 *
 * Architectural constraints enforced here:
 *  • No hidden feedback: canvas interactions never silently mutate state.
 *  • Explicit commands only: inject / tick are labelled buttons, not
 *    automatic results of drag-and-drop or clicks.
 *  • Visual connections ≠ physical coupling: edge lines are drawn for
 *    display only; they do not represent any FEEN coupling coefficient.
 *  • Observer separation: polling uses only GET endpoints.
 */

'use strict';

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
const NODE_RADIUS = 28;
const FEEN_NODE_COLOR   = '#1565c0';   // deep blue for physics nodes
const PLUGIN_NODE_COLOR = '#4a148c';   // deep purple for plugin nodes
const SELECTED_RING     = '#03dac6';
const EDGE_COLOR        = 'rgba(255,255,255,0.08)';
const FONT_COLOR        = '#e0e0e0';
const ENERGY_MAX        = 2.0;         // normalise energy bar display

// ---------------------------------------------------------------------------
// State (UI only — no physics state stored here)
// ---------------------------------------------------------------------------
let nodes        = [];     // FEEN physics nodes (from GET /api/network/nodes)
let pluginNodes  = [];     // plugin-provided virtual nodes (from GET /api/plugins)
let selectedId   = null;   // currently selected node id (string)
let graphLayout  = {};     // { id: {x, y} }  — canvas positions
let autoRefresh  = true;
let refreshTimer = null;
const REFRESH_INTERVAL_MS = 1500;

// ---------------------------------------------------------------------------
// DOM references
// ---------------------------------------------------------------------------
let canvas, ctx;

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------
document.addEventListener('DOMContentLoaded', () => {
    canvas = document.getElementById('graph-canvas');
    ctx    = canvas.getContext('2d');

    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);
    canvas.addEventListener('click', onCanvasClick);

    // Toolbar buttons
    document.getElementById('refresh-btn').addEventListener('click', refreshAll);
    document.getElementById('auto-refresh-toggle').addEventListener('change', e => {
        autoRefresh = e.target.checked;
        if (autoRefresh) scheduleRefresh(); else clearTimeout(refreshTimer);
    });
    document.getElementById('inject-selected-btn').addEventListener('click', injectSelected);
    document.getElementById('reset-layout-btn').addEventListener('click', () => {
        graphLayout = {};
        renderGraph();
    });

    // Initial fetch
    refreshAll();
});

// ---------------------------------------------------------------------------
// Canvas helpers
// ---------------------------------------------------------------------------
function resizeCanvas() {
    const rect = canvas.parentElement.getBoundingClientRect();
    canvas.width  = Math.max(600, rect.width - 2);
    canvas.height = 480;
    renderGraph();
}

// ---------------------------------------------------------------------------
// Data fetching — read-only GET endpoints only
// ---------------------------------------------------------------------------
async function refreshAll() {
    await Promise.all([fetchFEENNodes(), fetchPlugins()]);
    renderGraph();
    // Keep the selected node info panel in sync with freshly-fetched data so
    // that live resonator values (energy, x, v) are always current.
    if (selectedId && selectedId.startsWith('feen_')) {
        const nodeId = parseInt(selectedId.replace('feen_', ''), 10);
        const fresh  = nodes.find(n => n.id === nodeId);
        if (fresh) updateSelectedInfo(fresh, 'feen');
    }
    if (autoRefresh) scheduleRefresh();
}

function scheduleRefresh() {
    clearTimeout(refreshTimer);
    refreshTimer = setTimeout(refreshAll, REFRESH_INTERVAL_MS);
}

async function fetchFEENNodes() {
    try {
        const r = await fetch('/api/network/nodes');
        if (!r.ok) return;
        const data = await r.json();
        nodes = data.nodes || [];
        updateNetworkInfo(data);
    } catch (err) {
        console.warn('[FEEN] fetchFEENNodes failed:', err.message);
    }
}

async function fetchPlugins() {
    try {
        const r = await fetch('/api/plugins');
        if (!r.ok) { pluginNodes = []; return; }
        const data = await r.json();
        pluginNodes = (data.plugins || []).filter(p => p.state === 'active');
        renderPluginList(data.plugins || []);
    } catch (err) {
        console.warn('[FEEN] fetchPlugins failed:', err.message);
        pluginNodes = [];
    }
}

// ---------------------------------------------------------------------------
// Graph rendering
// ---------------------------------------------------------------------------
function renderGraph() {
    if (!ctx) return;
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const allNodes = buildDisplayNodes();
    ensureLayout(allNodes);
    drawEdges(allNodes);
    allNodes.forEach(n => drawNode(n));
    updateStatus(allNodes.length);
}

/**
 * Build the flat list of display nodes merging FEEN nodes + plugin nodes.
 * Each display node has: { id, label, kind, data }
 */
function buildDisplayNodes() {
    const result = [];

    nodes.forEach(n => result.push({
        id:    'feen_' + n.id,
        label: n.name || ('N' + n.id),
        kind:  'feen',
        data:  n,
    }));

    pluginNodes.forEach(p => result.push({
        id:    'plugin_' + p.name,
        label: p.name,
        kind:  'plugin',
        data:  p,
    }));

    return result;
}

/**
 * Assign canvas positions to new nodes without disturbing existing positions.
 * Uses a simple circular layout as the default.
 */
function ensureLayout(allNodes) {
    const cx = canvas.width  / 2;
    const cy = canvas.height / 2;
    const r  = Math.min(cx, cy) * 0.6;
    const total = allNodes.length || 1;

    allNodes.forEach((n, i) => {
        if (!graphLayout[n.id]) {
            const angle = (2 * Math.PI * i) / total - Math.PI / 2;
            graphLayout[n.id] = {
                x: cx + r * Math.cos(angle),
                y: cy + r * Math.sin(angle),
            };
        }
    });
}

/**
 * Draw faint edges between sequential FEEN nodes to suggest adjacency.
 * These edges are purely visual — they do NOT represent physical coupling.
 */
function drawEdges(allNodes) {
    const feenNodes = allNodes.filter(n => n.kind === 'feen');
    ctx.strokeStyle = EDGE_COLOR;
    ctx.lineWidth   = 1;
    for (let i = 0; i + 1 < feenNodes.length; i++) {
        const a = graphLayout[feenNodes[i].id];
        const b = graphLayout[feenNodes[i + 1].id];
        if (!a || !b) continue;
        ctx.beginPath();
        ctx.moveTo(a.x, a.y);
        ctx.lineTo(b.x, b.y);
        ctx.stroke();
    }
}

function drawNode(n) {
    const pos = graphLayout[n.id];
    if (!pos) return;

    const isSelected = (n.id === selectedId);
    const color = n.kind === 'feen' ? FEEN_NODE_COLOR : PLUGIN_NODE_COLOR;

    // Selection ring
    if (isSelected) {
        ctx.beginPath();
        ctx.arc(pos.x, pos.y, NODE_RADIUS + 5, 0, 2 * Math.PI);
        ctx.strokeStyle = SELECTED_RING;
        ctx.lineWidth   = 2;
        ctx.stroke();
    }

    // Energy fill (FEEN nodes only)
    if (n.kind === 'feen' && n.data) {
        const energy    = Math.min(Math.abs(n.data.energy || 0), ENERGY_MAX);
        const fillAngle = (energy / ENERGY_MAX) * 2 * Math.PI;
        ctx.beginPath();
        ctx.moveTo(pos.x, pos.y);
        ctx.arc(pos.x, pos.y, NODE_RADIUS - 2, -Math.PI / 2, -Math.PI / 2 + fillAngle);
        ctx.closePath();
        ctx.fillStyle = 'rgba(3,218,198,0.2)';
        ctx.fill();
    }

    // Node circle
    ctx.beginPath();
    ctx.arc(pos.x, pos.y, NODE_RADIUS, 0, 2 * Math.PI);
    ctx.fillStyle   = color + '99';
    ctx.strokeStyle = color;
    ctx.lineWidth   = 1.5;
    ctx.fill();
    ctx.stroke();

    // Label
    ctx.fillStyle  = FONT_COLOR;
    ctx.font       = '11px monospace';
    ctx.textAlign  = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(n.label, pos.x, pos.y);
}

// ---------------------------------------------------------------------------
// Canvas interaction — explicit, labelled actions only
// ---------------------------------------------------------------------------
function onCanvasClick(e) {
    const rect = canvas.getBoundingClientRect();
    const mx   = (e.clientX - rect.left) * (canvas.width  / rect.width);
    const my   = (e.clientY - rect.top)  * (canvas.height / rect.height);

    const allNodes = buildDisplayNodes();
    let hit = null;
    for (const n of allNodes) {
        const pos = graphLayout[n.id];
        if (!pos) continue;
        const dx = mx - pos.x, dy = my - pos.y;
        if (dx * dx + dy * dy <= (NODE_RADIUS + 5) ** 2) { hit = n; break; }
    }

    selectedId = hit ? hit.id : null;
    renderGraph();
    updateSelectedInfo(hit ? hit.data : null, hit ? hit.kind : null);
}

// ---------------------------------------------------------------------------
// Explicit command: inject energy into selected FEEN node
// Labelled button — never triggered automatically by observation or rendering.
// ---------------------------------------------------------------------------
async function injectSelected() {
    if (!selectedId || !selectedId.startsWith('feen_')) {
        log('Select a FEEN node first.');
        return;
    }
    const nodeId = selectedId.replace('feen_', '');
    log(`Injecting into node ${nodeId}…`);
    try {
        const r = await fetch(`/api/network/nodes/${nodeId}/inject`, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({ amplitude: 1.0, phase: 0.0 }),
        });
        if (r.ok) {
            log(`Injected into node ${nodeId}.`);
            await fetchFEENNodes();
            renderGraph();
            // Refresh the selected-node panel with the updated state
            const fresh = nodes.find(n => n.id === parseInt(nodeId, 10));
            if (fresh) updateSelectedInfo(fresh, 'feen');
        } else {
            const d = await r.json();
            log(`Error: ${d.error}`);
        }
    } catch (err) {
        log(`Network error: ${err.message}`);
    }
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------
function updateNetworkInfo(data) {
    const el = document.getElementById('graph-net-info');
    if (!el) return;
    el.textContent = `Nodes: ${(data.nodes || []).length}`;
}

function updateStatus(nodeCount) {
    const el = document.getElementById('graph-status');
    if (el) el.textContent = `Displaying ${nodeCount} node(s) — visual only`;
}

function updateSelectedInfo(data, kind) {
    const el = document.getElementById('selected-info');
    if (!el) return;
    if (!data) { el.innerHTML = '<span style="color:#555">No node selected</span>'; return; }

    if (kind === 'feen') {
        el.innerHTML = `
          <div class="field"><span class="label">ID</span><span class="value">${data.id}</span></div>
          <div class="field"><span class="label">Name</span><span class="value">${data.name || '—'}</span></div>
          <div class="field"><span class="label">x</span><span class="value">${fmt(data.x)}</span></div>
          <div class="field"><span class="label">v</span><span class="value">${fmt(data.v)}</span></div>
          <div class="field"><span class="label">Energy</span><span class="value">${fmt(data.energy)}</span></div>
          <div class="field"><span class="label">SNR</span><span class="value">${fmt(data.snr)}</span></div>`;
    } else {
        el.innerHTML = `
          <div class="field"><span class="label">Plugin</span><span class="value">${data.name}</span></div>
          <div class="field"><span class="label">Type</span><span class="value">${data.type}</span></div>
          <div class="field"><span class="label">State</span><span class="value">${data.state}</span></div>
          <div class="field"><span class="label">Version</span><span class="value">${(data.version || []).join('.')}</span></div>`;
    }
}

function renderPluginList(plugins) {
    const el = document.getElementById('plugin-list');
    if (!el) return;
    const newHtml = !plugins.length
        ? '<li style="color:#555">No plugins loaded</li>'
        : plugins.map(p => `
      <li class="plugin-row">
        <span><span class="name">${p.name}</span><span class="type">${p.type}</span></span>
        <span class="state-badge ${p.state}">${p.state}</span>
      </li>`).join('');
    // Only touch the DOM when content actually changes to prevent flicker.
    if (el.innerHTML !== newHtml) el.innerHTML = newHtml;
}

function fmt(v) {
    return (typeof v === 'number') ? v.toExponential(3) : '—';
}

function log(msg) {
    const el = document.getElementById('graph-log');
    if (!el) return;
    const p = document.createElement('p');
    p.textContent = '> ' + msg;
    p.className   = 'log-entry';
    el.prepend(p);
    while (el.children.length > 5) el.lastElementChild.remove();
}

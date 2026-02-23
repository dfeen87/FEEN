document.addEventListener('DOMContentLoaded', () => {
    const ctx = document.getElementById('resonatorChart').getContext('2d');
    let chart;
    let isRunning = false;
    let speed = 5;
    const dt = 1e-4; // Small time step for physics

    // Chart initialization
    chart = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: [],
            datasets: [{
                label: 'Resonator Displacement (x)',
                data: [],
                backgroundColor: 'rgba(54, 162, 235, 0.6)',
                borderColor: 'rgba(54, 162, 235, 1)',
                borderWidth: 1
            },
            {
                label: 'Resonator Velocity (v)',
                data: [],
                backgroundColor: 'rgba(255, 99, 132, 0.6)',
                borderColor: 'rgba(255, 99, 132, 1)',
                borderWidth: 1
            }]
        },
        options: {
            responsive: true,
            animation: false, // Disable chart animation for performance
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: false,
                    suggestedMin: -2.0,
                    suggestedMax: 2.0
                }
            }
        }
    });

    // UI Elements
    const tickBtn = document.getElementById('tick-btn');
    const injectBtn = document.getElementById('inject-btn');
    const addNodeBtn = document.getElementById('add-node-btn');
    const resetBtn = document.getElementById('reset-btn');
    const speedControl = document.getElementById('speed-control');
    const speedVal = document.getElementById('speed-val');
    const logConsole = document.getElementById('log-console');

    // Controls
    tickBtn.addEventListener('click', () => {
        isRunning = !isRunning;
        tickBtn.textContent = isRunning ? 'Pause Simulation' : 'Start Simulation';
        tickBtn.classList.toggle('active', isRunning);
        if (isRunning) tick();
    });

    injectBtn.addEventListener('click', async () => {
        log("Injecting energy into Node 0...");
        try {
            const response = await fetch('/api/network/nodes/0/inject', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ amplitude: 1.0, phase: 0.0 })
            });
            if (response.ok) {
                // Fetch state immediately to see update
                fetchNodes();
            } else {
                const data = await response.json();
                log(`Error: ${data.error}`);
            }
        } catch (e) {
            log(`Network error: ${e.message}`);
        }
    });

    addNodeBtn.addEventListener('click', async () => {
        try {
            const response = await fetch('/api/network/nodes', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    frequency_hz: 1000.0,
                    q_factor: 200.0,
                    beta: 1e-4,
                    name: `node_${chart.data.labels.length}`
                })
            });
            const data = await response.json();
            log(`Added Node ${data.id}`);
            fetchNodes();
        } catch (e) {
            log(`Error adding node: ${e.message}`);
        }
    });

    resetBtn.addEventListener('click', async () => {
        if (confirm('Reset entire network?')) {
            await fetch('/api/network/reset', { method: 'POST' });
            log("Network reset.");
            fetchNodes();
        }
    });

    speedControl.addEventListener('input', (e) => {
        speed = parseInt(e.target.value);
        speedVal.textContent = speed;
    });

    // Simulation Loop
    async function tick() {
        if (!isRunning) return;

        try {
            const response = await fetch('/api/network/tick', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ dt: dt, steps: speed })
            });

            if (!response.ok) throw new Error("Tick failed");

            const data = await response.json();
            updateUI(data);

            if (isRunning) {
                requestAnimationFrame(tick);
            }
        } catch (error) {
            console.error('Tick error:', error);
            log(`Simulation error: ${error.message}`);
            isRunning = false;
            tickBtn.textContent = 'Start Simulation';
            tickBtn.classList.remove('active');
        }
    }

    async function fetchNodes() {
        try {
            // We can reuse the tick endpoint with 0 steps just to get state if needed,
            // or use /api/network/nodes. Let's use /api/network/nodes for full refresh.
            const response = await fetch('/api/network/nodes');
            const data = await response.json();

            // To match updateUI signature which expects {nodes: [...], status: {...}}
            // But /nodes returns {nodes: [...], count: ...}
            // And doesn't return status. So let's fetch status too.
            const statusResp = await fetch('/api/network/status');
            const statusData = await statusResp.json();

            updateUI({
                nodes: data.nodes,
                status: statusData
            });
        } catch (e) {
            console.error("Fetch error:", e);
        }
    }

    function updateUI(data) {
        const nodes = data.nodes || [];

        // Update Node Count
        document.getElementById('node-count').textContent = nodes.length;

        // Update Chart Labels
        if (chart.data.labels.length !== nodes.length) {
             chart.data.labels = nodes.map(n => `N${n.id}`);
        }

        // Update Chart Data
        chart.data.datasets[0].data = nodes.map(n => n.x);
        chart.data.datasets[1].data = nodes.map(n => n.v);
        chart.update();

        // Update Info
        if (data.status) {
            document.getElementById('sim-time').textContent = (data.status.time || 0).toFixed(3);
            document.getElementById('sim-ticks').textContent = data.status.ticks || 0;
        }
    }

    function log(msg) {
        const p = document.createElement('p');
        p.textContent = `> ${msg}`;
        p.className = 'log-entry';
        logConsole.prepend(p);
        // Keep log size manageable
        if (logConsole.children.length > 5) {
            logConsole.lastElementChild.remove();
        }
    }

    // Initialize
    fetchNodes();
});

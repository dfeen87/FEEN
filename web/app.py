import sys
import os
from flask import Flask, send_from_directory, render_template

# Ensure we can import from python/ directory
python_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'python'))
if python_dir not in sys.path:
    sys.path.append(python_dir)

try:
    from feen_rest_api import app, network
except ImportError as e:
    print(f"Error: Could not import feen_rest_api from {python_dir}")
    print(f"Details: {e}")
    sys.exit(1)

# Set up template and static folders relative to this file
base_dir = os.path.dirname(os.path.abspath(__file__))
app.template_folder = os.path.join(base_dir, 'templates')
app.static_folder = os.path.join(base_dir, 'static')

@app.route('/dashboard')
def dashboard():
    """Serve the main dashboard page."""
    return render_template('index.html')


@app.route('/node-graph')
def node_graph():
    """Serve the node-graph plugin visualization page."""
    return render_template('node_graph.html')

@app.route('/static/<path:filename>')
def custom_static(filename):
    """Serve static files from web/static."""
    return send_from_directory(app.static_folder, filename)

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    # In production (Render), gunicorn will be used, so app.run is not called.
    # But for local dev:
    app.run(host='0.0.0.0', port=port)

# SPDX-License-Identifier: GPL-2.0
# Subsurface Web UI - Flask Application

import json
import os
import subprocess
import tempfile
from flask import Flask, render_template, request, jsonify, abort, send_from_directory

# Get the directory containing app.py
APP_DIR = os.path.dirname(os.path.abspath(__file__))
# Get the Subsurface root directory (parent of webui)
SUBSURFACE_ROOT = os.path.dirname(APP_DIR)

app = Flask(__name__, static_folder='static')


@app.route('/static/subsurface-icon.png')
def serve_icon():
    """Serve the Subsurface icon from the icons directory."""
    return send_from_directory(os.path.join(SUBSURFACE_ROOT, 'icons'), 'subsurface-icon.png')

# Load configuration
config_name = os.environ.get('FLASK_CONFIG', 'config.DevelopmentConfig')
app.config.from_object(config_name)


def get_authenticated_user():
    """Get the authenticated user from headers or dev mode."""
    if app.config.get('DEV_MODE'):
        return app.config.get('DEV_USER', 'dev@localhost')

    auth_header = app.config.get('AUTH_HEADER', 'X-Authenticated-User')
    user = request.headers.get(auth_header)
    if not user:
        abort(401, description="Authentication required")
    return user


def get_cli_config_path(user):
    """Get or create CLI config file path for the user."""
    # If a single config path is specified, use it
    if app.config.get('CLI_CONFIG_PATH'):
        return app.config['CLI_CONFIG_PATH']

    # Multi-tenant mode: create per-user config
    repo_base = app.config.get('REPO_BASE_PATH')
    if not repo_base:
        abort(500, description="Server configuration error: no repo path configured")

    # Create temp config file for this user
    temp_dir = app.config.get('TEMP_DIR', '/tmp/subsurface-webui')
    os.makedirs(temp_dir, exist_ok=True)

    # User-specific repo path (this is a simplified example)
    # In production, you'd look up the user's repo path from a database
    user_repo = os.path.join(repo_base, user.replace('@', '_at_').replace('.', '_'))

    config_data = {
        "repo_path": user_repo,
        "userid": user,
        "temp_dir": os.path.join(temp_dir, "profiles")
    }

    # Write temp config file
    config_file = os.path.join(temp_dir, f"config_{user.replace('@', '_')}.json")
    with open(config_file, 'w') as f:
        json.dump(config_data, f)

    return config_file


def run_cli(command, args=None):
    """Run subsurface-cli and return parsed JSON output."""
    cli_path = app.config.get('CLI_PATH', 'subsurface-cli')
    user = get_authenticated_user()
    config_path = get_cli_config_path(user)

    cmd = [cli_path, f'--config={config_path}', command]
    if args:
        cmd.extend(args)

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30
        )

        if result.returncode != 0 and not result.stdout:
            # CLI failed without JSON output
            return {
                'status': 'error',
                'error_code': 'CLI_ERROR',
                'message': result.stderr or f'CLI exited with code {result.returncode}'
            }

        # Parse JSON output
        return json.loads(result.stdout)

    except subprocess.TimeoutExpired:
        return {
            'status': 'error',
            'error_code': 'TIMEOUT',
            'message': 'CLI command timed out'
        }
    except json.JSONDecodeError as e:
        return {
            'status': 'error',
            'error_code': 'PARSE_ERROR',
            'message': f'Failed to parse CLI output: {e}'
        }
    except FileNotFoundError:
        return {
            'status': 'error',
            'error_code': 'CLI_NOT_FOUND',
            'message': f'CLI not found at: {cli_path}'
        }


# --- Web Routes ---

@app.route('/')
def index():
    """Main page - show dive list."""
    # Get newest 20 dives/trips
    result = run_cli('list-dives', ['--start=0', '--count=20'])

    if result.get('status') == 'error':
        return render_template('error.html', error=result), 500

    return render_template('dive_list.html',
                         items=result.get('items', []),
                         total_dives=result.get('total_dives', 0),
                         total_trips=result.get('total_trips', 0),
                         statistics=result.get('statistics', {}))


@app.route('/trip/<path:trip_ref>')
def trip_detail(trip_ref):
    """Show trip details."""
    result = run_cli('get-trip', [f'--trip-ref={trip_ref}'])

    if result.get('status') == 'error':
        return render_template('error.html', error=result), 404 if result.get('error_code') == 'NOT_FOUND' else 500

    return render_template('trip_detail.html',
                         trip=result.get('trip', {}),
                         dives=result.get('dives', []))


@app.route('/dive/<path:dive_ref>')
def dive_detail(dive_ref):
    """Show dive details."""
    result = run_cli('get-dive', [f'--dive-ref={dive_ref}'])

    if result.get('status') == 'error':
        return render_template('error.html', error=result), 404 if result.get('error_code') == 'NOT_FOUND' else 500

    return render_template('dive_detail.html', dive=result.get('dive', {}))


# --- API Routes ---

@app.route('/api/dives')
def api_list_dives():
    """API: List dives with pagination."""
    start = request.args.get('start', 0, type=int)
    count = request.args.get('count', 50, type=int)

    result = run_cli('list-dives', [f'--start={start}', f'--count={count}'])
    return jsonify(result)


@app.route('/api/trip/<path:trip_ref>')
def api_get_trip(trip_ref):
    """API: Get trip details."""
    result = run_cli('get-trip', [f'--trip-ref={trip_ref}'])
    status_code = 200 if result.get('status') == 'success' else 404 if result.get('error_code') == 'NOT_FOUND' else 500
    return jsonify(result), status_code


@app.route('/api/dive/<path:dive_ref>')
def api_get_dive(dive_ref):
    """API: Get dive details."""
    result = run_cli('get-dive', [f'--dive-ref={dive_ref}'])
    status_code = 200 if result.get('status') == 'success' else 404 if result.get('error_code') == 'NOT_FOUND' else 500
    return jsonify(result), status_code


@app.route('/api/stats')
def api_get_stats():
    """API: Get statistics."""
    result = run_cli('get-stats')
    return jsonify(result)


if __name__ == '__main__':
    app.run(debug=True, host='127.0.0.1', port=5000)

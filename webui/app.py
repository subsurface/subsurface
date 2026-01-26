# SPDX-License-Identifier: GPL-2.0
# Subsurface Web UI - Flask Application

import json
import os
import re
import secrets
import subprocess
from flask import Flask, render_template, request, jsonify, abort, send_from_directory, session, redirect

# Security: Regex patterns for validating input parameters
# These must match the patterns used in the CLI (dive_ref.cpp)
DIVE_REF_PATTERN = re.compile(r"^(\d{4})/(\d{2})/(\d{2})-\w{3}-(\d{2})[=:](\d{2})[=:](\d{2})$")
TRIP_REF_PATTERN = re.compile(r"^(\d{4})/(\d{2})/(\d{2})-([\w\s-]{1,50})$")
# Email pattern matching CLI validation (config.cpp)
EMAIL_PATTERN = re.compile(
    r"^[a-zA-Z0-9._+-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]*[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]*[a-zA-Z0-9])?)+$"
)

# Get the directory containing app.py
APP_DIR = os.path.dirname(os.path.abspath(__file__))
# Get the Subsurface root directory (parent of webui)
SUBSURFACE_ROOT = os.path.dirname(APP_DIR)

app = Flask(__name__, static_folder="static")


@app.route("/static/subsurface-icon.png")
def serve_icon():
    """Serve the Subsurface icon from the icons directory."""
    return send_from_directory(os.path.join(SUBSURFACE_ROOT, "icons"), "subsurface-icon.png")


# Load configuration
config_name = os.environ.get("FLASK_CONFIG", "config.DevelopmentConfig")
app.config.from_object(config_name)


# Security: Add security headers to all responses
@app.after_request
def add_security_headers(response):
    """Add security headers to all responses."""
    # Prevent clickjacking
    response.headers["X-Frame-Options"] = "DENY"
    # Prevent MIME type sniffing
    response.headers["X-Content-Type-Options"] = "nosniff"
    # XSS protection (legacy, but still useful)
    response.headers["X-XSS-Protection"] = "1; mode=block"
    # Referrer policy
    response.headers["Referrer-Policy"] = "strict-origin-when-cross-origin"
    # Content Security Policy - restrict sources
    csp = (
        "default-src 'self'; "
        "script-src 'self' 'unsafe-inline'; "  # unsafe-inline needed for inline scripts in templates
        "style-src 'self' 'unsafe-inline' https://fonts.googleapis.com; "
        "font-src 'self' https://fonts.gstatic.com; "
        "img-src 'self' data:; "
        "frame-ancestors 'none';"
    )
    response.headers["Content-Security-Policy"] = csp
    return response


def validate_dive_ref(dive_ref):
    """Validate dive reference format. Returns True if valid."""
    if not dive_ref or len(dive_ref) > 30:
        return False
    return DIVE_REF_PATTERN.match(dive_ref) is not None


def validate_trip_ref(trip_ref):
    """Validate trip reference format. Returns True if valid."""
    if not trip_ref or len(trip_ref) > 100:
        return False
    return TRIP_REF_PATTERN.match(trip_ref) is not None


def validate_email(email):
    """Validate email format. Returns True if valid."""
    if not email or len(email) > 254:
        return False
    return EMAIL_PATTERN.match(email) is not None


class AuthenticationRequired(Exception):
    """Raised when authentication is required but user is not logged in."""

    pass


@app.errorhandler(AuthenticationRequired)
def handle_auth_required(e):
    """Redirect to login page when authentication is required."""
    return redirect(f"/login?next={request.path}")


def get_authenticated_user():
    """Get the authenticated user from Flask-Login session or dev mode.

    Authentication is handled by the flask-auth-proxy app on port 5000.
    Both apps share the same SECRET_KEY so they can read each other's sessions.
    Flask-Login stores the user ID in session['_user_id'].
    """
    if app.config.get("DEV_MODE"):
        return app.config.get("DEV_USER", "dev@localhost")

    # Check for Flask-Login session (shared with flask-auth-proxy)
    # Flask-Login stores the user ID in '_user_id' key
    user = session.get("_user_id")
    if not user:
        # Not authenticated
        # Use abort with 401 for API requests, custom exception for web pages
        if request.path.startswith("/api/"):
            abort(401, description="Authentication required")
        raise AuthenticationRequired()

    # Security: Validate user format to prevent injection attacks
    if not validate_email(user):
        abort(400, description="Invalid user format")

    return user


def get_cli_config_path(user):
    """Get or create CLI config file path for the user.

    Security considerations:
    - Config files are created with random names to prevent symlink attacks
    - User email is validated before use in paths
    - All paths are validated to prevent traversal attacks
    """
    # If a single config path is specified, use it
    if app.config.get("CLI_CONFIG_PATH"):
        return app.config["CLI_CONFIG_PATH"]

    # Multi-tenant mode: create per-user config
    repo_base = app.config.get("REPO_BASE_PATH")
    if not repo_base:
        abort(500, description="Server configuration error")

    # Security: Validate repo_base is an absolute path without traversal
    if not os.path.isabs(repo_base) or ".." in repo_base:
        abort(500, description="Server configuration error")

    # Create temp config directory with secure random subdirectory
    temp_base = app.config.get("TEMP_DIR", "/tmp/subsurface-webui")

    # Security: Create a random subdirectory for this request
    # This prevents symlink attacks since the path is unpredictable
    random_token = secrets.token_hex(16)
    temp_dir = os.path.join(temp_base, random_token)

    try:
        os.makedirs(temp_dir, mode=0o700, exist_ok=False)
    except FileExistsError:
        # Extremely unlikely with 128 bits of randomness, but handle it
        abort(500, description="Failed to create secure temp directory")

    # Security: Sanitize username for use in path
    # Replace characters that could be problematic in paths
    # Note: @ is allowed since git repos use email addresses as directory names
    safe_user = re.sub(r"[^a-zA-Z0-9._+@-]", "_", user)
    if not safe_user or safe_user.startswith("."):
        abort(400, description="Invalid user format")

    # User-specific repo path
    # In production, you'd look up the user's repo path from a database
    user_repo = os.path.join(repo_base, safe_user)

    # Security: Verify the constructed path is still under repo_base
    # (defense against any edge cases in path construction)
    real_repo_base = os.path.realpath(repo_base)
    real_user_repo = os.path.realpath(user_repo)
    if not real_user_repo.startswith(real_repo_base + os.sep):
        abort(400, description="Invalid user")

    # For bare git repos, the branch is the user email
    # Format: /path/to/repo[branch]
    repo_path_with_branch = f"{user_repo}[{user}]"

    config_data = {"repo_path": repo_path_with_branch, "userid": user, "temp_dir": os.path.join(temp_dir, "profiles")}

    # Write temp config file with restrictive permissions
    config_file = os.path.join(temp_dir, "config.json")
    fd = os.open(config_file, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        with os.fdopen(fd, "w") as f:
            json.dump(config_data, f)
    except:
        os.close(fd)
        raise

    return config_file


def cleanup_temp_dir(config_path):
    """Clean up temporary config directory after use."""
    try:
        temp_dir = os.path.dirname(config_path)
        # Only clean up if it looks like our random temp dir
        if "/subsurface-webui/" in temp_dir and len(os.path.basename(temp_dir)) == 32:
            import shutil

            shutil.rmtree(temp_dir, ignore_errors=True)
    except Exception:
        pass  # Best effort cleanup


def run_cli(command, args=None):
    """Run subsurface-cli and return parsed JSON output.

    Security: Error messages are sanitized to avoid leaking internal paths.
    """
    cli_path = app.config.get("CLI_PATH", "subsurface-cli")
    user = get_authenticated_user()
    config_path = get_cli_config_path(user)

    cmd = [cli_path, f"--config={config_path}", command]
    if args:
        cmd.extend(args)

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

        if result.returncode != 0 and not result.stdout:
            # Security: Don't leak stderr details which may contain paths
            return {"status": "error", "error_code": "CLI_ERROR", "message": "Command failed"}

        # Parse JSON output
        return json.loads(result.stdout)

    except subprocess.TimeoutExpired:
        return {"status": "error", "error_code": "TIMEOUT", "message": "Request timed out"}
    except json.JSONDecodeError:
        # Security: Don't include parse error details
        return {"status": "error", "error_code": "INTERNAL_ERROR", "message": "Internal error"}
    except FileNotFoundError:
        # Security: Don't reveal CLI path
        return {"status": "error", "error_code": "SERVICE_UNAVAILABLE", "message": "Service temporarily unavailable"}
    finally:
        # Clean up temp config directory
        if config_path and not app.config.get("CLI_CONFIG_PATH"):
            cleanup_temp_dir(config_path)


# --- Web Routes ---


@app.route("/")
def index():
    """Main page - show dive list."""
    # Get newest 20 dives/trips
    result = run_cli("list-dives", ["--start=0", "--count=20"])

    if result.get("status") == "error":
        return render_template("error.html", error=result), 500

    return render_template(
        "dive_list.html",
        items=result.get("items", []),
        total_dives=result.get("total_dives", 0),
        total_trips=result.get("total_trips", 0),
        statistics=result.get("statistics", {}),
    )


@app.route("/trip/<path:trip_ref>")
def trip_detail(trip_ref):
    """Show trip details."""
    # Security: Validate trip_ref format before passing to CLI
    if not validate_trip_ref(trip_ref):
        abort(400, description="Invalid trip reference format")

    result = run_cli("get-trip", [f"--trip-ref={trip_ref}"])

    if result.get("status") == "error":
        return render_template("error.html", error=result), 404 if result.get("error_code") == "NOT_FOUND" else 500

    return render_template("trip_detail.html", trip=result.get("trip", {}), dives=result.get("dives", []))


@app.route("/dive/<path:dive_ref>")
def dive_detail(dive_ref):
    """Show dive details."""
    # Security: Validate dive_ref format before passing to CLI
    if not validate_dive_ref(dive_ref):
        abort(400, description="Invalid dive reference format")

    result = run_cli("get-dive", [f"--dive-ref={dive_ref}"])

    if result.get("status") == "error":
        return render_template("error.html", error=result), 404 if result.get("error_code") == "NOT_FOUND" else 500

    return render_template("dive_detail.html", dive=result.get("dive", {}))


# --- API Routes ---

# Security: Pagination limits (must match CLI limits in commands.cpp)
MAX_START = 100000
MAX_COUNT = 500
DEFAULT_COUNT = 50


@app.route("/api/dives")
def api_list_dives():
    """API: List dives with pagination."""
    start = request.args.get("start", 0, type=int)
    count = request.args.get("count", DEFAULT_COUNT, type=int)

    # Security: Validate and clamp pagination parameters
    start = max(0, min(start, MAX_START))
    count = max(1, min(count, MAX_COUNT))

    result = run_cli("list-dives", [f"--start={start}", f"--count={count}"])
    return jsonify(result)


@app.route("/api/trip/<path:trip_ref>")
def api_get_trip(trip_ref):
    """API: Get trip details."""
    # Security: Validate trip_ref format
    if not validate_trip_ref(trip_ref):
        return jsonify({"status": "error", "error_code": "INVALID_FORMAT", "message": "Invalid trip reference format"}), 400

    result = run_cli("get-trip", [f"--trip-ref={trip_ref}"])
    status_code = 200 if result.get("status") == "success" else 404 if result.get("error_code") == "NOT_FOUND" else 500
    return jsonify(result), status_code


@app.route("/api/dive/<path:dive_ref>")
def api_get_dive(dive_ref):
    """API: Get dive details."""
    # Security: Validate dive_ref format
    if not validate_dive_ref(dive_ref):
        return jsonify({"status": "error", "error_code": "INVALID_FORMAT", "message": "Invalid dive reference format"}), 400

    result = run_cli("get-dive", [f"--dive-ref={dive_ref}"])
    status_code = 200 if result.get("status") == "success" else 404 if result.get("error_code") == "NOT_FOUND" else 500
    return jsonify(result), status_code


@app.route("/api/stats")
def api_get_stats():
    """API: Get statistics."""
    result = run_cli("get-stats")
    return jsonify(result)


if __name__ == "__main__":
    app.run(debug=True, host="127.0.0.1", port=5001)

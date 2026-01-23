# Subsurface Web UI - Design Document

## Project Overview

Add a web-based user interface to Subsurface that leverages the existing C++ core logic without reimplementing it. The web UI will be read-only initially and support both single-user (local) and multi-tenant (cloud server) deployment models.

### Key Goals
- Reuse 80k+ LOC of existing C++/Qt core logic
- Avoid the scalability problems of the current HTML export (which generates hundreds of MB to GB of data)
- Transfer only the data needed for the current view to the browser (typically 1-5 PNGs plus a few KB of metadata)
- Support users with anywhere from <100 to 5000+ dives
- Work in both standalone and multi-tenant cloud deployment scenarios

## Architecture

### High-Level Design

```
┌─────────────────┐
│   Web Browser   │
│   (User's)      │
└────────┬────────┘
         │ HTTPS
         ↓
┌─────────────────┐
│  Flask Web App  │
│   (Python)      │
└────────┬────────┘
         │ subprocess/CLI
         ↓
┌─────────────────┐
│  subsurface-cli │
│   CLI Tool      │
│   (C++/Qt)      │
└────────┬────────┘
         │
         ↓
┌─────────────────┐
│  Cloud Storage  │
│  (Git Repo)     │
└─────────────────┘
```

### Why This Architecture?

**CLI Tool Approach (Option B from discussion):**
- Stateless operations - no Qt event loop complications
- Clean separation between web layer (Python) and core logic (C++)
- Reuses existing C++ code (largely) without modification
- Simple to debug (can test CLI tool independently)
- Scales well for multi-tenant (process per request or process pooling)
- CLI tool may have other use cases beyond web UI

**Trade-offs:**
- Some overhead from process spawning (mitigated by keeping processes warm or using fast startup)
- Need to pass data via JSON serialization

---

## Dive and Trip Identification

Dives and trips are identified using their git storage path, which provides stable, human-readable identifiers that persist across sessions.

### Git Storage Structure

Subsurface stores dives in a git repository with the following hierarchy:

```
{year}/{month}/{trip-directory}/{dive-directory}/Dive-{number}
```

**Example:**
```
2026/01/09-Kona/11-Sun-09=51=53/Dive-731
│    │  │       │                │
│    │  │       │                └─ Dive file (dive number 731)
│    │  │       └─ Dive directory: day-weekday-time
│    │  └─ Trip directory: day-TripName (trip started on 9th)
│    └─ Month
└─ Year
```

**Format details:**
- Time uses `=` as separator instead of `:` (Windows compatibility, per existing git storage)
- Weekday is 3-letter English abbreviation (Sun, Mon, Tue, Wed, Thu, Fri, Sat)
- Trip names are sanitized (letters only, max 15 chars) with uniqueness suffix if needed

### Identifier Formats

**Trip ID:** `{year}/{month}/{day}-{TripName}`
- Example: `2026/01/09-Kona`
- URL-encoded for use in URLs: `2026%2F01%2F09-Kona`

**Dive ID:** `{year}/{month}/{dive-directory}` (the timestamp portion)
- Example: `2026/01/11-Sun-09=51=53`
- This uniquely identifies a dive by its exact start time
- The dive number is metadata, not part of the identifier

### URL Structure

```
/trip/2026/01/09-Kona                    # Trip detail page
/dive/2026/01/11-Sun-09=51=53            # Dive detail page
/api/trip/2026/01/09-Kona                # Trip JSON API
/api/dive/2026/01/11-Sun-09=51=53        # Dive JSON API
/api/profile/2026/01/11-Sun-09=51=53/0   # Profile image (dc_index=0)
```

### CLI Tool Reference Format

The CLI tool accepts dive and trip references using the `--dive-ref` and `--trip-ref` parameters:

```bash
subsurface-cli get-dive --dive-ref="2026/01/11-Sun-09=51=53"
subsurface-cli get-trip --trip-ref="2026/01/09-Kona"
subsurface-cli get-profile --dive-ref="2026/01/11-Sun-09=51=53" --dc-index=0
```

### Implementation Notes

The CLI tool resolves these path-based references to internal dive structures using the same parsing logic as `load-git.cpp`:
- Parse year, month, day from the path components
- Parse hour, minute, second from the time portion (after the weekday)
- Construct timestamp via `utc_mktime()`
- Look up dive by timestamp match

This approach ensures:
- **Stability:** IDs don't change unless the dive's start time changes
- **Human readability:** Users can understand what dive they're looking at from the URL
- **Bookmarkability:** URLs can be saved and shared
- **Git alignment:** Uses the same structure as the underlying storage

---

## Component 1: CLI Tool (`subsurface-cli`)

### Purpose
A C++ command-line tool that exposes Subsurface's core functionality via a JSON-based interface. Takes commands via arguments, returns results via JSON to stdout, with optional binary files (PNGs) written to temp locations.

### Configuration
The tool reads configuration from a file (not command-line args for security):

**Config file location:**
- Local use: `~/.config/subsurface-cli/config.json` (or platform equivalent)
- Multi-tenant: Per-request temporary file created by Flask, path specified via `--config` flag

**Config file format:**
```json
{
  "repo_path": "/path/to/git/repo/clone",
  "temp_dir": "/tmp/subsurface-cli",
  "cache_ttl_seconds": 300,
  "userid": "user@example.com"
}
```

**Field descriptions:**
- `repo_path`: Path to the user's git repository (required)
- `temp_dir`: Directory for temporary files like profile PNGs (default: system temp)
- `cache_ttl_seconds`: How long to cache git sync status (default: 300)
- `userid`: User identifier for logging purposes (optional, used in multi-tenant mode)

### Commands

#### 1. `list-dives` - Get dive list with pagination

**Purpose:** Return overview statistics plus a paginated list of top-level items (dives or trips).

**Usage:**
```bash
subsurface-cli --config=/path/to/config.json list-dives \
  --start=0 \
  --count=50
```

**Arguments:**
- `--start`: Zero-based index into the top-level list (default: 0)
- `--count`: Number of top-level items to return (default: 50, max: 500)

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "total_dives": 1247,
  "total_trips": 42,
  "statistics": {
    "total_dive_time_minutes": 89450,
    "max_depth_meters": 87.3,
    "avg_depth_meters": 23.4
  },
  "items": [
    {
      "type": "trip",
      "trip_ref": "2024/06/15-Bonaire",
      "description": "Bonaire 2024",
      "date_start": "2024-06-15",
      "date_end": "2024-06-22",
      "location": "Bonaire",
      "dive_count": 18
    },
    {
      "type": "dive",
      "dive_ref": "2024/06/10-Mon-14=30=00",
      "dive_number": 1247,
      "date": "2024-06-10",
      "time": "14:30:00",
      "location": "Blue Heron Bridge",
      "duration_minutes": 67,
      "max_depth_meters": 12.4,
      "buddy": "Jane Smith",
      "gases": ["Air"],
      "sac_rate": 15.2,
      "dc_count": 2
    }
  ]
}
```

**Error handling:**
```json
{
  "status": "error",
  "error_code": "AUTH_FAILED",
  "message": "Authentication token invalid or expired"
}
```

**Error codes:**
- `AUTH_FAILED`: Authentication/authorization failed
- `REPO_NOT_FOUND`: Git repository not found at specified path
- `INVALID_RANGE`: Start/count parameters out of bounds
- `SYNC_ERROR`: Failed to sync with cloud storage

---

#### 2. `get-trip` - Get trip details with dive list

**Purpose:** Return detailed information about a specific trip including all its dives.

**Usage:**
```bash
subsurface-cli --config=/path/to/config.json get-trip \
  --trip-ref="2024/06/15-Bonaire" \
  --dive-start=0 \
  --dive-count=20
```

**Arguments:**
- `--trip-ref`: Trip reference path (required), e.g., "2024/06/15-Bonaire"
- `--dive-start`: Zero-based index into the trip's dive list (default: 0)
- `--dive-count`: Number of dives to return (default: 100, max: 500)

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "trip": {
    "trip_ref": "2024/06/15-Bonaire",
    "description": "Bonaire 2024",
    "date_start": "2024-06-15",
    "date_end": "2024-06-22",
    "location": "Bonaire",
    "notes": "Amazing shore diving...",
    "dive_count": 18
  },
  "dives": [
    {
      "dive_ref": "2024/06/15-Sat-10=30=00",
      "dive_number": 1247,
      "date": "2024-06-15",
      "time": "10:30:00",
      "location": "1000 Steps",
      "duration_minutes": 67,
      "max_depth_meters": 31.2,
      "buddy": "Jane Smith",
      "gases": ["EAN32"],
      "sac_rate": 14.8,
      "dc_count": 2
    }
  ]
}
```

---

#### 3. `get-dive` - Get detailed dive information

**Purpose:** Return comprehensive information about a specific dive, including all metadata but NOT the profile images (those are requested separately).

**Usage:**
```bash
subsurface-cli --config=/path/to/config.json get-dive \
  --dive-ref="2024/06/15-Sat-10=30=00"
```

**Arguments:**
- `--dive-ref`: Dive reference path (required), e.g., "2024/06/15-Sat-10=30=00"

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "dive": {
    "dive_ref": "2024/06/15-Sat-10=30=00",
    "dive_number": 1247,
    "date": "2024-06-15",
    "time": "10:30:00",
    "duration_minutes": 67,
    "max_depth_meters": 31.2,
    "avg_depth_meters": 18.7,
    "trip_ref": "2024/06/15-Bonaire",
    "location": {
      "name": "1000 Steps",
      "gps": "12.1234,-68.2345"
    },
    "buddy": "Jane Smith",
    "diveguide": "Local Guide",
    "suit": "3mm wetsuit",
    "tags": ["shore", "reef"],
    "rating": 4,
    "visibility": 4,
    "air_temp_celsius": 28,
    "water_temp_celsius": 27,
    "gases": [
      {
        "name": "EAN32",
        "o2_percent": 32,
        "he_percent": 0
      }
    ],
    "cylinders": [
      {
        "description": "AL80",
        "start_pressure_bar": 200,
        "end_pressure_bar": 50,
        "gas_name": "EAN32"
      }
    ],
    "weights": [
      {
        "description": "integrated",
        "weight_kg": 4
      }
    ],
    "sac_rate": 14.8,
    "notes": "Beautiful reef dive...",
    "dive_computers": [
      {
        "dc_index": 0,
        "model": "Shearwater Perdix",
        "device_id": "12345678",
        "sample_count": 2010
      },
      {
        "dc_index": 1,
        "model": "Suunto D5",
        "device_id": "87654321",
        "sample_count": 2015
      }
    ]
  }
}
```

---

#### 4. `get-profile` - Get dive profile PNG

**Purpose:** Generate and return a profile image for a specific dive computer.

**Usage:**
```bash
subsurface-cli --config=/path/to/config.json get-profile \
  --dive-ref="2024/06/15-Sat-10=30=00" \
  --dc-index=0 \
  --width=1024 \
  --height=768
```

**Arguments:**
- `--dive-ref`: Dive reference path (required)
- `--dc-index`: Dive computer index (default: 0)
- `--width`: Image width in pixels (default: 1024, max: 4096)
- `--height`: Image height in pixels (default: 768, max: 4096)

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "profile": {
    "dive_ref": "2024/06/15-Sat-10=30=00",
    "dc_index": 0,
    "dc_model": "Shearwater Perdix",
    "width": 1024,
    "height": 768,
    "file_path": "/tmp/subsurface-cli/profile_20240615_103000_0_a3f2b9.png",
    "file_size_bytes": 145234
  }
}
```

**Notes:**
- PNG file is written to temp directory specified in config
- Filename includes date, time, dc_index, and random hash to avoid collisions
- Flask app is responsible for serving the file and cleaning it up

---

#### 5. `get-stats` - Get overall statistics

**Purpose:** Return aggregate statistics across all dives.

**Usage:**
```bash
subsurface-cli --config=/path/to/config.json get-stats
```

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "statistics": {
    "total_dives": 1247,
    "total_dive_time_minutes": 89450,
    "total_dive_time_human": "62 days 2 hours",
    "max_depth_meters": 87.3,
    "avg_depth_meters": 23.4,
    "max_duration_minutes": 187,
    "avg_duration_minutes": 71.7,
    "date_first_dive": "2010-03-15",
    "date_last_dive": "2024-12-20",
    "total_trips": 42,
    "countries_visited": 23,
    "dive_sites_visited": 387
  }
}
```

---

### Authentication & Authorization

Authentication is handled differently depending on deployment mode:

#### Local Single-User Mode

No authentication is required. The CLI tool trusts the config file, which points to the user's local git repository. Security relies on file system permissions.

#### Multi-Tenant Cloud Mode

Authentication is handled by the existing `flask-auth-proxy` application, which runs alongside this new Flask app. The flow is:

```
Internet (HTTPS)
    ↓
Apache :443 (SSL termination)
    ↓
flask-auth-proxy :5000 (handles login, validates credentials against MySQL)
    ↓
Apache proxies authenticated requests with X-Authenticated-User header
    ↓
New Flask WebUI App :5001
    ↓
subsurface-cli (invoked with per-user config)
    ↓
Git Repo at /var/www/git/{username}/
```

**Key points:**
1. The existing `flask-auth-proxy` handles all login/logout and session management
2. Apache adds an `X-Authenticated-User` header to authenticated requests
3. The new Flask app reads this header to identify the user
4. The Flask app derives the git repo path: `/var/www/git/{username}/`
5. The Flask app creates a per-request config file for the CLI tool
6. The CLI tool does NOT validate tokens - it trusts the config file
7. Security is enforced by the fact that only the Flask app (running locally) can invoke the CLI

**Flask app authentication handling:**
```python
from flask import request, g, abort
import tempfile
import json

@app.before_request
def authenticate():
    # Read user identity from Apache proxy header
    username = request.headers.get('X-Authenticated-User')
    if not username:
        abort(401, "Authentication required")

    g.username = username
    g.repo_path = f"/var/www/git/{username}/"

    # Create per-request config for CLI tool
    g.cli_config = create_cli_config(g.username, g.repo_path)

def create_cli_config(username: str, repo_path: str) -> str:
    """Create a temporary config file for this request."""
    config = {
        "repo_path": repo_path,
        "temp_dir": "/tmp/subsurface-cli",
        "cache_ttl_seconds": 300,
        "userid": username
    }

    # Write to temp file (cleaned up after request)
    fd, path = tempfile.mkstemp(suffix='.json', prefix='subsurface-cli-')
    with os.fdopen(fd, 'w') as f:
        json.dump(config, f)

    return path
```

**Security considerations:**
- The CLI tool is only invoked by the Flask app, never directly by users
- The Flask app runs on localhost, not exposed to the internet
- Apache ensures all requests have valid authentication before proxying
- The `X-Authenticated-User` header can only be set by Apache (stripped from client requests)
- Each user can only access their own git repository at `/var/www/git/{username}/`
- Path traversal is prevented by the Flask app constructing the repo path, not accepting it from users

---

### Data Synchronization

The CLI tool handles syncing with cloud storage using a **cached TTL strategy**:

1. **On each invocation:** Check if local repo needs sync based on last sync time
2. **Sync decision:**
   - Read last sync timestamp from cache file (`{repo_path}/.subsurface-cli-sync`)
   - If last sync was within `cache_ttl_seconds` (default: 300 = 5 minutes), skip sync
   - Otherwise, perform git pull from cloud storage
3. **Sync implementation:**
   - Use existing C++ cloud sync code from `git-access.cpp`
   - Handles authentication, merge conflicts, etc.
4. **Error handling:**
   - If sync fails, return error to Flask app
   - Do not serve stale data on sync failure

**Manual refresh:** The Flask app provides an `/api/refresh` endpoint that forces a sync regardless of TTL:
```python
@app.route('/api/refresh', methods=['POST'])
def force_refresh():
    """Force a git sync, bypassing the cache TTL."""
    # Delete the sync cache file to force refresh
    sync_file = os.path.join(g.repo_path, '.subsurface-cli-sync')
    if os.path.exists(sync_file):
        os.unlink(sync_file)

    # Call any CLI command to trigger sync
    return g.cli.get_stats()
```

**Cache file format** (`{repo_path}/.subsurface-cli-sync`):
```json
{
  "last_sync": "2024-01-15T10:30:00Z",
  "last_commit": "a3f2b9c1d4e5f6..."
}
```

---

### Implementation Notes

**Existing C++ code to leverage:**
- Cloud storage sync/pull: Already exists, needs minimal wrapper
- PNG generation: `void exportProfile(ProfileScene &profile, const struct dive &dive, const QString &filename, bool diveinfo)` is a static function in backend-shared/exportfuncs.cpp -- this may need some modification as it is print focused, but most of what we need is there
- Data structures: Use existing dive/trip structures, just serialize to JSON
- Filtering: Exists but not needed for v1 (just use start/count pagination)

**New C++ code needed:**
1. **CLI argument parsing:** Use Qt's QCommandLineParser or similar
2. **JSON serialization:** Convert C++ dive/trip structures to JSON
   - potentially use Qt's QJsonDocument/QJsonObject
3. **Pagination logic:** Extract subset of dive list based on start/count
4. **Config file handling:** Read JSON config file
5. **Token validation:** Read and validate token file
6. **Main function:** Route commands to appropriate handlers

**Error handling:**
- All errors output JSON with `status: "error"`
- Use appropriate exit codes (0 = success, non-zero = error)
- Log errors to stderr (Flask can capture and log these)

---

## Component 2: Flask Web Application

### Purpose
Provide the HTTP interface for the web UI. Handle user sessions, route requests, call the CLI tool, serve results, and manage temporary files.

### Technology Stack
- Python 3.10+
- Flask 3.x
- Flask-CORS (for API endpoints if needed)
- Gunicorn or similar WSGI server for production

### Architecture

```
Flask App Structure:
├── app.py                 # Main Flask application
├── config.py              # Configuration management
├── auth.py                # Authentication/session handling
├── subsurface_cli.py      # Wrapper for calling CLI tool
├── routes/
│   ├── __init__.py
│   ├── dive_list.py       # Route handlers for dive list
│   ├── dive_detail.py     # Route handlers for dive details
│   └── api.py             # JSON API endpoints
├── templates/
│   ├── base.html          # Base template
│   ├── dive_list.html     # Dive list view
│   └── dive_detail.html   # Dive detail view
├── static/
│   ├── css/
│   ├── js/
│   └── images/
└── requirements.txt
```

### Key Routes

#### Web UI Routes (HTML responses)

```python
GET  /                                      # Redirect to /dives
GET  /dives                                 # Dive list page (paginated)
GET  /dives?start=0&count=50                # Dive list with pagination
GET  /trip/<path:trip_ref>                  # Trip detail page
GET  /dive/<path:dive_ref>                  # Dive detail page

# Examples:
# /trip/2024/06/15-Bonaire
# /dive/2024/06/15-Sat-10=30=00
```

#### API Routes (JSON responses)

```python
GET  /api/dives                             # Get dive list (JSON)
GET  /api/trip/<path:trip_ref>              # Get trip details (JSON)
GET  /api/dive/<path:dive_ref>              # Get dive details (JSON)
GET  /api/profile/<path:dive_ref>/<int:dc>  # Get profile image
GET  /api/stats                             # Get statistics (JSON)
POST /api/refresh                           # Force git sync

# Examples:
# /api/trip/2024/06/15-Bonaire
# /api/dive/2024/06/15-Sat-10=30=00
# /api/profile/2024/06/15-Sat-10=30=00/0
```

### CLI Tool Wrapper (`subsurface_cli.py`)

**Purpose:** Encapsulate all CLI tool interactions in a single Python module.

```python
import subprocess
import json
import tempfile
from pathlib import Path
from typing import Dict, Any, Optional

class SubsurfaceWebCLI:
    def __init__(self, config_path: str, cli_binary: str = "subsurface-cli"):
        self.config_path = config_path
        self.cli_binary = cli_binary

    def _run_command(self, command: str, args: Dict[str, Any]) -> Dict[str, Any]:
        """
        Execute CLI command and return parsed JSON result.

        Raises:
            SubsurfaceCLIError: If command fails or returns error status
        """
        cmd = [self.cli_binary, f"--config={self.config_path}", command]

        # Add arguments
        for key, value in args.items():
            cmd.append(f"--{key}={value}")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
                check=False
            )

            if result.returncode != 0:
                raise SubsurfaceCLIError(
                    f"CLI command failed: {result.stderr}"
                )

            data = json.loads(result.stdout)

            if data.get("status") == "error":
                raise SubsurfaceCLIError(
                    data.get("message", "Unknown error"),
                    error_code=data.get("error_code")
                )

            return data

        except subprocess.TimeoutExpired:
            raise SubsurfaceCLIError("CLI command timed out")
        except json.JSONDecodeError as e:
            raise SubsurfaceCLIError(f"Invalid JSON from CLI: {e}")

    def list_dives(self, start: int = 0, count: int = 50) -> Dict[str, Any]:
        """Get paginated dive list."""
        return self._run_command("list-dives", {
            "start": start,
            "count": count
        })

    def get_trip(self, trip_ref: str, dive_start: int = 0,
                 dive_count: int = 100) -> Dict[str, Any]:
        """Get trip details with dive list."""
        return self._run_command("get-trip", {
            "trip-ref": trip_ref,
            "dive-start": dive_start,
            "dive-count": dive_count
        })

    def get_dive(self, dive_ref: str) -> Dict[str, Any]:
        """Get detailed dive information."""
        return self._run_command("get-dive", {
            "dive-ref": dive_ref
        })

    def get_profile(self, dive_ref: str, dc_index: int = 0,
                    width: int = 1024, height: int = 768) -> Dict[str, Any]:
        """Generate dive profile PNG."""
        return self._run_command("get-profile", {
            "dive-ref": dive_ref,
            "dc-index": dc_index,
            "width": width,
            "height": height
        })

    def get_stats(self) -> Dict[str, Any]:
        """Get overall statistics."""
        return self._run_command("get-stats", {})


class SubsurfaceCLIError(Exception):
    """Exception raised for CLI tool errors."""
    def __init__(self, message: str, error_code: Optional[str] = None):
        self.message = message
        self.error_code = error_code
        super().__init__(self.message)
```

### Authentication/Session Handling

**Two deployment modes:**

#### 1. Single-User Mode (local deployment)
- No authentication needed
- Config file points to user's local git repo, relying on file system permissions for control
- Flask runs on localhost only

#### 2. Multi-Tenant Mode (cloud server)
- External authentication already handled by existing cloud web frontend
- User session includes:
  - User ID
  - Token for accessing their git repo
  - Repo path
- Flask receives authenticated requests (via reverse proxy or similar)
- Session management:
  ```python
  from flask import session

  @app.before_request
  def ensure_authenticated():
      if not session.get('user_id'):
          return redirect('/login')  # Or return 401 for API

      # Create per-user CLI instance
      config = create_user_config(
          user_id=session['user_id'],
          token=session['token'],
          repo_path=session['repo_path']
      )
      g.cli = SubsurfaceWebCLI(config_path=config)
  ```

### Route Handler Example

```python
from flask import Flask, render_template, request, jsonify, g, send_file
from pathlib import Path
from subsurface_cli import SubsurfaceWebCLI, SubsurfaceCLIError

@app.route('/dives')
def dive_list():
    """Render dive list page."""
    start = request.args.get('start', 0, type=int)
    count = request.args.get('count', 50, type=int)

    try:
        data = g.cli.list_dives(start=start, count=count)

        return render_template(
            'dive_list.html',
            items=data['items'],
            total_dives=data['total_dives'],
            total_trips=data['total_trips'],
            statistics=data['statistics'],
            start=start,
            count=count
        )

    except SubsurfaceCLIError as e:
        return render_template('error.html', error=str(e)), 500

@app.route('/dive/<path:dive_ref>')
def dive_detail(dive_ref: str):
    """Render dive detail page."""
    try:
        data = g.cli.get_dive(dive_ref=dive_ref)

        return render_template(
            'dive_detail.html',
            dive=data['dive']
        )

    except SubsurfaceCLIError as e:
        return render_template('error.html', error=str(e)), 500

@app.route('/api/profile/<path:dive_ref>/<int:dc_index>')
def api_profile(dive_ref: str, dc_index: int):
    """Return dive profile image."""
    width = request.args.get('width', 1024, type=int)
    height = request.args.get('height', 768, type=int)

    try:
        data = g.cli.get_profile(
            dive_ref=dive_ref,
            dc_index=dc_index,
            width=width,
            height=height
        )

        profile = data['profile']
        file_path = profile['file_path']

        # Serve file and schedule cleanup
        response = send_file(
            file_path,
            mimetype='image/png',
            as_attachment=False,
            download_name=f'profile_{dc_index}.png'
        )

        # Clean up temp file after response is sent
        @response.call_on_close
        def cleanup():
            try:
                Path(file_path).unlink()
            except Exception:
                pass

        return response

    except SubsurfaceCLIError as e:
        return jsonify({'error': str(e)}), 500
```

### Frontend (Responsive Design)

**Technology:**
- Responsive HTML/CSS (Bootstrap 5 or similar)
- Minimal JavaScript (progressive enhancement)
- Mobile-first design

**Key views:**

1. **Dive List View**
   - Collapsible trips
   - Infinite scroll or pagination
   - Quick stats at top
   - Filter/search (future)

2. **Dive Detail View**
   - Profile image(s) with DC switcher
   - All metadata in organized sections
   - Responsive layout (single column on mobile)

3. **Trip Detail View**
   - Trip info at top
   - List of dives in trip

**Profile Image Handling:**
- Request different sizes based on viewport:
  - Mobile: 800x600
  - Tablet: 1024x768
  - Desktop: 1280x960 or 1920x1080
- Use responsive images: `<img srcset="...">`
- Lazy loading for images below fold

---

## Security Considerations

### Path Traversal Prevention
- All repo paths validated against allowed base directory
- Token scoped to specific repo path
- CLI tool validates all file paths before access

### Data Exfiltration Prevention
- CLI tool only returns data for authenticated user's repo
- Token validation ensures user can't access other users' data
- No direct file system access from web layer

### Input Validation
- All user inputs sanitized (dive_id, dc_index, pagination params)
- Size limits on image dimensions
- Rate limiting on API endpoints (future)

### Token Security
- Short-lived tokens (1 hour TTL)
- Stored in secure location with restrictive permissions
- Refreshed automatically by Flask app
- Never exposed in URLs or logs

---

## Deployment Scenarios

### Scenario 1: Local Single-User

**Setup:**
1. User installs `subsurface-cli` CLI tool
2. User installs Flask app (or uses pre-packaged version)
3. User runs: `flask run --host=127.0.0.1 --port=5000`
4. User opens browser to `http://localhost:5000`

**Config:**
- Single config file pointing to user's local git repo
- No authentication needed
- Flask runs in development mode

### Scenario 2: Cloud Multi-Tenant

**Setup:**
1. Cloud server has `subsurface-cli` CLI tool installed
2. Flask app deployed with Gunicorn + Nginx
3. Existing authentication layer (current cloud web frontend) handles auth
4. For each authenticated user:
   - Create/sync their git repo clone
   - Generate time-limited token
   - Create per-user config file
   - Pass user to Flask app via session

**Config:**
- Per-user config files in `/var/subsurface/users/<user_id>/config.json`
- Token files co-located with repos
- Flask runs behind Apache2 reverse proxy
- HTTPS enforced

**Architecture:**
```
Internet
   ↓
Apache2 (SSL termination, reverse proxy) -- this exists and also handles git traffic via https:// and cloud storage user management interface
   ↓
Existing Cloud Frontend (authentication)
   ↓
Flask App (multiple workers)
   ↓
subsurface-cli CLI (one or more processes)
   ↓
Git Repos (/var/www/git/<user_id>/)
```

---

## Implementation Plan

### Phase 1: CLI Tool (C++)
1. Set up CLI argument parsing and config file reading
2. Implement token validation and repo path security
3. Implement `list-dives` command with JSON output
4. Implement `get-dive` command
5. Implement `get-profile` command (wrap existing PNG generation)
6. Implement `get-trip` command
7. Implement `get-stats` command
8. Add error handling and logging
9. Write unit tests for each command

### Phase 2: Flask App (Python)
1. Set up Flask project structure
2. Implement `subsurface_cli.py` wrapper module
3. Implement basic routes (dive list, dive detail)
4. Create HTML templates with responsive design
5. Implement API routes
6. Add session management for multi-tenant mode
7. Add error handling and logging
8. Test with CLI tool

### Phase 3: Integration & Testing
1. End-to-end testing with real dive data
2. Performance testing with large dive sets (5000+ dives)
3. Security testing (path traversal, token validation)
4. Multi-tenant testing
5. Browser compatibility testing
6. Mobile responsiveness testing

### Phase 4: Deployment
1. Create installation documentation
2. Package for single-user deployment
3. Deploy to cloud server for multi-tenant
4. Monitor and tune performance

---

## Future Enhancements (Out of Scope for v1)

- Write operations (edit dives, add notes, etc.)
- Advanced filtering and search
- Export functionality (PDF, CSV)
- Social features (share dives, compare with buddies)
- Real-time sync (WebSocket updates when data changes)
- Progressive Web App (offline support)
- Performance optimizations (process pooling, caching)
- API rate limiting and quotas
- User-configurable themes
- Dive computer direct downloads
- Photos/media display and serving
- Screen-optimized profile rendering (different colors/styling than print)
- Structured file logging with rotation

---

## Open Questions / Decisions Needed

### All Resolved

1. ✅ **CLI tool binary name:** `subsurface-cli`
2. ✅ **Config file location:**
   - Local: Platform-standard location (`~/.config/subsurface-cli/config.json` on Linux)
   - Multi-tenant: Per-request temporary file created by Flask app
3. ✅ **Temp file cleanup strategy:** Immediate after serving (using Flask's `@response.call_on_close`)
4. ✅ **JSON library choice:** Qt's QJsonDocument (already a dependency, no new libs needed)
5. ✅ **Token generation:** Not needed - authentication handled by existing flask-auth-proxy
6. ✅ **Git sync frequency:** Cached with TTL (default 5 minutes) + manual refresh endpoint
7. ✅ **Dive/Trip identification:** Git path-based references (e.g., `2024/06/15-Sat-10=30=00`)
8. ✅ **Authentication approach:** X-Authenticated-User header from Apache/flask-auth-proxy
9. ✅ **Flask app packaging:** Systemd service (matches flask-auth-proxy pattern)
10. ✅ **Flask app location:** In Subsurface repo as `webui/` directory
11. ✅ **Profile rendering:** Use current print rendering for v1; review screen-optimized styling for v2
12. ✅ **Photos/media:** Defer to v2 (no photo references in v1)
13. ✅ **Error logging:** stderr only (Flask captures); structured file logging can be added in v2

---

## Success Criteria

1. ✅ Web UI can display dive list for users with 5000+ dives without performance issues
2. ✅ Profile images load quickly (<2 seconds on typical connection)
3. ✅ Data transfer per page view is <10 MB (vs. hundreds of MB with current HTML export)
4. ✅ CLI tool reuses existing C++ code without major refactoring
5. ✅ Multi-tenant deployment works with isolated user data
6. ✅ Security: no path traversal, data exfiltration, or unauthorized access possible
7. ✅ Mobile-friendly responsive design works on phones, tablets, desktops
8. ✅ Single-user deployment is simple (one command to run)

---

## Appendix: Example Data Flow

**User views dive list:**

1. User browses to `https://cloud.subsurface-divelog.org/webui/dives`
2. Apache validates auth via flask-auth-proxy, adds `X-Authenticated-User: user@example.com` header
3. Apache proxies to Flask WebUI app
4. Flask reads header, creates per-request config file with repo path `/var/www/git/user@example.com/`
5. Flask calls: `g.cli.list_dives(start=0, count=50)`
6. CLI wrapper executes: `subsurface-cli --config=/tmp/subsurface-cli-xxx.json list-dives --start=0 --count=50`
7. CLI tool:
   - Reads config file
   - Checks sync cache file for last sync time
   - If cache expired, syncs git repo from cloud storage
   - Loads dive data from local git repo
   - Extracts items 0-49 from top-level list
   - Serializes to JSON with `dive_ref` and `trip_ref` identifiers
   - Outputs to stdout
8. Flask wrapper parses JSON
9. Flask renders HTML template with data
10. User sees dive list in browser

**User clicks on a dive:**

1. User clicks dive "Bonaire - 1000 Steps" from June 15, 2024
2. Browser navigates to `/dive/2024/06/15-Sat-10=30=00`
3. Flask extracts `dive_ref` from URL path
4. Flask calls: `g.cli.get_dive(dive_ref="2024/06/15-Sat-10=30=00")`
5. CLI tool parses dive_ref, looks up dive by timestamp, returns dive metadata JSON
6. Flask renders dive detail template
7. Template includes: `<img src="/api/profile/2024/06/15-Sat-10=30=00/0?width=1024&height=768">`
8. Browser requests profile image
9. Flask calls: `g.cli.get_profile(dive_ref="2024/06/15-Sat-10=30=00", dc_index=0, width=1024, height=768)`
10. CLI tool generates PNG, returns file path in JSON
11. Flask serves PNG file to browser
12. Flask cleans up temp file after response sent

**User switches to dive computer #2:**

1. JavaScript in page changes image src to `/api/profile/2024/06/15-Sat-10=30=00/1?width=1024&height=768`
2. Browser requests new profile (steps 9-12 above)
3. New profile displays

---

## Notes for Claude Code

**Key files to create:**

C++ CLI Tool (in `cli/` directory):
- `cli/main.cpp` - Entry point, command routing
- `cli/commands.cpp` - Command implementations (list-dives, get-dive, get-trip, get-profile, get-stats)
- `cli/json_serializer.cpp` - Convert C++ dive/trip structures to JSON
- `cli/config.cpp` - Config file handling
- `cli/dive_ref.cpp` - Parse dive_ref/trip_ref strings to internal structures
- `cli/CMakeLists.txt` - Build configuration

Python Flask App (in `webui/` directory):
- `webui/app.py` - Main Flask application
- `webui/subsurface_cli.py` - CLI wrapper module
- `webui/routes.py` - Route handlers
- `webui/templates/base.html` - Base template
- `webui/templates/dive_list.html` - Dive list view
- `webui/templates/dive_detail.html` - Dive detail view
- `webui/templates/trip_detail.html` - Trip detail view
- `webui/static/css/style.css` - Responsive CSS
- `webui/static/js/app.js` - Minimal JavaScript (DC switcher, lazy loading)
- `webui/requirements.txt` - Python dependencies
- `webui/config.py` - Flask configuration

**Dependencies:**

C++:
- Qt 6 (QCore, QJson, etc.) - already a dependency; do not bother with supporting Qt 5
- Existing Subsurface core libraries (core/, backend-shared/)

Python:
- Flask >= 3.0
- gunicorn (for production)
- python >= 3.10

**Build/Run Commands:**

C++ CLI Tool:
```bash
cd subsurface
mkdir build && cd build
cmake .. -DSUBSURFACE_TARGET_EXECUTABLE=CliToolExecutable
make subsurface-cli
./subsurface-cli --help
```

Flask App (development):
```bash
cd subsurface/webui
pip install -r requirements.txt
flask run --port 5001
```

Flask App (production):
```bash
cd subsurface/webui
gunicorn -w 4 -b 127.0.0.1:5001 app:app
```

**Testing Strategy:**

1. **Unit Testing**:
   - Use a framework like `unittest` or `pytest` for the Flask application.
   - Ensure all core functionalities, such as token management, user authentication, and API endpoints, are covered.
   - For the CLI tool, use a C++ testing framework like `Catch2` or `Google Test` to validate individual commands and error handling.

2. **Integration Testing**:
   - Test the interaction between the Flask app and the CLI tool.
   - Simulate real-world scenarios, such as token generation and validation workflows.
   - Use tools like `Postman` or `pytest` with `requests` to test API endpoints.

3. **End-to-End Testing**:
   - Automate end-to-end tests using tools like `Selenium` or `Playwright` for the frontend.
   - Simulate user workflows, such as logging in, uploading dive logs, and viewing profiles.
   - Ensure compatibility across major browsers (Chrome, Firefox, Safari, Edge).

4. **Load Testing**:
   - Use tools like `Apache JMeter` or `Locust` to simulate high traffic and measure performance.
   - Focus on critical endpoints, such as token generation, data synchronization, and user authentication.
   - Identify bottlenecks and optimize the application for scalability.

5. **Security Testing**:
   - Perform penetration testing to identify vulnerabilities in the Flask app and CLI tool.
   - Use tools like `OWASP ZAP` or `Burp Suite` to test for common security issues, such as SQL injection and XSS.
   - Ensure secure handling of sensitive data, such as tokens and user credentials.

6. **Regression Testing**:
   - Maintain a suite of regression tests to ensure new changes do not break existing functionality.
   - Automate regression tests using CI/CD pipelines.

7. **Test Coverage**:
   - Aim for at least 80% code coverage for both the Flask app and CLI tool.
   - Use tools like `coverage.py` for Python and `gcov` for C++ to measure coverage.

8. **Implementation Notes**:
   - Set up a dedicated testing environment with mock data for integration and end-to-end tests.
   - Use Docker containers to ensure consistent testing environments across different platforms.
   - Document all test cases and scenarios for future reference.

This comprehensive testing strategy ensures the reliability, security, and scalability of the Subsurface Web UI and CLI tool.

---

# Error Logging Strategy

**Overview**

A robust error logging strategy is essential for diagnosing issues and ensuring the reliability of both the CLI tool and the Flask application. This section outlines the logging mechanisms, storage locations, rotation policies, and monitoring tools for the Subsurface Web UI.

#### CLI Tool Logging

1. **Log Levels**:
   - The CLI tool will support multiple log levels: `INFO`, `WARNING`, `ERROR`, and `DEBUG`.
   - By default, only `WARNING` and `ERROR` messages will be logged in production.

2. **Log Storage**:
   - **Local User Scenario**:
     - Logs will be stored in a platform-appropriate location:
       - **Linux**: `~/.local/share/subsurface-cli/logs/cli.log`
       - **macOS**: `~/Library/Logs/SubsurfaceCLI/cli.log`
       - **Windows**: `%LOCALAPPDATA%\SubsurfaceCLI\logs\cli.log`
     - This ensures logs are stored in a standard location for each platform.
   - **Multi-Tenant Scenario**:
     - Logs will be written to a centralized log file on the server, e.g., `/var/log/subsurface/cli.log`.
     - Each log entry will include a tag identifying the `userid` (email address) of the user for whom the CLI was invoked.

3. **Log Rotation**:
   - Use the standard `logrotate` tool for managing log rotation.
   - Retain logs for 30 days, compressing older logs to save space.

4. **Log Format**:
   - Each log entry will include a timestamp, log level, and message.
   - For multi-tenant scenarios, the `userid` will also be included.
   - Example:
     ```
     [2026-01-23 14:30:00] [ERROR] [User: user@example.com] Failed to validate token: Token expired
     ```

5. **Error Reporting**:
   - Critical errors (e.g., token validation failures, file access issues) will be reported to `stderr` in addition to being logged.

6. **Implementation Notes**:
   - Use a logging library like `boost::log` to handle log formatting and writing.
   - Ensure the log directory is created if it does not exist.

**Updated Implementation Plan**:

1. **Local User Scenario**:
   - Update the CLI tool to detect the platform and write logs to the appropriate directory.
   - Provide a configuration option to override the default log path if needed.

2. **Multi-Tenant Scenario**:
   - Add a `userid` field to the log context for tagging log entries.
   - Ensure the CLI tool writes logs to `/var/log/subsurface/cli.log` on the server.
   - Configure `logrotate` to manage log rotation and retention.

This updated strategy ensures that logs are stored in appropriate locations for both local and multi-tenant scenarios, with clear tagging and rotation policies.

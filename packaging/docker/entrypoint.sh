#!/usr/bin/env bash

set -Eeuo pipefail

export DISPLAY="${DISPLAY:-:99}"
export QT_QUICK_BACKEND="${QT_QUICK_BACKEND:-software}"
export LANG="${LANG:-C.UTF-8}"
export SUBSURFACE_PREFIX="${SUBSURFACE_PREFIX:-/opt/subsurface}"
export QT_PLUGIN_PATH="${SUBSURFACE_PREFIX}/usr/lib/x86_64-linux-gnu/qt6/plugins"

XVFB_PID=""
OPENBOX_PID=""
VNC_PID=""
WEBSOCKIFY_PID=""
APP_PID=""

cleanup() {
  trap - EXIT INT TERM

  local pid
  local -a pids=()
  local -a remaining_pids=()

  # Collect children in reverse startup order.
  for pid in \
    "$APP_PID" \
    "$WEBSOCKIFY_PID" \
    "$VNC_PID" \
    "$OPENBOX_PID" \
    "$XVFB_PID"; do
    if [[ -n "$pid" ]]; then
      pids+=("$pid")
    fi
  done

  # Request graceful shutdown.
  for pid in "${pids[@]}"; do
    kill -TERM "$pid" 2>/dev/null || true
  done

  # Allow children up to five seconds to exit.
  remaining_pids=("${pids[@]}")

  for ((attempt = 0; attempt < 50 && ${#remaining_pids[@]} > 0; attempt++)); do
    local -a still_running=()

    for pid in "${remaining_pids[@]}"; do
      if kill -0 "$pid" 2>/dev/null; then
        still_running+=("$pid")
      fi
    done

    remaining_pids=("${still_running[@]}")

    if ((${#remaining_pids[@]} > 0)); then
      sleep 0.1
    fi
  done

  # Force-stop children that have not exited.
  for pid in "${remaining_pids[@]}"; do
    kill -KILL "$pid" 2>/dev/null || true
  done

  # Reap all children.
  for pid in "${pids[@]}"; do
    wait "$pid" 2>/dev/null || true
  done
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

for command in Xvfb xdpyinfo openbox x11vnc websockify; do
  if ! command -v "$command" >/dev/null 2>&1; then
    echo "Required command not found: $command" >&2
    exit 1
  fi
done

SUBSURFACE="${SUBSURFACE_PREFIX}/bin/subsurface"

if [[ ! -x "$SUBSURFACE" ]]; then
  echo "Subsurface executable not found or not executable: $SUBSURFACE" >&2
  exit 1
fi

Xvfb \
  "$DISPLAY" \
  -screen 0 1920x1080x24 \
  -nolisten tcp &
XVFB_PID=$!

# Wait for the X server to become available.
for ((attempt = 0; attempt < 50; attempt++)); do
  if xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
    break
  fi

  if ! kill -0 "$XVFB_PID" 2>/dev/null; then
    echo "Xvfb exited before becoming ready" >&2
    exit 1
  fi

  sleep 0.2
done

if ! xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
  echo "Xvfb failed to start on $DISPLAY" >&2
  exit 1
fi

openbox &
OPENBOX_PID=$!

x11vnc \
  -display "$DISPLAY" \
  -forever \
  -shared \
  -nopw \
  -listen 127.0.0.1 \
  -rfbport 5900 &
VNC_PID=$!

websockify \
  --web=/usr/share/novnc/ \
  6080 \
  localhost:5900 &
WEBSOCKIFY_PID=$!

# Open the persisted dive log when one is present.
if [[ -f /data/divelog.ssrf ]]; then
  "$SUBSURFACE" /data/divelog.ssrf &
else
  "$SUBSURFACE" &
fi
APP_PID=$!

# Exit when Subsurface exits, or fail if a supporting service exits first.
if wait -n -p finished_pid \
  "$XVFB_PID" \
  "$OPENBOX_PID" \
  "$VNC_PID" \
  "$WEBSOCKIFY_PID" \
  "$APP_PID"; then
  status=0
else
  status=$?
fi

if [[ "${finished_pid:-}" != "$APP_PID" ]]; then
  echo "A required desktop service exited unexpectedly (PID: ${finished_pid:-unknown}, status: $status)" >&2
  exit 1
fi

exit "$status"

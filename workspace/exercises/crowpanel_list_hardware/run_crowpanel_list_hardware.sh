#!/usr/bin/env bash
# Upload, monitor, or sanity-check this sketch (crowpanel_list_hardware).
# Usage: ./run_crowpanel_list_hardware.sh [devices|upload|monitor|check]
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_DIR="$DIR"

usage() {
  cat <<EOF
Usage: $(basename "$0") [devices|upload|monitor|check]

  1) devices  - list connected boards/ports (arduino-cli board list)
  2) upload   - compile the sketch and flash it to the device
  3) monitor  - open a live serial connection (Ctrl+C to exit)
  4) check    - read the serial port for 10s, then stop (quick sanity check)
EOF
}

do_devices() {
  arduino-cli board list
}

# Walk upward from the sketch folder to find config.json, since the
# sketch isn't always a direct child of the project root. Only needed for
# upload/monitor/check -- devices works standalone, so it's dispatched before
# this runs and never fails just because config is missing/stale.
load_config() {
  search="$DIR"
  CONFIG=""
  while [ "$search" != "/" ]; do
    if [ -f "$search/config.json" ]; then
      CONFIG="$search/config.json"
      break
    fi
    search="$(dirname "$search")"
  done
  if [ -z "$CONFIG" ]; then
    echo "Could not find config.json in any parent of $DIR" >&2
    exit 1
  fi

  # Re-read every run so a port/fqbn change picked up later doesn't go stale.
  FQBN=$(sed -n 's/.*"fqbn"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$CONFIG")
  PORT=$(sed -n 's/.*"port"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$CONFIG")
  MONITOR_BAUD=$(sed -n 's/.*"monitorBaud"[[:space:]]*:[[:space:]]*\([0-9]*\).*/\1/p' "$CONFIG")
}

do_upload() {
  echo "--- compiling $(basename "$SKETCH_DIR") ---"
  arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR"

  echo "--- uploading to $PORT ---"
  arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SKETCH_DIR"
}

do_monitor() {
  echo "--- opening live serial monitor on $PORT (Ctrl+C to exit) ---"
  # arduino-cli monitor produced garbled output against this board no matter
  # what flags were tried; plain stty+cat was reliably clean, so use that
  # instead of the generic arduino-cli monitor.
  stty -F "$PORT" "$MONITOR_BAUD" raw -echo -hupcl
  cat "$PORT"
}

do_check() {
  echo "--- reading $PORT for 10s ---"
  stty -F "$PORT" "$MONITOR_BAUD" raw -echo -hupcl
  timeout 10 cat "$PORT"
}

ACTION="${1:-}"

if [ -z "$ACTION" ]; then
  usage
  read -rp "Choice [1-4]: " CHOICE
  case "$CHOICE" in
    1) ACTION="devices" ;;
    2) ACTION="upload" ;;
    3) ACTION="monitor" ;;
    4) ACTION="check" ;;
    *) echo "Unrecognized choice: $CHOICE" >&2; exit 1 ;;
  esac
fi

case "$ACTION" in
  devices) do_devices ;;
  upload) load_config; do_upload ;;
  monitor) load_config; do_monitor ;;
  check) load_config; do_check ;;
  -h|--help) usage ;;
  *) usage; exit 1 ;;
esac

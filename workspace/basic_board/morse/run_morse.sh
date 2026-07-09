#!/usr/bin/env bash
# Upload, monitor, or sanity-check this sketch (morse).
# Usage: ./run_morse.sh [upload|monitor|check]
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_DIR="$DIR"

# Walk upward from the sketch folder to find config.json (normally the one
# right here in this folder), so a shared parent-level config also works.
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
  echo "Could not find config.json in $DIR or any parent" >&2
  exit 1
fi

# Re-read every run so a port/fqbn change picked up later doesn't go stale.
FQBN=$(sed -n 's/.*"fqbn"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$CONFIG")
PORT=$(sed -n 's/.*"port"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$CONFIG")
MONITOR_BAUD=$(sed -n 's/.*"monitorBaud"[[:space:]]*:[[:space:]]*\([0-9]*\).*/\1/p' "$CONFIG")

usage() {
  cat <<EOF
Usage: $(basename "$0") [upload|monitor|check]

  1) upload   - compile the sketch and flash it to the device
  2) monitor  - open a live serial connection (Ctrl+C to exit)
  3) check    - read the serial port for 10s, then stop (quick sanity check)
EOF
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
  read -rp "Choice [1-3]: " CHOICE
  case "$CHOICE" in
    1) ACTION="upload" ;;
    2) ACTION="monitor" ;;
    3) ACTION="check" ;;
    *) echo "Unrecognized choice: $CHOICE" >&2; exit 1 ;;
  esac
fi

case "$ACTION" in
  upload) do_upload ;;
  monitor) do_monitor ;;
  check) do_check ;;
  -h|--help) usage ;;
  *) usage; exit 1 ;;
esac

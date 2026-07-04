#!/usr/bin/env bash
# Compiles and uploads this folder's sketch to the board over USB.
# Usage: ./install.sh [port]   (defaults to /dev/ttyUSB0)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
PORT="${1:-/dev/ttyUSB0}"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found on PATH -- see docs/dev-tools.md's arduino-cli section to install it." >&2
  exit 1
fi

FQBN="$(cat "$REPO_ROOT/docs/fqbn.txt")"

echo "--- compiling $(basename "$SCRIPT_DIR") ---"
arduino-cli compile --fqbn "$FQBN" "$SCRIPT_DIR"

echo "--- uploading to $PORT ---"
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SCRIPT_DIR"

echo "--- done -- confirm with: arduino-cli monitor -p $PORT -c baudrate=115200 ---"

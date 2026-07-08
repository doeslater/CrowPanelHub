#!/usr/bin/env bash
# Compiles and uploads this folder's sketch to the board over USB.
# Usage: ./install.sh [port]   (defaults to /dev/ttyUSB0 -- the CrowPanel,
# since this exercise draws to the real e-paper panel)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-/dev/ttyUSB0}"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found on PATH -- see docs/dev-tools.md's arduino-cli section to install it." >&2
  exit 1
fi

# Same board for every sketch in this repo. Hardcoded here (not read from a
# shared file) so this script still works if this folder is ever copied out
# on its own, without the rest of the repo alongside it -- see docs/fqbn.txt
# for the same value kept for manual/documentation use.
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,PartitionScheme=default"

echo "--- compiling $(basename "$SCRIPT_DIR") ---"
arduino-cli compile --fqbn "$FQBN" "$SCRIPT_DIR"

echo "--- uploading to $PORT ---"
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SCRIPT_DIR"

echo "--- done -- confirm with: arduino-cli monitor -p $PORT -c baudrate=115200 ---"

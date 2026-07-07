#!/usr/bin/env bash
# Compiles and uploads this folder's sketch to the board over USB.
# Usage: ./install.sh [port]   (defaults to /dev/ttyACM0)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-/dev/ttyACM0}"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found on PATH -- see docs/dev-tools.md's arduino-cli section to install it." >&2
  exit 1
fi

# Generic "ESP32S3 Dev Module" FQBN -- same board family as the CrowPanel's
# ESP32-S3-WROOM-1 (this board is a N16R8/N8R2 variant), so the same option
# values apply (8MB embedded PSRAM confirmed via esptool chip-id).
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,PartitionScheme=default"

echo "--- compiling $(basename "$SCRIPT_DIR") ---"
arduino-cli compile --fqbn "$FQBN" "$SCRIPT_DIR"

echo "--- uploading to $PORT ---"
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SCRIPT_DIR"

echo "--- done -- confirm with: arduino-cli monitor -p $PORT -c baudrate=115200 ---"

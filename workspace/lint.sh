#!/usr/bin/env bash
# Compiles exercises/pwr_pin and side-track/rgb_led with extra compiler
# warnings enabled (the closest thing to a "linter" available here -- no
# standalone C/C++ linter is installed on this machine), saving output to
# lint.log in the repo root instead of printing to the terminal.
# Usage: ./lint.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LOG_FILE="$SCRIPT_DIR/lint.log"
# Hardcoded here (not read from ../docs/fqbn.txt) so this script keeps
# working regardless of where workspace/ sits relative to the repo root --
# same self-contained convention as every install.sh in this repo.
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,PartitionScheme=default"

{
  echo "--- exercises/pwr_pin ---"
  arduino-cli compile --warnings all --fqbn "$FQBN" "$SCRIPT_DIR/exercises/pwr_pin/"
  echo
  echo "--- side-track/rgb_led ---"
  arduino-cli compile --warnings all --fqbn "$FQBN" "$SCRIPT_DIR/side-track/rgb_led/"
} > "$LOG_FILE" 2>&1

echo "done -- see $LOG_FILE"

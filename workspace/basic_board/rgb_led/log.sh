#!/usr/bin/env bash
# Runs install.sh and saves everything it prints to install.log in this folder.
# Usage: ./log.sh [port]   (defaults to /dev/ttyACM0)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

"$SCRIPT_DIR/install.sh" "$@" > "$SCRIPT_DIR/install.log" 2>&1
echo "done -- see $SCRIPT_DIR/install.log"

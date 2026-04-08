#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

find "$SCRIPT_DIR" -maxdepth 1 -type f -name '*.data' -delete

echo "playground reset complete"

#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
MEMBER_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
ROOT_DIR=$(CDPATH= cd -- "$MEMBER_DIR/.." && pwd)
PROCESSOR="$MEMBER_DIR/src/sql_processor"

usage() {
    echo "usage: ./run_suite.sh <public|hidden>"
}

if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

SUITE=$1

case "$SUITE" in
    public|hidden)
        ;;
    *)
        usage
        exit 1
        ;;
esac

if [ ! -x "$PROCESSOR" ]; then
    echo "sql_processor not found. building it first..."
    (cd "$MEMBER_DIR/src" && make)
fi

(cd "$ROOT_DIR" && ./common/scripts/run_tests.sh ./member-dragon/src/sql_processor "$SUITE")

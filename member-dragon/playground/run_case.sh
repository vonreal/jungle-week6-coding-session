#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
MEMBER_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
ROOT_DIR=$(CDPATH= cd -- "$MEMBER_DIR/.." && pwd)
TMP_DIR="$SCRIPT_DIR/.tmp"
PROCESSOR="$MEMBER_DIR/src/sql_processor"

usage() {
    echo "usage: ./run_case.sh <public|hidden> <case_name>"
    echo "example: ./run_case.sh public 01_basic"
}

if [ "$#" -ne 2 ]; then
    usage
    exit 1
fi

SUITE=$1
CASE_NAME=${2%.sql}

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

SQL_FILE="$ROOT_DIR/common/tests/$SUITE/$CASE_NAME.sql"
EXPECTED_STDOUT="$ROOT_DIR/common/tests/$SUITE/$CASE_NAME.expected"
EXPECTED_STDERR="$ROOT_DIR/common/tests/$SUITE/$CASE_NAME.expected_err"

if [ ! -f "$SQL_FILE" ]; then
    echo "test case not found: $SQL_FILE"
    exit 1
fi

mkdir -p "$TMP_DIR"
STDOUT_FILE="$TMP_DIR/$SUITE-$CASE_NAME.stdout"
STDERR_FILE="$TMP_DIR/$SUITE-$CASE_NAME.stderr"
STDOUT_DIFF="$TMP_DIR/$SUITE-$CASE_NAME.stdout.diff"
STDERR_DIFF="$TMP_DIR/$SUITE-$CASE_NAME.stderr.diff"

"$SCRIPT_DIR/reset.sh" >/dev/null

STATUS=0
(cd "$SCRIPT_DIR" && "$PROCESSOR" "$SQL_FILE") >"$STDOUT_FILE" 2>"$STDERR_FILE" || STATUS=$?

STDOUT_OK=1
STDERR_OK=1

rm -f "$STDOUT_DIFF" "$STDERR_DIFF"

if [ -f "$EXPECTED_STDOUT" ]; then
    if diff -u "$EXPECTED_STDOUT" "$STDOUT_FILE" >"$STDOUT_DIFF"; then
        STDOUT_OK=0
        rm -f "$STDOUT_DIFF"
    fi
else
    if [ ! -s "$STDOUT_FILE" ]; then
        STDOUT_OK=0
    fi
fi

if [ -f "$EXPECTED_STDERR" ]; then
    if diff -u "$EXPECTED_STDERR" "$STDERR_FILE" >"$STDERR_DIFF"; then
        STDERR_OK=0
        rm -f "$STDERR_DIFF"
    fi
else
    if [ ! -s "$STDERR_FILE" ]; then
        STDERR_OK=0
    fi
fi

echo "suite: $SUITE"
echo "case:  $CASE_NAME"
echo "exit:  $STATUS"
echo "stdout: $STDOUT_FILE"
echo "stderr: $STDERR_FILE"

if [ "$STDOUT_OK" -eq 0 ] && [ "$STDERR_OK" -eq 0 ]; then
    echo "result: PASS"
    exit 0
fi

echo "result: FAIL"

if [ "$STDOUT_OK" -ne 0 ]; then
    echo ""
    echo "[stdout diff]"
    if [ -f "$STDOUT_DIFF" ]; then
        sed -n '1,80p' "$STDOUT_DIFF"
    else
        echo "expected empty stdout"
        sed -n '1,40p' "$STDOUT_FILE"
    fi
fi

if [ "$STDERR_OK" -ne 0 ]; then
    echo ""
    echo "[stderr diff]"
    if [ -f "$STDERR_DIFF" ]; then
        sed -n '1,80p' "$STDERR_DIFF"
    else
        echo "expected empty stderr"
        sed -n '1,40p' "$STDERR_FILE"
    fi
fi

exit 1

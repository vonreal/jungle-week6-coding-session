#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <sql_processor_path>"
  exit 1
fi

BIN_INPUT="$1"
BIN="$(cd "$(dirname "$BIN_INPUT")" && pwd)/$(basename "$BIN_INPUT")"

if [ ! -x "$BIN" ]; then
  echo "[ERROR] executable not found or not executable: $BIN"
  exit 1
fi

TOTAL=0
PASS=0
FAIL=0

run_case() {
  local name="$1"
  shift

  TOTAL=$((TOTAL + 1))
  if "$@"; then
    echo "[PASS] $name"
    PASS=$((PASS + 1))
  else
    echo "[FAIL] $name"
    FAIL=$((FAIL + 1))
  fi
}

write_required_schemas() {
  local dir="$1"

  cat >"$dir/users.schema" <<'SCHEMA_USERS'
{
  "table": "users",
  "columns": [
    { "name": "name", "type": "string" },
    { "name": "age", "type": "int" },
    { "name": "major", "type": "string" }
  ]
}
SCHEMA_USERS

  cat >"$dir/products.schema" <<'SCHEMA_PRODUCTS'
{
  "table": "products",
  "columns": [
    { "name": "name", "type": "string" },
    { "name": "price", "type": "int" },
    { "name": "category", "type": "string" }
  ]
}
SCHEMA_PRODUCTS
}

case_logging_creates_log_file_for_successful_run() {
  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' RETURN

  write_required_schemas "$tmp"
  printf "%s" $'INSERT INTO users (name, age, major) VALUES (\'김민준\', 25, \'컴퓨터공학\');\nSELECT name FROM users;\n' >"$tmp/input.sql"

  set +e
  (
    cd "$tmp"
    "$BIN" "input.sql" >"$tmp/stdout.txt" 2>"$tmp/stderr.txt"
  )
  local code=$?
  set -e

  [ "$code" -eq 0 ] || return 1
  [ -f "$tmp/sql_processor.log" ] || return 1
  grep -F "SUCCESS INSERT users" "$tmp/sql_processor.log" >/dev/null
  grep -F "SUCCESS SELECT users" "$tmp/sql_processor.log" >/dev/null
}

case_logging_records_error_result() {
  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' RETURN

  write_required_schemas "$tmp"
  printf "%s" $'INSERT INTO users (name, age, major) VALUES (\'김민준\', 25);\n' >"$tmp/input.sql"

  set +e
  (
    cd "$tmp"
    "$BIN" "input.sql" >"$tmp/stdout.txt" 2>"$tmp/stderr.txt"
  )
  local code=$?
  set -e

  [ "$code" -eq 1 ] || return 1
  [ -f "$tmp/sql_processor.log" ] || return 1
  grep -F "ERROR INSERT users column count does not match value count" "$tmp/sql_processor.log" >/dev/null
}

case_logging_appends_across_multiple_runs() {
  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' RETURN

  write_required_schemas "$tmp"
  printf "%s" $'INSERT INTO users (name, age, major) VALUES (\'김민준\', 25, \'컴퓨터공학\');\n' >"$tmp/first.sql"
  printf "%s" $'SELECT name FROM users;\n' >"$tmp/second.sql"

  set +e
  (
    cd "$tmp"
    "$BIN" "first.sql" >"$tmp/out1.txt" 2>"$tmp/err1.txt"
    "$BIN" "second.sql" >"$tmp/out2.txt" 2>"$tmp/err2.txt"
  )
  local code=$?
  set -e

  [ "$code" -eq 0 ] || return 1
  [ -f "$tmp/sql_processor.log" ] || return 1

  local line_count
  line_count="$(wc -l <"$tmp/sql_processor.log")"
  [ "$line_count" -ge 2 ] || return 1
  grep -F "SUCCESS INSERT users" "$tmp/sql_processor.log" >/dev/null
  grep -F "SUCCESS SELECT users" "$tmp/sql_processor.log" >/dev/null
}

run_case "logging_creates_log_file_for_successful_run" case_logging_creates_log_file_for_successful_run
run_case "logging_records_error_result" case_logging_records_error_result
run_case "logging_appends_across_multiple_runs" case_logging_appends_across_multiple_runs

echo ""
echo "Total: $TOTAL, Pass: $PASS, Fail: $FAIL"

[ "$FAIL" -eq 0 ]

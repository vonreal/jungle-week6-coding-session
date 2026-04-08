#!/usr/bin/env python3

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from uuid import uuid4


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def main() -> None:
    parser = argparse.ArgumentParser(description="Initialize a harness loop log file.")
    parser.add_argument("--output-dir", default="runtime/loop-logs", help="Directory where log files are stored")
    parser.add_argument("--project-name", default="[PROJECT_NAME]", help="Project name to record in the first event")
    parser.add_argument("--session-id", help="Optional explicit session id")
    parser.add_argument("--turn-id", default="turn-001", help="Initial turn id")
    args = parser.parse_args()

    session_id = args.session_id or f"session-{uuid4().hex[:8]}"
    date_prefix = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    log_path = output_dir / f"{date_prefix}-{session_id}.jsonl"

    event = {
        "timestamp": utc_now(),
        "session_id": session_id,
        "turn_id": args.turn_id,
        "event": "session_started",
        "stage": "discover",
        "status": "started",
        "summary": f"Started harness session for {args.project_name}",
        "artifacts": [],
        "changed_files": [],
        "tests": [],
        "risks": [],
        "next_step": "Read project config and required docs",
        "approval": "not_required",
    }

    with log_path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(event, ensure_ascii=False) + "\n")

    print(log_path)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def load_metadata(raw: Optional[str]) -> dict:
    if not raw:
        return {}
    try:
        value = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid --metadata JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise SystemExit("--metadata must decode to a JSON object")
    return value


def main() -> None:
    parser = argparse.ArgumentParser(description="Append a harness loop event to a JSONL log file.")
    parser.add_argument("--file", required=True, help="Path to the JSONL log file")
    parser.add_argument("--event", required=True, help="Event name")
    parser.add_argument("--summary", required=True, help="Human-readable event summary")
    parser.add_argument("--status", default="info", help="Event status")
    parser.add_argument("--stage", default="", help="Workflow stage")
    parser.add_argument("--session-id", default="", help="Optional session id override")
    parser.add_argument("--turn-id", default="", help="Optional turn id")
    parser.add_argument("--next-step", default="", help="Suggested next step")
    parser.add_argument("--approval", default="not_required", help="Approval state")
    parser.add_argument("--artifact", action="append", default=[], help="Artifact path")
    parser.add_argument("--changed-file", action="append", default=[], help="Changed file path")
    parser.add_argument("--test", action="append", default=[], help="Executed or planned test")
    parser.add_argument("--risk", action="append", default=[], help="Risk note")
    parser.add_argument("--metadata", help="Extra JSON object payload")
    args = parser.parse_args()

    log_path = Path(args.file)
    log_path.parent.mkdir(parents=True, exist_ok=True)

    event = {
        "timestamp": utc_now(),
        "session_id": args.session_id,
        "turn_id": args.turn_id,
        "event": args.event,
        "stage": args.stage,
        "status": args.status,
        "summary": args.summary,
        "artifacts": args.artifact,
        "changed_files": args.changed_file,
        "tests": args.test,
        "risks": args.risk,
        "next_step": args.next_step,
        "approval": args.approval,
    }

    metadata = load_metadata(args.metadata)
    if metadata:
        event["metadata"] = metadata

    with log_path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(event, ensure_ascii=False) + "\n")

    print(log_path)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Hardware Validation Program command line tooling.

This utility prepares repeatable hardware validation runs and records operator
results. It does not execute physical robot tests and it does not mark any test
case as passed automatically.
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
import enum
import json
import os
import platform
import shutil
import socket
import subprocess
import sys
import threading
import uuid
from pathlib import Path
from typing import Any


class ValidationStatus(str, enum.Enum):
    """Allowed status values for hardware validation results."""

    PASS = "PASS"
    FAIL = "FAIL"
    SKIPPED = "SKIPPED"
    NOT_EXECUTED = "NOT EXECUTED"
    UNKNOWN = "UNKNOWN"


@dataclasses.dataclass(frozen=True)
class ValidationCase:
    """One operator-executed hardware validation case."""

    case_id: str
    title: str
    area: str
    objective: str
    procedure: list[str]
    expected_result: str
    safety_notes: list[str]
    evidence: list[str]

    @staticmethod
    def from_dict(data: dict[str, Any]) -> "ValidationCase":
        """Create a validation case from catalog JSON data."""

        return ValidationCase(
            case_id=str(data["id"]),
            title=str(data["title"]),
            area=str(data["area"]),
            objective=str(data["objective"]),
            procedure=[str(step) for step in data.get("procedure", [])],
            expected_result=str(data["expected_result"]),
            safety_notes=[str(note) for note in data.get("safety_notes", [])],
            evidence=[str(item) for item in data.get("evidence", [])],
        )


@dataclasses.dataclass
class ValidationResult:
    """Recorded result for one validation case."""

    case_id: str
    status: ValidationStatus
    notes: str
    operator: str
    timestamp_utc: str
    evidence: list[str]


class EnvironmentRecorder:
    """Captures host environment information for validation traceability."""

    def collect(self) -> dict[str, Any]:
        """Return environment metadata without requiring privileged access."""

        return {
            "timestamp_utc": _utc_now(),
            "hostname": socket.gethostname(),
            "platform": platform.platform(),
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
            "python": sys.version.split()[0],
            "cwd": str(Path.cwd()),
            "git": _run_optional(["git", "rev-parse", "--short", "HEAD"]),
        }


class RobotInformationCollector:
    """Records operator-supplied robot identity and firmware metadata."""

    def collect(
        self,
        robot_id: str,
        vendor: str,
        model: str,
        serial_number: str,
        firmware: str,
    ) -> dict[str, str]:
        """Return normalized robot information supplied by the operator."""

        return {
            "robot_id": robot_id,
            "vendor": vendor,
            "model": model,
            "serial_number": serial_number,
            "firmware": firmware,
        }


class LogCollector:
    """Copies validation evidence logs into a run directory."""

    def __init__(self, run_dir: Path) -> None:
        """Create a collector for a validation run directory."""

        self._log_dir = run_dir / "logs"
        self._log_dir.mkdir(parents=True, exist_ok=True)

    def collect(self, source: Path) -> Path:
        """Copy one log file or directory into the validation run."""

        if not source.exists():
            raise FileNotFoundError(f"log source does not exist: {source}")
        destination = self._log_dir / source.name
        if source.is_dir():
            if destination.exists():
                shutil.rmtree(destination)
            shutil.copytree(source, destination)
        else:
            shutil.copy2(source, destination)
        return destination


class VideoReferenceManager:
    """Records external video evidence references for a validation run."""

    def __init__(self, run_dir: Path) -> None:
        """Create a manager backed by ``video_references.json``."""

        self._path = run_dir / "video_references.json"
        if not self._path.exists():
            self._write([])

    def add(self, case_id: str, reference: str, description: str) -> None:
        """Add a video reference without copying large media files."""

        references = self._read()
        references.append(
            {
                "case_id": case_id,
                "reference": reference,
                "description": description,
                "timestamp_utc": _utc_now(),
            }
        )
        self._write(references)

    def _read(self) -> list[dict[str, Any]]:
        with self._path.open("r", encoding="utf-8") as handle:
            return list(json.load(handle))

    def _write(self, data: list[dict[str, Any]]) -> None:
        with self._path.open("w", encoding="utf-8") as handle:
            json.dump(data, handle, indent=2, sort_keys=True)
            handle.write("\n")


class ResultRecorder:
    """Thread-safe result store for validation case execution records."""

    def __init__(self, run_dir: Path) -> None:
        """Create a recorder backed by ``results.json``."""

        self._path = run_dir / "results.json"
        self._lock = threading.Lock()

    def initialize(self, cases: list[ValidationCase], operator: str) -> None:
        """Create default NOT EXECUTED records for all catalog cases."""

        timestamp = _utc_now()
        results = [
            dataclasses.asdict(
                ValidationResult(
                    case_id=case.case_id,
                    status=ValidationStatus.NOT_EXECUTED,
                    notes="Awaiting human operator execution.",
                    operator=operator,
                    timestamp_utc=timestamp,
                    evidence=[],
                )
            )
            for case in cases
        ]
        with self._lock:
            self._write(results)

    def record(
        self,
        case_id: str,
        status: ValidationStatus,
        notes: str,
        operator: str,
        evidence: list[str],
    ) -> None:
        """Record or replace the result for one validation case."""

        with self._lock:
            results = self._read()
            updated = False
            for result in results:
                if result["case_id"] == case_id:
                    result.update(
                        {
                            "status": status.value,
                            "notes": notes,
                            "operator": operator,
                            "timestamp_utc": _utc_now(),
                            "evidence": evidence,
                        }
                    )
                    updated = True
                    break
            if not updated:
                results.append(
                    {
                        "case_id": case_id,
                        "status": status.value,
                        "notes": notes,
                        "operator": operator,
                        "timestamp_utc": _utc_now(),
                        "evidence": evidence,
                    }
                )
            self._write(results)

    def load(self) -> list[dict[str, Any]]:
        """Load all recorded results."""

        with self._lock:
            return self._read()

    def _read(self) -> list[dict[str, Any]]:
        if not self._path.exists():
            return []
        with self._path.open("r", encoding="utf-8") as handle:
            return list(json.load(handle))

    def _write(self, data: list[dict[str, Any]]) -> None:
        with self._path.open("w", encoding="utf-8") as handle:
            json.dump(data, handle, indent=2, sort_keys=True)
            handle.write("\n")


class TestReporter:
    """Generates human-readable validation summaries."""

    def summarize(self, run_dir: Path) -> dict[str, int]:
        """Return status counts for one validation run."""

        recorder = ResultRecorder(run_dir)
        summary = {status.value: 0 for status in ValidationStatus}
        for result in recorder.load():
            status = str(result.get("status", ValidationStatus.UNKNOWN.value))
            summary[status] = summary.get(status, 0) + 1
        return summary

    def write_markdown(self, run_dir: Path) -> Path:
        """Write ``summary.md`` for one validation run."""

        summary = self.summarize(run_dir)
        path = run_dir / "summary.md"
        with path.open("w", encoding="utf-8") as handle:
            handle.write("# Hardware Validation Run Summary\n\n")
            handle.write(f"Generated UTC: `{_utc_now()}`\n\n")
            for status in ValidationStatus:
                handle.write(f"- {status.value}: {summary.get(status.value, 0)}\n")
        return path


class ValidationRunner:
    """Creates repeatable hardware validation run directories."""

    def __init__(self, catalog_path: Path) -> None:
        """Load a validation catalog."""

        self._catalog_path = catalog_path
        self._cases = _load_catalog(catalog_path)

    def create_run(
        self,
        output_dir: Path,
        operator: str,
        robot_info: dict[str, str],
    ) -> Path:
        """Create a new run with all cases marked NOT EXECUTED."""

        run_id = f"hvp-{dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%SZ')}-{uuid.uuid4().hex[:8]}"
        run_dir = output_dir / run_id
        run_dir.mkdir(parents=True, exist_ok=False)
        metadata = {
            "run_id": run_id,
            "created_utc": _utc_now(),
            "operator": operator,
            "catalog": str(self._catalog_path),
            "robot": robot_info,
            "environment": EnvironmentRecorder().collect(),
        }
        _write_json(run_dir / "run.json", metadata)
        ResultRecorder(run_dir).initialize(self._cases, operator)
        TestReporter().write_markdown(run_dir)
        return run_dir

    def list_cases(self) -> list[ValidationCase]:
        """Return validation cases in catalog order."""

        return list(self._cases)


def _load_catalog(path: Path) -> list[ValidationCase]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    return [ValidationCase.from_dict(item) for item in data["cases"]]


def _write_json(path: Path, data: Any) -> None:
    with path.open("w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=2, sort_keys=True)
        handle.write("\n")


def _utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def _run_optional(command: list[str]) -> str:
    try:
        completed = subprocess.run(
            command,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError:
        return "unavailable"
    if completed.returncode != 0:
        return "unavailable"
    return completed.stdout.strip()


def _default_catalog() -> Path:
    return Path(__file__).resolve().parents[1] / "Test_Case_Catalog.json"


def _parse_status(value: str) -> ValidationStatus:
    normalized = value.strip().upper().replace("_", " ")
    for status in ValidationStatus:
        if status.value == normalized:
            return status
    raise argparse.ArgumentTypeError(f"invalid validation status: {value}")


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=_default_catalog())
    subparsers = parser.add_subparsers(dest="command", required=True)

    new_run = subparsers.add_parser("new-run", help="create a validation run")
    new_run.add_argument("--output", type=Path, default=Path("validation/reports/runs"))
    new_run.add_argument("--operator", required=True)
    new_run.add_argument("--robot-id", default="unrecorded")
    new_run.add_argument("--vendor", default="unrecorded")
    new_run.add_argument("--model", default="unrecorded")
    new_run.add_argument("--serial-number", default="unrecorded")
    new_run.add_argument("--firmware", default="unrecorded")

    record = subparsers.add_parser("record", help="record a test case result")
    record.add_argument("--run-dir", type=Path, required=True)
    record.add_argument("--case-id", required=True)
    record.add_argument("--status", type=_parse_status, required=True)
    record.add_argument("--operator", required=True)
    record.add_argument("--notes", default="")
    record.add_argument("--evidence", action="append", default=[])

    subparsers.add_parser("list-cases", help="list catalog cases")

    summarize = subparsers.add_parser("summarize", help="write run summary")
    summarize.add_argument("--run-dir", type=Path, required=True)

    collect_log = subparsers.add_parser("collect-log", help="copy log evidence")
    collect_log.add_argument("--run-dir", type=Path, required=True)
    collect_log.add_argument("--source", type=Path, required=True)

    add_video = subparsers.add_parser("add-video", help="add video reference")
    add_video.add_argument("--run-dir", type=Path, required=True)
    add_video.add_argument("--case-id", required=True)
    add_video.add_argument("--reference", required=True)
    add_video.add_argument("--description", default="")

    return parser


def main() -> int:
    """Command line entry point."""

    args = _build_parser().parse_args()
    runner = ValidationRunner(args.catalog)

    if args.command == "new-run":
        robot_info = RobotInformationCollector().collect(
            args.robot_id,
            args.vendor,
            args.model,
            args.serial_number,
            args.firmware,
        )
        run_dir = runner.create_run(args.output, args.operator, robot_info)
        print(run_dir)
        return 0

    if args.command == "record":
        ResultRecorder(args.run_dir).record(
            args.case_id,
            args.status,
            args.notes,
            args.operator,
            args.evidence,
        )
        TestReporter().write_markdown(args.run_dir)
        return 0

    if args.command == "list-cases":
        for case in runner.list_cases():
            print(f"{case.case_id}\t{case.area}\t{case.title}")
        return 0

    if args.command == "summarize":
        summary_path = TestReporter().write_markdown(args.run_dir)
        print(summary_path)
        return 0

    if args.command == "collect-log":
        destination = LogCollector(args.run_dir).collect(args.source)
        print(destination)
        return 0

    if args.command == "add-video":
        VideoReferenceManager(args.run_dir).add(
            args.case_id,
            args.reference,
            args.description,
        )
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())

"""Validate the curated migration map without compiling or executing the game.

This is a file/symbol-presence check, NOT a call-graph or behavior verifier.
No tests, binaries, subprocess builds, resource imports or network requests run.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
STATUSES = {"portable-rules", "legacy-boundary", "mixed", "reference"}


def checked_path(root: Path, value: str) -> Path:
    if not isinstance(value, str) or not value or "\\" in value or ":" in value:
        raise ValueError(f"invalid repository path: {value!r}")
    relative = PurePosixPath(value)
    if relative.is_absolute() or ".." in relative.parts:
        raise ValueError(f"path escapes repository: {value!r}")
    path = root.joinpath(*relative.parts)
    if not path.resolve().is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError(f"missing or external file: {value}")
    return path


def validate(root: Path) -> dict:
    document = json.loads((root / "docs/migration/source-map.json").read_text(encoding="utf-8"))
    if document.get("schema_version") != 1:
        raise ValueError("unsupported source-map schema")
    if not re.fullmatch(r"[0-9a-f]{40}", document.get("baseline_commit", "")):
        raise ValueError("baseline_commit must identify an exact source revision")
    routes = document.get("routes")
    if not isinstance(routes, list) or not routes:
        raise ValueError("routes must be a nonempty array")
    ids, paths, symbols = set(), set(), 0
    for route in routes:
        identifier = route["id"]
        if not re.fullmatch(r"[a-z0-9-]+", identifier) or identifier in ids:
            raise ValueError(f"invalid/duplicate route id: {identifier}")
        ids.add(identifier)
        if route["status"] not in STATUSES:
            raise ValueError(f"unknown status: {identifier}")
        for field in ("invariants", "blockers"):
            if not isinstance(route[field], list) or not route[field] or not all(
                isinstance(item, str) and item.strip() for item in route[field]
            ):
                raise ValueError(f"{identifier}: {field} must contain review notes")
        if not isinstance(route["sources"], dict) or not route["sources"]:
            raise ValueError(f"{identifier}: missing source files")
        if not isinstance(route["contracts"], list):
            raise ValueError(f"{identifier}: contracts must be an array")
        for filename, names in route["sources"].items():
            path = checked_path(root, filename)
            paths.add(filename)
            if not isinstance(names, list):
                raise ValueError(f"{identifier}: symbols must be an array")
            text = path.read_text(encoding="utf-8-sig") if names else ""
            for name in names:
                if not isinstance(name, str) or not re.fullmatch(r"[A-Za-z_]\w*", name):
                    raise ValueError(f"invalid symbol name: {name!r}")
                if not re.search(r"\b" + re.escape(name) + r"\s*\(", text):
                    raise ValueError(f"missing symbol text: {filename}: {name}")
                symbols += 1
        for filename in route["contracts"]:
            checked_path(root, filename)
            paths.add(filename)
    # Validate relative Markdown document links, without following external URLs.
    documents = [root / "MIGRATION.md", *(root / "docs/migration").glob("*.md")]
    for document_path in documents:
        text = document_path.read_text(encoding="utf-8")
        for target in re.findall(r"\]\(([^\s)]+)\)", text):
            if re.match(r"[A-Za-z][A-Za-z0-9+.-]*:", target) or target.startswith("#"):
                continue
            target = target.split("#", 1)[0]
            resolved = (document_path.parent / target).resolve()
            if not resolved.is_relative_to(root.resolve()) or not resolved.exists():
                raise ValueError(f"broken local link in {document_path.name}: {target}")
    return {"baseline_commit": document["baseline_commit"], "routes": len(ids),
            "referenced_files": len(paths), "symbol_mentions": symbols,
            "meaning": "File and symbol-text presence only; no call graph or behavioral certification."}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="New report path; existing reports are never overwritten")
    args = parser.parse_args()
    try:
        report = validate(ROOT)
        report["checked_commit"] = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
        ).strip()
        report["worktree_dirty"] = bool(subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=ROOT, text=True
        ).strip())
        output = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf-8") as stream:
                stream.write(output)
        print(output, end="")
        return 0
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f"migration source map: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

"""Export committed tracked source and recoverable Git history; never run tests."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]

    def git(*arguments):
        return subprocess.check_output(["git", "-C", str(repo), *arguments])

    if git("status", "--porcelain", "--untracked-files=no").strip():
        raise SystemExit("Commit tracked changes before exporting a recorded baseline")
    revision = git("rev-parse", "HEAD").decode().strip()
    destination = args.destination.resolve()
    if destination == repo or repo in destination.parents:
        raise SystemExit("Use an independent destination outside this checkout")
    destination.mkdir(parents=True, exist_ok=False)
    archive = destination / "source.zip"
    bundle = destination / "source.bundle"
    git("archive", "--format=zip", "--output=" + str(archive), revision)
    git("bundle", "create", str(bundle), "HEAD")
    (destination / "SOURCE_TREE.txt").write_bytes(git("ls-tree", "-r", revision))
    (destination / "SOURCE_REVISION.txt").write_text(revision + "\n", encoding="ascii")
    source = destination / "source"
    source.mkdir()
    inventory = []
    with zipfile.ZipFile(archive) as zipped:
        for entry in zipped.infolist():
            target = (source / entry.filename).resolve()
            if source not in target.parents:
                raise SystemExit("Archive member escapes extraction root")
            if entry.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            if (entry.external_attr >> 16) & 0o170000 == 0o120000:
                raise SystemExit("Symlink requires explicit migration handling: " + entry.filename)
            target.parent.mkdir(parents=True, exist_ok=True)
            data = zipped.read(entry)  # ZIP CRC verified during extraction.
            target.write_bytes(data)
            checksum = hashlib.sha256(data).hexdigest()
            if digest(target) != checksum:
                raise SystemExit("Extracted source hash mismatch: " + entry.filename)
            inventory.append({"path": entry.filename, "size": len(data), "sha256": checksum})
    # This is export metadata, not a modification to a tracked source file.
    (source / "SOURCE_REVISION.txt").write_text(revision + "\n", encoding="ascii")
    manifest = {
        "source_commit": revision,
        "tree": git("rev-parse", "HEAD^{tree}").decode().strip(),
        "source_archive_sha256": digest(archive),
        "git_bundle_sha256": digest(bundle),
        "source_file_count": len(inventory),
        "source_files": inventory,
        "game_run": False,
        "tests_run": False,
        "assets_included": False,
        "scope": "All tracked source/evidence/vendor files; no untracked local artifacts or user saves",
    }
    (destination / "source-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: value for key, value in manifest.items() if key != "source_files"}, indent=2))
    print("SOURCE_DIRECTORY", source)


if __name__ == "__main__":
    main()

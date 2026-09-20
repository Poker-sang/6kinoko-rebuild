"""Reproduce the small, unmodified Xiph release source sets used by this build.

Downloads are hash-pinned. --archives can instead point at existing archives.
Only listed C/header/license files are copied; tar paths are not extracted.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import tarfile
import urllib.request

RELEASES = {
    "libogg-1.1.3": ("https://downloads.xiph.org/releases/ogg/libogg-1.1.3.tar.gz",
        "bae29e79fbc50bbedf1235852094b71c8c910a1ef0cd42fe4163b7b545630b65"),
    "libvorbis-1.2.0": ("https://downloads.xiph.org/releases/vorbis/libvorbis-1.2.0.tar.gz",
        "6eb7040048e35448fe224fa3fd993eb4e49a905c57893886082f1674d43b0e73"),
}
VORBIS_C = set("mdct smallft block envelope window lsp lpc analysis synthesis psy info floor1 floor0 res0 mapping0 registry codebook sharedbook lookup bitrate vorbisfile vorbisenc".split())

def selected(name: str, path: PurePosixPath) -> bool:
    if str(path) in {"COPYING", "AUTHORS", "CHANGES", "README", "lib/Makefile.am", "src/Makefile.am"}:
        return True
    if path.parts[0] == "include" and path.suffix in {".h", ".in"}:
        return True
    if name.startswith("libogg"):
        return str(path) in {"src/framing.c", "src/bitwise.c", "src/crctable.h"}
    return path.parts[0] == "lib" and (path.suffix == ".h" or (len(path.parts) == 2 and path.suffix == ".c" and path.stem in VORBIS_C))

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archives", type=Path)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    for name, (url, digest) in RELEASES.items():
        data = (args.archives / (name + ".tar.gz")).read_bytes() if args.archives else urllib.request.urlopen(url, timeout=90).read()
        if hashlib.sha256(data).hexdigest() != digest:
            raise RuntimeError("Release archive hash mismatch: " + name)
        output = args.root / "third_party" / name
        files = {}
        with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as archive:
            for member in archive.getmembers():
                path = PurePosixPath(member.name)
                if not member.isfile() or path.parts[0] != name:
                    continue
                relative = PurePosixPath(*path.parts[1:])
                if not relative.parts or ".." in relative.parts or not selected(name, relative):
                    continue
                payload = archive.extractfile(member).read()
                destination = output / str(relative)
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(payload)
                files[str(relative)] = hashlib.sha256(payload).hexdigest()
        (output / "UPSTREAM.json").write_text(json.dumps({"release": name, "url": url,
            "archive_sha256": digest, "files": dict(sorted(files.items()))}, indent=2) + "\n", encoding="utf-8")
        print(name, len(files), "verified upstream files")

if __name__ == "__main__":
    main()

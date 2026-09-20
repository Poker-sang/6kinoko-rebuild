"""Vendor selected historical binding/counting sources from hash-pinned archives.

No VM, examples, bundled Google Test, binaries, or full Boost distribution is
copied. The fetched archive is data, never executed. --archives is an offline
cache populated by the source checkpoint. Existing destination bytes must match.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import tarfile
import urllib.request
import zipfile

RELEASES = {
    'sqrat-0.8.1': ('sqrat_0.8.1.zip',
        'https://downloads.sourceforge.net/project/scrat/Sqrat/Sqrat%200.8/sqrat_0.8.1.zip',
        'b9fead15226f3a44405e66d87d90e1d8c4a2f249ca3e1fa1b949ca4f3462051c'),
    'sqplus-20080713': ('sqplus-candidate.zip',
        'https://sourceforge.net/projects/sqplus/files/latest/download',
        '56b3c39fe77a19f5bad9ee0982e5ecb8e4f86625b3bfcd31945589e5c613cd56'),
    'boost-1.44.0': ('boost_1_44_0.tar.bz2',
        'https://archives.boost.io/release/1.44.0/source/boost_1_44_0.tar.bz2',
        '45c328029d97d1f1dc7ff8c9527cd0c5cc356636084a800bca2ee4bfab1978db'),
}

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def archive_files(data: bytes, zipped: bool, prefix: str) -> dict[str, bytes]:
    result = {}
    if zipped:
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            for entry in archive.infolist():
                if not entry.is_dir() and entry.filename.startswith(prefix):
                    result[entry.filename[len(prefix):]] = archive.read(entry)
    else:
        with tarfile.open(fileobj=io.BytesIO(data), mode='r:bz2') as archive:
            for entry in archive:
                if entry.isfile() and entry.name.startswith(prefix):
                    result[entry.name[len(prefix):]] = archive.extractfile(entry).read()
    return result

def select(name: str, files: dict[str, bytes]) -> set[str]:
    if name.startswith('sqrat'):
        return {p for p in files if (p.startswith('include/sqrat/') and p.endswith('.h'))
                or p in {'include/sqrat.h', 'docs/History.txt', 'docs/binding.html', 'docs/index.html'}}
    if name.startswith('sqplus'):
        return {p for p in files if (p.startswith('sqplus/') and p.endswith(('.h', '.cpp', '.txt')))
                or p == 'COPYRIGHT'}
    # Configuration uses computed #include names. Retain that small family;
    # other includes are resolved transitively from the two used backends.
    chosen = {p for p in files if p.startswith('boost/config/') and p.endswith('.hpp')}
    chosen |= {'boost/config.hpp', 'boost/version.hpp', 'LICENSE_1_0.txt',
               'boost/smart_ptr/detail/sp_counted_base_w32.hpp',
               'boost/smart_ptr/detail/sp_counted_base_pt.hpp'}
    pending = list(chosen)
    while pending:
        path = pending.pop()
        if path not in files:
            raise RuntimeError('Missing required upstream header: ' + path)
        for include in re.findall(rb'#\s*include\s*[<"](boost/[^>"\r\n]+)[>"]', files[path]):
            dependency = include.decode('ascii')
            if dependency not in chosen:
                chosen.add(dependency); pending.append(dependency)
    return chosen

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archives', type=Path)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    for name, (filename, url, digest) in RELEASES.items():
        data = (args.archives / filename).read_bytes() if args.archives else urllib.request.urlopen(url, timeout=120).read()
        if sha(data) != digest:
            raise RuntimeError('Release archive hash mismatch: ' + name)
        prefix = ('SQUIRREL2_1_1_sqplus_snapshot_20080713/' if name.startswith('sqplus')
                  else 'boost_1_44_0/' if name.startswith('boost') else '')
        files = archive_files(data, filename.endswith('.zip'), prefix)
        output = args.root / 'third_party' / name
        manifest = {}
        for path in sorted(select(name, files)):
            relative = PurePosixPath(path)
            if relative.is_absolute() or '..' in relative.parts:
                raise RuntimeError('Unsafe archive member')
            destination = output / path
            payload = files[path]
            destination.parent.mkdir(parents=True, exist_ok=True)
            if destination.exists() and destination.read_bytes() != payload:
                raise RuntimeError('Refusing to overwrite local edits: ' + str(destination))
            destination.write_bytes(payload)
            manifest[path] = sha(payload)
        (output / 'UPSTREAM.json').write_text(json.dumps({'release': name, 'url': url,
            'archive_sha256': digest, 'files': manifest}, indent=2) + '\n', encoding='utf-8')
        print(name, len(manifest), 'verified upstream files')

if __name__ == '__main__':
    main()

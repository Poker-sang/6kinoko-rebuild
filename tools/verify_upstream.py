"""Verify retained historical source members and documented local adaptations.

This checks the reviewed manifests, not the authenticity of a remote server or
an original game executable. Git's CRLF/LF checkout conversion alone is allowed.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path, PurePosixPath
import re

LIBRARIES = ('boost-1.44.0', 'sqplus-20080713', 'sqrat-0.8.1',
             'libogg-1.1.3', 'libvorbis-1.2.0')
SOURCE_SUFFIXES = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inl'}
SHA256 = re.compile(r'[0-9a-f]{64}\Z')


def _digest(value: object) -> str:
    if not isinstance(value, str) or not SHA256.fullmatch(value):
        raise ValueError('invalid SHA-256')
    return value


def _member(root: Path, name: str) -> Path:
    path = PurePosixPath(name)
    if not name or '\\' in name or path.is_absolute() or '..' in path.parts:
        raise ValueError(f'unsafe member path: {name}')
    result = root.joinpath(*path.parts)
    if not result.resolve().is_relative_to(root.resolve()):
        raise ValueError(f'member escapes source directory: {name}')
    return result


def _matches(data: bytes, expected: str) -> bool:
    # No trimming, whitespace rewriting or source-token normalization.
    lf = data.replace(b'\r\n', b'\n')
    return any(hashlib.sha256(candidate).hexdigest() == expected
               for candidate in (data, lf, lf.replace(b'\n', b'\r\n')))


def verify(root: Path, libraries: tuple[str, ...] = LIBRARIES) -> tuple[list[str], int]:
    errors: list[str] = []
    checked = 0
    for library in libraries:
        directory = root / 'third_party' / library
        try:
            manifest = json.loads((directory / 'UPSTREAM.json').read_text(encoding='utf-8'))
            _digest(manifest['archive_sha256'])
            if not manifest['release'] or not manifest['url'].startswith('https://'):
                raise ValueError('release and HTTPS source URL are required')
            members = manifest['files']
            if not isinstance(members, dict) or not members:
                raise ValueError('source member manifest is empty')
            patch_file = directory / 'PATCHES.json'
            patches = json.loads(patch_file.read_text(encoding='utf-8')) if patch_file.exists() else {}
            patches = patches.get('files', patches)
            if not isinstance(patches, dict) or not set(patches).issubset(members):
                raise ValueError('patch refers to an unmanifested member')
            if patches and not (directory / 'PATCHES.md').is_file():
                raise ValueError('local adaptations need PATCHES.md')
            for name, original in members.items():
                expected = _digest(original)
                if name in patches:
                    patch = patches[name]
                    before = patch.get('original_sha256', patch.get('upstream_sha256'))
                    if _digest(before) != original:
                        raise ValueError(f'{name}: patch baseline does not match upstream manifest')
                    expected = _digest(patch.get('adapted_sha256', patch.get('patched_sha256')))
                source = _member(directory, name)
                if not source.is_file() or not _matches(source.read_bytes(), expected):
                    errors.append(f'{library}/{name}: missing or unrecorded source modification')
                checked += 1
            for path in directory.rglob('*'):
                if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
                    if path.relative_to(directory).as_posix() not in members:
                        errors.append(f'{library}/{path.name}: source member missing from manifest')
        except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
            errors.append(f'{library}: invalid provenance: {error}')
    return errors, checked


if __name__ == '__main__':
    failures, count = verify(Path(__file__).resolve().parents[1])
    print('\n'.join(failures) if failures else f'PASS: {count} historical source members and local patches verified.')
    raise SystemExit(bool(failures))

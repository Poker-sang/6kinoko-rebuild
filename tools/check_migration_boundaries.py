"""Fail CI if handwritten assembly re-enters a compilable source tree.

The untouched original decompiler reference is evidence, not a build input.
Identifiers such as __asm_movsd are ordinary recovered C functions, not inline
assembly, and must not be mistaken for the compiler's __asm keyword.
"""
from __future__ import annotations
import hashlib
from pathlib import Path
import re
import sys
from verify_upstream import verify as verify_upstream

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = Path('src/decompiled/6kinoko.exe.c')
REFERENCE_SHA256 = '504d899c97d12700aad88d88edbc072f95c600184b684c0bf15bad7471821014'
LEXICAL = re.compile(r'R"([A-Za-z0-9_]*)\([\s\S]*?\)\1"|//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
# Unmodified upstream FPU code is not handwritten game compatibility code.
# Pin each reviewed exception: never exempt third_party/ as a directory.
UPSTREAM_ASM = {
    "third_party/libvorbis-1.2.0/lib/os.h":
        "cb8ecc2fb374d421915d6749492e099aee0461d6383bad5bc85893b504dbb32e",
}
ASM = re.compile(r'\b(?:__asm|__asm__|_asm|asm)\b|\b__declspec\s*\(\s*naked\s*\)')

# Original EXE addresses are not relocated C function pointers. A retained
# generated registration must name a compiled callback, even when the original
# decompiler merged that callback into another function's error branch.
CALLBACK_LITERAL = re.compile(r'\bsq_newclosure\s*\([^;]*,\s*\(\s*SQFUNCTION\s*\)\s*(?:kinoko_pointer\s*\(\s*)?(?:0x[0-9A-Fa-f]+|\d+)\b')

def masked(text: str) -> str:
    return LEXICAL.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)

def main() -> int:
    errors, upstream_members = verify_upstream(ROOT)
    reference = (ROOT / REFERENCE).read_bytes()
    # Git working trees on Windows may use CRLF. Preserve byte content modulo
    # only this checkout transformation; arbitrary whitespace is not ignored.
    if hashlib.sha256(reference.replace(b'\r\n', b'\n')).hexdigest() != REFERENCE_SHA256:
        errors.append('The original decompiler reference was changed.')
    scanned = 0
    for directory in ('src', 'include', 'tests', 'third_party'):
        for path in sorted((ROOT / directory).rglob('*')):
            if not path.is_file() or path.relative_to(ROOT) == REFERENCE:
                continue
            if path.suffix.lower() not in {'.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.inl'}:
                continue
            text = masked(path.read_text(encoding='utf-8', errors='strict'))
            scanned += 1
            upstream_hash = UPSTREAM_ASM.get(path.relative_to(ROOT).as_posix())
            if upstream_hash and hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest() != upstream_hash:
                errors.append(f'{path.relative_to(ROOT)}: reviewed upstream assembly source was modified')
            for match in ([] if upstream_hash else ASM.finditer(text)):
                line = text.count('\n', 0, match.start()) + 1
                errors.append(f'{path.relative_to(ROOT)}:{line}: handwritten assembly/naked entry')
            for match in CALLBACK_LITERAL.finditer(text):
                line = text.count('\n', 0, match.start()) + 1
                errors.append(f'{path.relative_to(ROOT)}:{line}: original-image native callback address')
    # The transitional frame/stack adapters have been fully retired. Do not
    # require their old compiler flags, and do not permit their reintroduction.
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    for source in ('src/platform/legacy_frame_entry.cpp',
                   'src/platform/legacy_frame_copy.cpp'):
        if (ROOT / source).exists() or source in cmake:
            errors.append('Retired stack/frame adapter returned: ' + source)
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print(f'PASS: {scanned} source/header files; zero handwritten inline assembly/naked entries and literal native-closure addresses; original reference intact.')
    print(f'PASS: {upstream_members} historical upstream members and documented patches verified.')
    print('PASS: retired frame/stack adapters remain absent.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

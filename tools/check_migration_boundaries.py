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

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = Path('src/decompiled/6kinoko.exe.c')
REFERENCE_SHA256 = '504d899c97d12700aad88d88edbc072f95c600184b684c0bf15bad7471821014'
LEXICAL = re.compile(r'R"([A-Za-z0-9_]*)\([\s\S]*?\)\1"|//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
ASM = re.compile(r'\b(?:__asm|__asm__|_asm|asm)\b|\b__declspec\s*\(\s*naked\s*\)')

def masked(text: str) -> str:
    return LEXICAL.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)

def main() -> int:
    errors: list[str] = []
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
            for match in ASM.finditer(text):
                line = text.count('\n', 0, match.start()) + 1
                errors.append(f'{path.relative_to(ROOT)}:{line}: handwritten assembly/naked entry')
    # These settings belong ONLY to the transitional adapter; removing them
    # changes its ABI. They are not a substitute for recovering copy operands.
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    if not re.search(r'set_source_files_properties\(src/platform/legacy_frame_entry\.cpp\s+PROPERTIES COMPILE_OPTIONS "/Oy-;/GL-"\)', cmake):
        errors.append('The isolated frame adapter lost its /Oy- /GL- ABI settings.')
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print(f'PASS: {scanned} source/header files; zero inline assembly/naked entries; original reference intact.')
    print('NOTE: legacy_frame_copy.cpp still preserves the old operand-selection heuristic.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

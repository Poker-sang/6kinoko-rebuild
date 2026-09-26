"""Pinned lexical migration inventory; NOT an AST or semantic completion score.

An independent file-wide marker pass keeps unparsed functions visible. No game,
resource import or contract binary is executed. Historical reports are immutable.
"""
from __future__ import annotations

import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = 'src/decompiled/6kinoko.exe.c'
EXTENSIONS = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.inl'}
LEX = re.compile(r'R"([A-Za-z0-9_]*)\([\s\S]*?\)\1"|//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
# Handles the previously missed T *name / T* name and noexcept(expression).
# Initializer lists, macros, operators and arbitrary C++ syntax remain outside
# the guarantee. Markers outside these candidates are reported independently.
SIGNATURE = re.compile(
    r'^[ \t]*(?:[\w:<>,~]+(?:[ \t]+|(?=[*&]))|[*&]+[ \t]*)*'
    r'(?P<name>[A-Za-z_~][\w:~]*)\s*\([^;{}]*\)\s*'
    r'(?:const\s*)?(?:noexcept(?:\([^()]*\))?\s*)?'
    r'(?:(?:override|final)\s*)?\{', re.M)
CONTROL = {'if', 'for', 'while', 'switch', 'catch', 'sizeof', 'alignof', 'static_assert'}
PATTERNS = {
    'address_symbol': r'\bfunction_[0-9a-fA-F]+(?:_\w+)?\b',
    'decompiler_identifier': r'\b(?:v\d+|g\d+)\b',
    'goto': r'\bgoto\b',
    'literal_offset_access': r'\b(?:field|load|store|pointer)\s*<[^;{}\n]+>\s*\([^;{}\n]*?\+\s*(?:0x[0-9a-fA-F]+|\d+)\b',
    'integer_pointer_cast': r'\breinterpret_cast\s*<\s*(?:u?int(?:32|64)_t|u?intptr_t)\s*>',
    'legacy_memory_api': r'\b(?:address|pointer|field)\s*(?:<[^;{}\n]+>)?\s*\(',
    'calling_convention': r'\b(?:__thiscall|__fastcall|__cdecl|__stdcall)\b',
    'platform_type': r'\b(?:HANDLE|HWND|CRITICAL_SECTION|IDirect3D\w*|IDirectSound\w*)\b',
    'layout_assertion': r'\bstatic_assert\s*\([^;{}]*\b(?:sizeof|offsetof)\s*\(',
}
RISK = {name: re.compile(pattern) for name, pattern in PATTERNS.items()}
DEBT = {'address_symbol', 'decompiler_identifier', 'goto', 'literal_offset_access', 'integer_pointer_cast'}


def masked(source: str) -> str:
    return LEX.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source)


def inspect_source(filename: str, source: str) -> dict:
    code = masked(source)
    markers = sorted(({'kind': kind, 'offset': m.start(),
                       'line': source.count('\n', 0, m.start()) + 1}
                      for kind, pattern in RISK.items() for m in pattern.finditer(code)),
                     key=lambda item: item['offset'])
    entries, consumed = [], 0
    for match in SIGNATURE.finditer(code):
        if match.start() < consumed or match['name'] in CONTROL:
            continue
        end, depth = match.end(), 1
        while end < len(code) and depth:
            depth += (code[end] == '{') - (code[end] == '}')
            end += 1
        if depth:
            # Do not certify a partial region. The whole-file scan still sees it.
            continue
        consumed = end
        hits = collections.Counter(m['kind'] for m in markers if match.start() <= m['offset'] < end)
        body = code[match.end():end-1]
        thin = body.count(';') <= 2 and not re.search(r'\b(if|for|while|switch|goto)\b', body)
        exported = ('extern "C"' in source[match.start():match.end()] or
                    match['name'].startswith(('kinoko_method_', 'function_')))
        category = ('legacy_marked_candidate' if DEBT.intersection(hits) else
                    'abi_adapter_candidate' if thin and exported else 'structured_candidate_unreviewed')
        entries.append({'name': match['name'], 'line': source.count('\n', 0, match.start())+1,
                        'lines': source[match.start():end].count('\n')+1, 'category': category,
                        'markers': dict(hits), '_range': (match.start(), end)})
    outside = [dict(kind=m['kind'], line=m['line']) for m in markers
               if not any(a <= m['offset'] < b for e in entries for a, b in [e['_range']])]
    for entry in entries:
        del entry['_range']
    return {'file': filename, 'lines': source.count('\n')+1,
            'markers': dict(collections.Counter(m['kind'] for m in markers)),
            'markers_outside_function_candidates': outside, 'entries': entries}


def git(*args: str) -> bytes:
    return subprocess.check_output(['git', '-C', str(ROOT), *args])


def inventory(source_ref: str, scope: str) -> dict:
    commit = git('rev-parse', '--verify', source_ref+'^{commit}').decode().strip()
    blobs = {}
    for entry in git('ls-tree', '-rz', commit).split(b'\0'):
        if not entry:
            continue
        metadata, name = entry.split(b'\t', 1)
        mode, kind, oid = metadata.decode().split()
        if kind == 'blob' and mode in {'100644', '100755'}:
            blobs[name.decode('utf-8')] = oid
    cmake = git('show', commit+':CMakeLists.txt').decode('utf-8-sig')
    selected = sorted(name for name in blobs if name.startswith(('src/', 'include/'))
                      and Path(name).suffix in EXTENSIONS and name != REFERENCE)
    if scope == 'cmake':
        selected = sorted(set(re.findall(r'src/[\w/.-]+\.(?:cpp|c)\b', cmake)))
        selected = [name for name in selected if name != REFERENCE]
    if any(name not in blobs for name in selected):
        raise ValueError('Selected source is missing or not an ordinary tracked blob')
    # One process, exact object sizes: no shell interpolation or per-file git spawn.
    request = ''.join(blobs[name]+'\n' for name in selected).encode()
    result = subprocess.run(['git', '-C', str(ROOT), 'cat-file', '--batch'],
                            input=request, stdout=subprocess.PIPE, check=True).stdout
    position, files, fingerprint = 0, [], hashlib.sha256()
    for name in selected:
        newline = result.index(b'\n', position)
        oid, kind, count = result[position:newline].decode().split()
        if oid != blobs[name] or kind != 'blob':
            raise ValueError('Unexpected Git blob response for '+name)
        position = newline+1
        data = result[position:position+int(count)]
        position += int(count)+1
        fingerprint.update(name.encode()+b'\0'+oid.encode()+b'\n')
        record = inspect_source(name, data.decode('utf-8-sig'))
        record['blob'] = oid
        files.append(record)
    categories, markers, outside = collections.Counter(), collections.Counter(), collections.Counter()
    for file in files:
        categories.update(e['category'] for e in file['entries'])
        markers.update(file['markers'])
        outside.update(m['kind'] for m in file['markers_outside_function_candidates'])
    return {'schema_version': 2, 'source_commit': commit, 'source_fingerprint_sha256': fingerprint.hexdigest(),
            'scope': scope, 'scope_definition': (
                'All ordinary tracked src/include C/C++ files, including headers; original evidence and vendor/tests excluded.'
                if scope == 'project' else 'Unique literal src C/C++ paths in top-level CMake; not evaluated build reachability.'),
            'limitations': ['Lexical candidates, not an AST, actual call graph or semantic acceptance.',
                           'Conditional branches/templates are not resolved or deduplicated; headers are not separate linked functions.',
                           'Macros/operators/initializer-list definitions may be missed. Whole-file markers cover their text independently.',
                           'Markers are review leads, including necessary ABI/layout boundaries and possible false positives.',
                           'No A/B acceptance or percent-complete is inferred. Historical schema-1 percentages are not comparable.'],
            'summary': {'files': len(files), 'function_candidates': sum(categories.values()),
                        'categories': dict(categories), 'file_wide_markers': dict(markers),
                        'markers_outside_function_candidates': dict(outside)}, 'files': files}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-ref', default='HEAD', help='Exact commit/ref to inspect, not uncommitted source')
    parser.add_argument('--scope', choices=('project', 'cmake'), default='project')
    parser.add_argument('--output', type=Path, required=True, help='New JSON report; never overwrite a prior report')
    args = parser.parse_args()
    try:
        if args.output.exists():
            raise FileExistsError('Report already exists: '+str(args.output))
        report = inventory(args.source_ref, args.scope)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open('x', encoding='utf-8') as stream:
            json.dump(report, stream, indent=2, ensure_ascii=False)
            stream.write('\n')
        print(json.dumps({k: v for k, v in report.items() if k != 'files'}, indent=2))
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print('Readability inventory: '+str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())

"""Read-only audit of compatibility exports against a pinned source and maps.

A missing map symbol is not enough: keep every lexical call, address-taking,
initializer, declaration, macro or string lookup reference in all tracked
runtime/vendor/test/build files. Ignore C/C++ comments, not string literals.
This intentionally does not guess aliasing, decode C++ RTTI or delete code.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = 'src/platform/retdec_runtime_compat.cpp'
REFERENCE = 'src/decompiled/6kinoko.exe.c'
LEX = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
DEFINITION = re.compile(
    r'^(?:static\s+)?(?:RETDEC_NOINLINE\s+)?'
    r'(?:void|int|long|unsigned|float|double|int\d+_t|uint\d+_t|char)'
    r'[^\n;{}=]*?\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{', re.M)


def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args])


def sha(data):
    return hashlib.sha256(data).hexdigest()


def mask(text, strings=False):
    def replace(match):
        if not strings and not match[0].startswith('/'):
            return match[0]
        return ''.join('\n' if c == '\n' else ' ' for c in match[0])
    return LEX.sub(replace, text)


def definitions(text):
    code = mask(text, strings=True)
    for match in DEFINITION.finditer(code):
        end, depth = match.end(), 1
        while depth and end < len(code):
            depth += (code[end] == '{') - (code[end] == '}')
            end += 1
        if depth:
            raise ValueError('Unbalanced definition: ' + match[1])
        yield dict(name=match[1], start=match.start(), end=end,
                   line=text.count('\n', 0, match.start()) + 1)


def audit(source_ref, map_paths):
    commit = git('rev-parse', '--verify', source_ref + '^{commit}').decode().strip()
    source = git('show', commit + ':' + TARGET)
    text = source.decode('utf-8')
    local_tokens = mask(text)
    maps = []
    for path in map_paths:
        recorded = path.with_name('source-commit.txt').read_text(encoding='utf-8-sig').strip()
        if recorded != commit:
            raise ValueError(f'{path}: source-commit.txt does not match {commit}')
        data = path.read_bytes()
        maps.append((path, data, data.decode('utf-8-sig')))
    if not maps:
        raise ValueError('At least one matching link map is required')
    # Pin files to the same revision as the maps, even when run after cleanup.
    files, fingerprint = [], hashlib.sha256()
    for name in sorted(git('ls-tree', '-r', '--name-only', commit).decode().splitlines()):
        if name in (TARGET, REFERENCE):
            continue
        if not (name.startswith(('src/', 'include/', 'tests/', 'third_party/', 'tools/', '.github/'))
                or name == 'CMakeLists.txt'):
            continue
        data = git('show', commit + ':' + name)
        try:
            content = data.decode('utf-8-sig')
        except UnicodeDecodeError:
            continue
        if '\0' in content:
            continue
        fingerprint.update(name.encode() + b'\0' + data + b'\0')
        files.append((name, mask(content)))
    removed, kept = [], []
    for entry in definitions(text):
        token = re.compile(r'(?<![A-Za-z0-9_])' + re.escape(entry['name']) + r'(?![A-Za-z0-9_])')
        references = [name for name, content in files if token.search(content)]
        self_references = len(token.findall(local_tokens)) - 1
        # These are extern-C x86 cdecl entries: add the linker underscore.
        symbol = re.compile(r'(?<![A-Za-z0-9_])_' + re.escape(entry['name']) + r'(?![A-Za-z0-9_])')
        linked = [str(path) for path, _, content in maps if symbol.search(content)]
        if references or self_references or linked:
            kept.append(dict(name=entry['name'], source_references=references,
                             local_references=self_references, linked=linked))
            continue
        body = text[entry['start']:entry['end']]
        removed.append(dict(name=entry['name'], line=entry['line'], lines=body.count('\n') + 1,
                            body_sha256=sha(body.encode())))
    return dict(source_commit=commit, implementation_file=TARGET,
                implementation_sha256=sha(source), scanned_file_count=len(files),
                scanned_source_sha256=fingerprint.hexdigest(),
                maps=[dict(name=path.name, sha256=sha(data)) for path, data, _ in maps],
                candidates=removed, retained=kept)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-ref', required=True)
    parser.add_argument('--map', type=Path, action='append', required=True, dest='maps')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.source_ref, args.maps)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f"{len(report['candidates'])} unreferenced compatibility definitions; "
          f"{len(report['retained'])} retained; {report['scanned_file_count']} files checked")


if __name__ == '__main__':
    main()

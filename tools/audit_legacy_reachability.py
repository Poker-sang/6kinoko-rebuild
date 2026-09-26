"""Conservative source graph for generated address functions; dry-run by default.

All conditional variants, main-TU initializers and non-address helpers are roots.
Other project sources, headers and tests are scanned even if not built. Symbol
references (including address-taking) and numeric address literals are edges.
An absent link-map symbol is additional evidence, not sufficient by itself.
"""
import argparse
import hashlib
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / 'src/reconstructed/runtime_host.cpp'
LEX = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
NAME = re.compile(r'\b(?:retdec_unbound_)?function_[0-9a-f]+(?:_\w+)?\b')
DEFINITION = re.compile(r'^(?:static\s+)?(?:void|bool|char|int|(?:u?int(?:8|16|32|64)_t)|float(?:32|64)_t)[^\n;{}=]*\b((?:retdec_unbound_)?function_[0-9a-f]+(?:_\w+)?)\s*\([^;{}]*\)\s*\{', re.M)
PROTOTYPE = re.compile(r'^(?:static\s+)?(?:void|bool|char|int|(?:u?int(?:8|16|32|64)_t)|float(?:32|64)_t)[^\n;{}=]*\b(?:retdec_unbound_)?function_[0-9a-f]+(?:_\w+)?\s*\([^;{}]*\)\s*;', re.M)


def mask(text):
    return LEX.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)


def definitions(text):
    code = mask(text)
    result = []
    for match in DEFINITION.finditer(code):
        end, depth = match.end(), 1
        while depth and end < len(code):
            depth += (code[end] == '{') - (code[end] == '}')
            end += 1
        if depth:
            raise ValueError('Unbalanced definition: ' + match[1])
        result.append(dict(name=match[1], start=match.start(), end=end,
                           line=text.count('\n', 0, match.start()) + 1))
    return code, result


def audit(link_map):
    text = MAIN.read_text(encoding='utf-8')
    code, entries = definitions(text)
    names = {d['name'] for d in entries}
    addresses = defaultdict(set)
    for name in names:
        addresses[int(re.search(r'function_([0-9a-f]+)', name)[1], 16)].add(name)

    def references(code):
        found = set(NAME.findall(code)) & names
        # Both decimal and hex pointers occur in recovered tables.
        for literal in re.findall(r'\b(?:0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*\b', code):
            number = re.sub(r'[uUlL]+$', '', literal)
            found.update(addresses.get(int(number, 16 if number.lower().startswith('0x') else 10), ()))
        return found

    remaining = list(code)
    edges = defaultdict(set)
    for d in entries:
        edges[d['name']].update(references(code[d['start']:d['end']]) - {d['name']})
        remaining[d['start']:d['end']] = ' ' * (d['end'] - d['start'])
    outside = PROTOTYPE.sub('', ''.join(remaining))
    roots = references(outside)
    files = []
    for folder in ('src', 'include', 'tests'):
        for path in sorted((ROOT / folder).rglob('*')):
            if path.suffix.lower() not in {'.c', '.cpp', '.h', '.hpp', '.inl'}:
                continue
            if path == MAIN or path.name == '6kinoko.exe.c':
                continue
            roots.update(references(mask(path.read_text(encoding='utf-8'))))
            files.append(str(path.relative_to(ROOT)))
    map_text = link_map.read_text(encoding='utf-8') if link_map else ''
    linked = set(re.findall(r'(?<![A-Za-z0-9_])_?((?:retdec_unbound_)?function_[0-9a-f]+(?:_\w+)?)\b', map_text))
    roots.update(linked & names)
    live = set(roots)
    pending = list(roots)
    while pending:
        for target in edges[pending.pop()] - live:
            live.add(target)
            pending.append(target)
    dead = names - live
    removed = []
    for d in entries:
        if d['name'] in dead:
            body = text[d['start']:d['end']]
            removed.append(dict(**d, lines=body.count('\n') + 1,
                                sha256=hashlib.sha256(body.encode()).hexdigest(),
                                linked=d['name'] in linked))
    return dict(revision=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                source_sha256=hashlib.sha256(MAIN.read_bytes()).hexdigest(),
                definition_count=len(entries), unique_names=len(names), roots=len(roots),
                retained=len(live), candidate_names=len(dead),
                candidate_lines=sum(d['lines'] for d in removed),
                link_map=str(link_map) if link_map else None,
                link_map_sha256=hashlib.sha256(link_map.read_bytes()).hexdigest() if link_map else None,
                link_map_revision=(link_map.parent / 'source-commit.txt').read_text().strip()
                    if link_map and (link_map.parent / 'source-commit.txt').exists() else None,
                linked_candidates=sorted(dead & linked), scanned_files=files, candidates=removed,
                root_policy=__doc__)


def apply_removal(result):
    if not result['link_map'] or result['linked_candidates']:
        raise ValueError('Removal requires a baseline link map and no linked candidates')
    if result['link_map_revision'] != result['revision']:
        raise ValueError('Build/commit mismatch: create a map from the current committed baseline')
    if subprocess.run(['git', 'diff', '--quiet', result['revision'], '--',
                       'src', 'include', 'CMakeLists.txt'], cwd=ROOT).returncode:
        raise ValueError('Runtime source has uncommitted changes since the baseline build')
    if hashlib.sha256(MAIN.read_bytes()).hexdigest() != result['source_sha256']:
        raise ValueError('Source changed since audit')
    text = MAIN.read_text(encoding='utf-8')
    code = mask(text)
    spans = []
    removed = {d['name'] for d in result['candidates']}
    for d in result['candidates']:
        start, end = d['start'], d['end']
        if hashlib.sha256(text[start:end].encode()).hexdigest() != d['sha256']:
            raise ValueError('Definition changed: ' + d['name'])
        marker = text.rfind('// Address range:', 0, start)
        if marker >= 0 and not code[marker:start].strip():
            start = marker
        spans.append((start, end))
    for match in PROTOTYPE.finditer(code):
        if set(NAME.findall(match[0])) & removed:
            spans.append(match.span())
    last_start = len(text)
    for start, end in sorted(spans, reverse=True):
        if end > last_start:
            raise ValueError('Overlapping removal spans')
        text = text[:start] + text[end:]
        last_start = start
    if set(NAME.findall(mask(text))) & removed:
        raise ValueError('A removed name is still referenced')
    text = re.sub(r'\n{4,}', '\n\n\n', text)
    MAIN.write_text(text, encoding='utf-8')
    result['after_lines'] = len(text.splitlines())
    result['after_sha256'] = hashlib.sha256(MAIN.read_bytes()).hexdigest()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    result = audit(args.map)
    if args.apply:
        apply_removal(result)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k not in {'candidates', 'scanned_files', 'root_policy'}}, indent=2))

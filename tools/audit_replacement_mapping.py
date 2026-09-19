"""Validate evidence-backed disposition of the 600 formerly unmapped entries.

This is a mapping audit, not a semantic equivalence or runtime coverage test.
Retirement records do not assert a one-to-one replacement in Squirrel source.
"""
import argparse
from collections import Counter
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / 'docs/replacement-mapping-20260919/entries.json'
LEX = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')


@lru_cache(maxsize=64)
def mask(text):
    return LEX.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)


def definition(text, name):
    clean = mask(text)
    # Require declaration tokens before the name. A ternary call inside an
    # if-condition is not a function definition (the old inventory allowed it).
    match = re.search(r'^[ \t]*(?:[A-Za-z_]\w*[\s*&]+)+' + re.escape(name)
                      + r'\s*\([^;{}]*\)\s*\{', clean, re.M)
    if not match:
        return None
    end, depth = match.end(), 1
    while depth and end < len(clean):
        depth += (clean[end] == '{') - (clean[end] == '}')
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--link-map', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    ledger = json.loads(LEDGER.read_text(encoding='utf-8'))
    rows = ledger['entries']
    errors = []
    snapshots = {}
    sources = {}
    for directory in ('src', 'include'):
        for path in (ROOT / directory).rglob('*'):
            if path.suffix in ('.c', '.cpp', '.h', '.hpp', '.inl') and path.name != '6kinoko.exe.c':
                sources[path.relative_to(ROOT).as_posix()] = path.read_text(encoding='utf-8')
    tokens = set(re.findall(r'\bfunction_[0-9a-f]+\b', '\n'.join(mask(s) for s in sources.values())))
    cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
    link_map = args.link_map.read_text(encoding='utf-8') if args.link_map else None
    map_hits = {}
    for row in rows:
        name = row['function']
        for key in ('last_present_revision', 'first_absent_revision'):
            revision = row[key]
            if revision not in snapshots:
                snapshots[revision] = subprocess.check_output(
                    ['git', 'show', revision + ':src/decompiled/6kinoko_rebuilt.c'], cwd=ROOT).decode('utf-8')
        old = definition(snapshots[row['last_present_revision']], name)
        if old is None or hashlib.sha256(old.encode()).hexdigest() != row['last_body_sha256']:
            errors.append(name + ': historical definition/hash mismatch')
        if definition(snapshots[row['first_absent_revision']], name) is not None:
            errors.append(name + ': claimed removal still has an exact definition')
        if row['status'] == 'unresolved':
            continue
        if name in tokens:
            errors.append(name + ': old exact symbol still occurs in project code')
        if link_map and re.search(r'(?<![\w])[_@]?' + name + r'(?:@\d+)?(?![\w])', link_map):
            errors.append(name + ': old exact entry appears in supplied link map')
        target = row.get('replacement')
        if target:
            if target['path'] not in cmake:
                errors.append(name + ': target source is not explicitly in CMake')
            if definition(sources.get(target['path'], ''), target['symbol']) is None:
                errors.append(name + ': replacement definition missing')
            if link_map:
                map_hits[name] = target['symbol'] in link_map
        if row['status'] == 'retired_source_runtime_switch' and not row['first_absent_revision'].startswith('49dfad5'):
            errors.append(name + ': wrong source-runtime retirement commit')
    counts = Counter(r['status'] for r in rows)
    independent = Counter(r['status'] for r in rows if r['original_is_start'])
    result = dict(entries=len(rows), independent_entries=sum(independent.values()),
                  categories=dict(counts), independent_categories=dict(independent),
                  replacement_symbols_in_map=map_hits, errors=errors,
                  limits='Exact-symbol source/link checks; not raw-address reachability or per-function equivalence. '
                         'A missing optimized helper symbol can mean inlining or link pruning.')
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k != 'replacement_symbols_in_map'}, indent=2))
    return bool(errors)


if __name__ == '__main__':
    raise SystemExit(main())

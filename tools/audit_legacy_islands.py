"""Read-only, pinned-source audit of closed generated-code/data components.

C tables are graph nodes, not automatic entry points. A linked table can keep a
whole retired library alive without a call from the program. All unmodelled
source, all conditional branches and other runtime/vendor/test/build files are
roots. Calls, address-taking, string lookup names and original numeric addresses
are references. Maps are reported (including linked candidates), not used to
infer runtime execution. This is a lexical proof under the recovered C symbols
and literal-address model, not an arbitrary pointer-arithmetic/runtime oracle.
Records without an original address are retained as roots, together with their
dependencies. No code is deleted by this tool. Review the selected component
before removal.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import subprocess

from audit_legacy_reachability import definitions, PROTOTYPE
from audit_unused_crt import mask, sha, definitions as named_definitions

ROOT = Path(__file__).resolve().parents[1]
MAIN = 'src/decompiled/6kinoko_rebuilt.c'
REFERENCE = 'src/decompiled/6kinoko.exe.c'
NAMED_TOKEN = re.compile(r'\b[A-Za-z_]\w*\b')
NAMED_PROTOTYPE = re.compile(
    r'^(?:static\s+)?(?:void|int|long|unsigned|float|double|int\d+_t|uint\d+_t|char)'
    r'[^\n;{}=]*?\b[A-Za-z_]\w*\s*\([^;{}]*\)\s*;', re.M)
TOKEN = re.compile(r'\b(?:g\d+(?:_\w+)?|(?:retdec_unbound_)?function_[0-9a-f]+(?:_\w+)?)\b')
NUMBER = re.compile(r'\b(?:0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*\b')
GLOBAL = re.compile(
    r'^(?:static\s+)?(?:const\s+)?(?:struct\s+\w+|u?int\d+_t|char|float\d+_t|float|double)'
    r'[^;{}\n]*?\b(g\d+(?:_\w+)?)(?:\[[^\n]*?\])?\s*(?:=[^;]*|)\s*;', re.M)


def graph(text, external, *, include_named_functions=False):
    """Return a conservative graph; external is {path: UTF-8 source text}."""
    if include_named_functions:
        code, functions = mask(text, strings=True), list(named_definitions(text))
    else:
        code, functions = definitions(text)
    token = NAMED_TOKEN if include_named_functions else TOKEN
    prototype = NAMED_PROTOTYPE if include_named_functions else PROTOTYPE
    # C top-level only: never turn locals within non-address helpers into nodes.
    depth, depths = 0, []
    for character in code:
        depths.append(depth)
        depth += (character == '{') - (character == '}')
        if depth < 0:
            raise ValueError('Unbalanced translation unit')
    if depth:
        raise ValueError('Unbalanced translation unit')
    entities = [dict(e, kind='function') for e in functions if depths[e['start']] == 0]
    for match in GLOBAL.finditer(code):
        if depths[match.start()] != 0:
            continue
        entities.append(dict(name=match[1], kind='data', start=match.start(),
                             end=match.end(), line=text.count('\n', 0, match.start()) + 1))
    entities.sort(key=lambda e: e['start'])
    names = {e['name'] for e in entities}
    if len(names) != len(entities):
        raise ValueError('Duplicate generated definition; review conditional variants')
    addresses = defaultdict(set)
    previous = 0
    for entry in entities:
        if entry['start'] < previous:
            raise ValueError('Overlapping generated definitions')
        previous = entry['end']
        if entry['kind'] == 'function':
            named = re.search(r'function_([0-9a-f]+)', entry['name'])
            if named:
                address = int(named[1], 16)
            else:
                marker = text.rfind('// Address range:', 0, entry['start'])
                adjacent = marker >= 0 and not code[marker:entry['start']].strip()
                original = re.match(r'// Address range: (0x[0-9a-f]+)', text[marker:]) if adjacent else None
                address = int(original[1], 16) if original else None
        else:
            comment = re.match(r'[ \t]*//[ \t]*(0x[0-9a-fA-F]+)\b', text[entry['end']:])
            address = int(comment[1], 16) if comment else None
        entry['original_address'] = address
        if address is not None:
            addresses[address].add(entry['name'])
        body = text[entry['start']:entry['end']]
        entry['sha256'] = sha(body.encode())
        entry['lines'] = body.count('\n') + 1

    def references(source):
        result = set(token.findall(source)) & names
        for literal in NUMBER.findall(source):
            value = re.sub(r'[uUlL]+$', '', literal)
            base = 16 if value.lower().startswith('0x') else 8 if len(value) > 1 and value[0] == '0' else 10
            try:
                result.update(addresses.get(int(value, base), ()))
            except ValueError:
                # A number inside a string need not be a valid C integer. Treat
                # it as decimal as well; conservative false positives are safe.
                result.update(addresses.get(int(value, 10), ()))
        return result

    tokens = mask(text)  # Retain string literals; omit only C/C++ comments.
    edges = {}
    remainder = list(tokens)
    for entry in entities:
        name, start, end = entry['name'], entry['start'], entry['end']
        edges[name] = references(tokens[start:end]) - {name}
        remainder[start:end] = ' ' * (end - start)
    # Mask prototypes using structurally parsed spans, never by matching inside
    # retained string literals (which may name a dynamically resolved function).
    for match in prototype.finditer(code):
        if not any(e['start'] <= match.start() < e['end'] for e in entities):
            remainder[match.start():match.end()] = ' ' * (match.end() - match.start())
    root_sources = defaultdict(set)
    for entry in entities:
        if entry['original_address'] is None:
            # Do not invent a numeric range for RetDec synthetic globals. Keep
            # the record and everything it references; unrelated dead callers
            # can still be audited using their own verified original ranges.
            root_sources[entry['name']].add(MAIN + ':unknown-original-address')
    for name in references(''.join(remainder)):
        root_sources[name].add(MAIN + ':unmodelled')
    for path, source in external.items():
        for name in references(mask(source)):
            root_sources[name].add(path)
    live = set(root_sources)
    pending = list(live)
    while pending:
        for name in edges[pending.pop()] - live:
            live.add(name)
            pending.append(name)
    return entities, edges, root_sources, live


def component(entities, edges, roots, live, seeds):
    """Select a whole weak component of non-root-reachable nodes."""
    names = {e['name'] for e in entities}
    if not seeds or not set(seeds) <= names:
        raise ValueError('Unknown or empty component seed')
    if set(seeds) & live:
        raise ValueError('Selected seed is reachable from source roots')
    dead = names - live
    neighbors = defaultdict(set)
    for source, targets in edges.items():
        for target in targets:
            if source in dead and target in dead:
                neighbors[source].add(target)
                neighbors[target].add(source)
    selected, pending = set(seeds), list(seeds)
    while pending:
        for name in neighbors[pending.pop()] - selected:
            selected.add(name)
            pending.append(name)
    inbound = {source: sorted(targets & selected) for source, targets in edges.items()
               if source not in selected and targets & selected}
    if inbound or selected & set(roots):
        raise ValueError('Selected component has an outside reference')
    return selected



def interior_references(text, entries, external, original_text):
    """Fail closed on unbounded records; report pointers into selected bodies.

    RetDec sometimes merges a real callback into another function's error arm.
    Exact-symbol reachability alone cannot prove that such a body is unused.
    Collapsed data records use the next original address, not sizeof(the first
    word), so a literal pointing into a table also prevents its removal.
    """
    original_addresses = sorted({int(m[1], 16) for m in re.finditer(
        r'}?;\s*//\s*(0x[0-9a-fA-F]+)', original_text)})
    ranges = {}
    remaining = list(mask(text))
    for entry in entries:
        name, start, end = entry['name'], entry['start'], entry['end']
        lo = entry['original_address']
        if lo is None:
            raise ValueError('No original address for selected record: ' + name)
        if entry['kind'] == 'function':
            marker = text.rfind('// Address range:', 0, start)
            match = re.match(r'// Address range: (0x[0-9a-f]+) - (0x[0-9a-f]+)', text[marker:]) if marker >= 0 else None
            if not match or int(match[1], 16) != lo or mask(text[marker:start], strings=True).strip():
                raise ValueError('Unverified original function range: ' + name)
            hi = int(match[2], 16)
        else:
            hi = next((address for address in original_addresses if address > lo), None)
        if hi is None or hi <= lo:
            raise ValueError('Unbounded original record: ' + name)
        ranges[name] = [lo, hi]
        remaining[start:end] = ' ' * (end - start)
    sources = {MAIN: ''.join(remaining)}
    sources.update({path: mask(source) for path, source in external.items()})
    hits = []
    for path, code in sources.items():
        for match in NUMBER.finditer(code):
            literal = re.sub(r'[uUlL]+$', '', match[0])
            base = 16 if literal.lower().startswith('0x') else 8 if len(literal) > 1 and literal[0] == '0' else 10
            try:
                value = int(literal, base)
            except ValueError:
                value = int(literal, 10)
            for name, (lo, hi) in ranges.items():
                if lo <= value < hi:
                    hits.append(dict(source=path, record=name, literal=match[0],
                                     line=code.count('\n', 0, match.start()) + 1))
    return ranges, hits

def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args])


def audit(source_ref, maps, seeds, *, include_named_functions=False):
    commit = git('rev-parse', '--verify', source_ref + '^{commit}').decode().strip()
    text = git('show', commit + ':' + MAIN).decode()
    external, corpus = {}, {}
    for path in sorted(git('ls-tree', '-r', '--name-only', commit).decode().splitlines()):
        if path == REFERENCE or not (path.startswith(('src/', 'include/', 'tests/', 'third_party/', 'tools/', 'cmake/', '.github/'))
                                    or path == 'CMakeLists.txt'):
            continue
        data = git('show', commit + ':' + path)
        try:
            source = data.decode('utf-8-sig')
        except UnicodeDecodeError:
            continue
        if '\0' in source:
            continue
        corpus[path] = sha(data)
        if path != MAIN:
            external[path] = source
    entities, edges, roots, live = graph(text, external, include_named_functions=include_named_functions)
    selected = component(entities, edges, roots, live, seeds)
    selected_entries = [entry for entry in entities if entry['name'] in selected]
    original_text = git('show', commit + ':' + REFERENCE).decode()
    ranges, interior_hits = interior_references(text, selected_entries, external, original_text)
    if interior_hits:
        raise ValueError('Outside literal points into selected records: ' + repr(interior_hits))
    linked = defaultdict(list)
    map_reports = []
    for path in maps:
        recorded = path.with_name('source-commit.txt').read_text(encoding='utf-8-sig').strip()
        if recorded != commit:
            raise ValueError('Map/source commit mismatch: ' + str(path))
        data = path.read_bytes()
        content = data.decode('utf-8-sig')
        for name in selected:
            if re.search(r'(?<!\w)_' + re.escape(name) + r'(?!\w)', content):
                linked[name].append(str(len(map_reports)))
        map_reports.append(dict(name=path.name, sha256=sha(data), source_commit=recorded))
    entries = [dict(e, references=sorted(edges[e['name']]), linked_maps=linked[e['name']])
               for e in entities if e['name'] in selected]
    return dict(source_commit=commit, source_sha256=sha(text.encode()), policy=__doc__,
                seeds=seeds, include_named_functions=include_named_functions, source_files=corpus, maps=map_reports,
                graph_nodes=len(entities), root_nodes=len(roots), source_reachable=len(live),
                outside_inbound=[], interior_literal_ranges=ranges,
                outside_interior_literal_references=[], reference_sha256=sha(original_text.encode()),
                data_interval_policy='Original address to next original global address', candidates=entries,
                functions=sum(e['kind'] == 'function' for e in entries),
                data=sum(e['kind'] == 'data' for e in entries),
                selected_lines=sum(e['lines'] for e in entries),
                linker_evidence_available=bool(maps),
                linked_candidates=sum(bool(e['linked_maps']) for e in entries) if maps else None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-ref', required=True)
    parser.add_argument('--include-named-functions', action='store_true',
                        help='Also model recovered CRT names; unmodelled functions remain roots')
    parser.add_argument('--map', type=Path, action='append', default=[], dest='maps',
                        help='Optional matching linker evidence; source reachability is always checked')
    parser.add_argument('--seed', action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.source_ref, args.maps, args.seed, include_named_functions=args.include_named_functions)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    linker = (f"{report['linked_candidates']} linker-retained nodes" if args.maps
              else 'source-only audit; linker retention not measured')
    print(f"Closed component: {report['functions']} functions, {report['data']} data records; "
          f"{linker}; NO runtime claim.")


if __name__ == '__main__':
    main()

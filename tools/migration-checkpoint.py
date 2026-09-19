"""One-shot, hash-locked migration payload. Removed after publication."""
import collections
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
P = ROOT / 'src/decompiled/6kinoko_rebuilt.c'
text = P.read_text()
before = '2e5e72d351167b5852a2cd2ccee46ea58939c1f8f870c3f5ba94efdb8f970b84'
after = '831315880b2385de2d84819564a76068497c540cafc1d7e7a5e9d3eadf1ee9f7'
digest = lambda s: hashlib.sha256(s.encode()).hexdigest()
if digest(text) == after:
    raise SystemExit('Batch already applied.')
assert digest(text) == before, 'Source changed; reconcile instead of overwriting.'
lex = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
def mask(s):
    return lex.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), s)
masked = mask(text)
pattern = re.compile(r'^[^\n;{}#]*?\b([A-Za-z_]\w*)\s*\([^;{}]*?\)\s*\{', re.M)
defs = []
names = collections.defaultdict(list)
for m in pattern.finditer(masked):
    if m[1] in {'if', 'while', 'for', 'switch', '__except'}:
        continue
    start, br = m.start(), m.end() - 1
    header = masked[start:br].rstrip()
    i, depth = len(header) - 2, 1
    while depth:
        depth += (header[i] == ')') - (header[i] == '(')
        i -= 1
    am = re.search(r'([A-Za-z_]\w*)\s*$', header[:i+1])
    if not am:
        # Function-pointer-return declarations remain outside our deletable
        # set; their complete contents therefore remain conservative roots.
        continue
    name = am[1]
    if name in {'if', 'while', 'for', 'switch', '__except'}:
        continue
    depth, end = 1, br + 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    if defs and start < defs[-1]['end']:
        continue
    d = dict(name=name, start=start, end=end,
             line=text.count('\n', 0, start)+1,
             lines=text.count('\n', start, end)+1)
    defs.append(d)
    names[name].append(d)
allnames = set(names)
def refs(s):
    identifiers = set(re.findall(r'\b[A-Za-z_]\w*\b', s))
    addresses = {'function_'+h.lower() for h in re.findall(r'\b0[xX]([0-9A-Fa-f]+)\b', s)}
    return (identifiers | addresses) & allnames
edges = {n: set() for n in names}
callers = {n: set() for n in names}
for d in defs:
    edges[d['name']] |= refs(masked[d['start']:d['end']]) - {d['name']}
for n, targets in edges.items():
    for target in targets:
        callers[target].add(n)
roots = set()
for p in ROOT.rglob('*'):
    if p.suffix not in {'.c', '.cpp', '.h', '.hpp'} or p == P or '.git' in p.parts:
        continue
    # These tools resolve absolute addresses inside the ORIGINAL executable,
    # not rebuilt symbols. The reference dump remains completely untouched.
    if p.name in {'6kinoko.exe.c', 'original_vm_oracle.cpp', 'original_vm_oracle_host.cpp'}:
        continue
    if p.relative_to(ROOT).parts[0] not in {'include', 'src', 'tests', 'tools', 'third_party'}:
        continue
    roots |= refs(mask(p.read_text(errors='replace')))
outside = list(masked)
for d in defs:
    outside[d['start']:d['end']] = ' ' * (d['end'] - d['start'])
outside = ''.join(outside)
outside = re.sub(r'^[^\n;{}#=]*?\b[A-Za-z_]\w*\s*\([^;{}=]*?\)\s*;', '', outside, flags=re.M)
roots |= refs(outside)
roots |= {'WinMain', 'main'} & allnames
live, todo = set(roots), list(roots)
while todo:
    for n in edges[todo.pop()] - live:
        live.add(n)
        todo.append(n)
dead = allnames - live
seed = {'function_4689d0'}
for d in defs:
    name = d['name']
    if name not in dead:
        continue
    body = masked[d['start']:d['end']]
    m = re.fullmatch(r'function_([0-9a-f]+)', name)
    address = int(m[1], 16) if m else 0
    if any(t in body for t in ('_memcpy2(', '__ftol(', '__asm {')) or 0x473000 <= address < 0x4aa000:
        seed.add(name)
removed, todo = set(seed), list(seed)
while todo:
    for n in callers[todo.pop()] - removed:
        assert n in dead, n
        removed.add(n)
        todo.append(n)
assert removed <= dead
selected = [d for d in defs if d['name'] in removed]
assert len(removed) == 239 and len(selected) == 242
assert sum(d['lines'] for d in selected) == 41874
manifest = {
    'baseline_commit': '3cffc802f4c54ebb5ac1b9c5dfb5482e0be84547',
    'source': 'src/decompiled/6kinoko_rebuilt.c',
    'classification': 'disconnected definitions; deletion, NOT C++ migration',
    'root_policy': 'all external compilable-source references, all main-TU data initializers and literal numeric function addresses; main-TU prototypes and original-EXE-only oracle tools excluded',
    'definitions': [dict(name=d['name'], line=d['line'], body_lines=d['lines'],
                         sha256=digest(text[d['start']:d['end']])) for d in selected]
}
edits = [(d['start'], d['end'], '') for d in selected]
for m in re.finditer(r'^[^\n;{}#=]*?\b([A-Za-z_]\w*)\s*\([^;{}=]*?\)\s*;[^\S\n]*\n?', masked, re.M):
    if m[1] in removed and not any(d['start'] <= m.start() < d['end'] for d in defs):
        edits.append((m.start(), m.end(), ''))
for a, b, replacement in sorted(edits, reverse=True):
    text = text[:a] + replacement + text[b:]
text = re.sub(r'#if defined\(_MSC_VER\) && defined\(_M_IX86\)\s*#else\s*#endif', '', text)
assert digest(text) == after
assert not re.search(r'__ftol\s*\(', mask(text))
P.write_text(text)
output = ROOT / 'analysis/no-inline-asm-20260919'
output.mkdir(exist_ok=True)
(output / 'removed-definitions.json').write_text(json.dumps(manifest, indent=2)+'\n')
# Preserve a link map to distinguish retained legacy compatibility code from
# code discarded by /OPT:REF in the actual Win32 executable.
p = ROOT / '.github/workflows/windows-x86.yml'
s = p.read_text()
s = s.replace('            -DKINOKO_RETDEC_TRACE_FILTER=ON `', '            -DKINOKO_RETDEC_TRACE_FILTER=ON `\n            -DKINOKO_RETDEC_MAP_FILE="${{ github.workspace }}/build-runs/ci-${{ matrix.trace }}/kinoko.map" `')
s = s.replace('            build-runs/ci-${{ matrix.trace }}/*.log', '            build-runs/ci-${{ matrix.trace }}/*.log\n            build-runs/ci-${{ matrix.trace }}/*.map')
p.write_text(s)
print('Removed 242 disconnected definitions / 41874 body lines; original reference retained.')

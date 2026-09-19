# commit: refactor: remove retired lexical-cast and Dinkumware stream component
import io, json, re, subprocess, sys, zipfile
from pathlib import Path
sys.path.insert(0, 'tools')
from audit_legacy_islands import audit, git, MAIN, REFERENCE, NUMBER
from audit_unused_crt import mask, sha
from audit_legacy_reachability import PROTOTYPE

BASE = 'a33d3b858bc50ead84e097fc5fc8a9d4416e7544'
ARTIFACTS = [
 (10588839455, 'quiet', '4afe26eb955793bab43ecc5a52801bf6f78a765e13b355c5fc444d3415dbc486'),
 (10589579991, 'diagnostic', '46e81016e28ddf84623ee322867e90eb59ffb1b344bb02ff14f9fc7ceda83be5')]
maps = []
for artifact, variant, digest in ARTIFACTS:
    local = Path('/mnt/data/thirdparty-direct-crt-' + variant + '.zip')
    data = local.read_bytes() if local.exists() else subprocess.check_output(
        ['gh', 'api', f'repos/Poker-sang/6kinoko-rebuild/actions/artifacts/{artifact}/zip'])
    assert sha(data) == digest
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        parent = Path('build-runs/retired-library-evidence') / variant
        parent.mkdir(parents=True, exist_ok=True)
        for name in ['source-commit.txt', 'kinoko.map', 'stage-contract.map']:
            content = archive.read(f'build-runs/ci-{variant}/{name}')
            (parent / name).write_bytes(content)
            if name.endswith('.map'):
                maps.append(parent / name)
report = audit(BASE, maps, ['function_41a010'])
entries = report['candidates']
selected = {e['name'] for e in entries}
assert (report['functions'], report['data']) == (135, 48)
assert sha('\n'.join(sorted(selected)).encode()) == '09294fccc32728c6f7d8d86af6b1409012d3439e98c37af51faaf90d028e2093'
path = Path(MAIN)
text = path.read_text()
assert sha(text.encode()) == report['source_sha256']
# Independently guard literal pointers into object/function interiors, not only
# exact symbol starts. Selected data is exclusively 32-bit scalars and vtables.
intervals = {}
for entry in entries:
    if entry['kind'] == 'function':
        matches = list(re.finditer(r'// Address range: (0x[0-9a-f]+) - (0x[0-9a-f]+)', text[:entry['start']]))
        lo, hi = (int(value, 16) for value in matches[-1].groups())
        assert lo == entry['original_address']
    elif entry['original_address'] is not None:
        body = text[entry['start']:entry['end']]
        if body.startswith('int32_t'):
            size = 4
        else:
            assert body.startswith('struct vtable_')
            size = 4 * len(re.findall(r'\.e\d+\s*=', body))
        lo, hi = entry['original_address'], entry['original_address'] + size
    else:
        continue
    intervals[entry['name']] = [lo, hi]
remaining = list(mask(text))
for entry in entries:
    remaining[entry['start']:entry['end']] = ' ' * (entry['end'] - entry['start'])
sources = {MAIN: ''.join(remaining)}
for source in report['source_files']:
    if source != MAIN:
        sources[source] = mask(git('show', BASE + ':' + source).decode('utf-8-sig'))
for source, content in sources.items():
    for match in NUMBER.finditer(content):
        value = re.sub(r'[uUlL]+$', '', match[0])
        base = 16 if value.lower().startswith('0x') else 8 if value.startswith('0') and len(value) > 1 else 10
        try:
            value = int(value, base)
        except ValueError:
            value = int(value, 10)
        for name, (lo, hi) in intervals.items():
            assert not lo <= value < hi, (source, name, value)
report['interior_literal_ranges'] = intervals
report['outside_interior_literal_references'] = []
code = mask(text, strings=True)
spans = []
removed_types = set()
for entry in entries:
    start, end = entry['start'], entry['end']
    assert sha(text[start:end].encode()) == entry['sha256']
    if entry['kind'] == 'function':
        marker = text.rfind('// Address range:', 0, start)
        if marker >= 0 and not code[marker:start].strip():
            start = marker
    else:
        declared_type = re.match(r'struct (vtable_\w+_type)', text[start:end])
        if declared_type:
            removed_types.add(declared_type[1])
        end = text.find('\n', end)
        assert end >= 0
    spans.append((start, end))
for match in PROTOTYPE.finditer(code):
    if re.search(r'\b(?:' + '|'.join(sorted(selected)) + r')\b', match[0]):
        spans.append(match.span())
last_start = len(text)
for start, end in sorted(spans, reverse=True):
    assert end <= last_start
    text = text[:start] + '/* ISLAND_REMOVED */' + text[end:]
    last_start = start
for name in sorted(removed_types):
    match = re.search(r'^struct ' + name + r'\s*\{[^}]*\};', text, re.M)
    assert match
    if len(re.findall(r'\b' + name + r'\b', mask(text))) == 1:
        text = text[:match.start()] + '/* ISLAND_REMOVED */' + text[match.end():]
    else:
        raise ValueError('Removed table type is still in use: ' + name)
assert not set(re.findall(r'\b(?:g\d+(?:_\w+)?|function_[0-9a-f]+)\b', mask(text))) & selected
text = re.sub(r'(?:\s*/\* ISLAND_REMOVED \*/\s*)+', '\n\n', text)
assert sha(text.encode()) == '2f4b0190bb172a2f09c5b3a0e67ff8efcb28bebbcc6a0c4bb80840173377ef50'
path.write_text(text)
report['after_sha256'] = sha(text.encode())
report['removed_table_types'] = sorted(removed_types)
report['artifacts'] = [dict(id=artifact, variant=variant, sha256=digest) for artifact, variant, digest in ARTIFACTS]
out = Path('docs/legacy-library-audit-20260920/retired-conversion-island.json')
out.write_text(json.dumps(report, indent=2) + '\n')
print('Removed reviewed library island; source SHA256', report['after_sha256'])

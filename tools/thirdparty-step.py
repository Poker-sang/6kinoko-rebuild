# commit: refactor: remove superseded Sqrat field and registration-cache implementations
import io, json, re, subprocess, sys, zipfile
from pathlib import Path
sys.path.insert(0, 'tools')
from audit_legacy_islands import audit, MAIN, NAMED_PROTOTYPE, NAMED_TOKEN
from audit_unused_crt import mask, sha
BASE = '44d1f41c468690263143e89dc3593de67fe69042'
ARTIFACTS = [
 (10589904537, 'quiet', 'b1f1f94d8f4205f93d9a865a09eb0b8998a53852fd0b4dd484e2aaf2a418ab52'),
 (10590264071, 'diagnostic', '3882249142ebb2f253e253ac87077d7e347eee42b24c7177ce2d9df293dc4769')]
seeds = ['function_417420', 'function_43c1d0', 'function_423cf0', 'function_423d60', 'function_42e3d0', 'function_433080', 'function_454c20', 'function_454c80', 'function_454cf0']
maps = []
for artifact, variant, digest in ARTIFACTS:
    local = Path('/mnt/data/thirdparty-callback-' + variant + '.zip')
    data = local.read_bytes() if local.exists() else subprocess.check_output(
        ['gh', 'api', f'repos/Poker-sang/6kinoko-rebuild/actions/artifacts/{artifact}/zip'])
    assert sha(data) == digest
    parent = Path('build-runs/field-cache-retirement-01/evidence') / variant
    parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for name in ['source-commit.txt', 'kinoko.map', 'stage-contract.map']:
            (parent/name).write_bytes(archive.read(f'build-runs/ci-{variant}/{name}'))
            if name.endswith('.map'): maps.append(parent/name)
report = audit(BASE, maps, seeds, include_named_functions=True)
entries = report['candidates']; selected = {e['name'] for e in entries}
assert (report['functions'], report['data']) == (71, 33)
assert sha('\n'.join(sorted(selected)).encode()) == 'c19a8060be1b37ed92e7724526da5e56f48aa24585a60e80669d001241706040'
assert report['outside_interior_literal_references'] == []
assert report['reference_sha256'] == '504d899c97d12700aad88d88edbc072f95c600184b684c0bf15bad7471821014'
path = Path(MAIN); text = path.read_text()
assert sha(text.encode()) == report['source_sha256']
code = mask(text, strings=True)
spans, types = [], set()
for e in entries:
    start, end = e['start'], e['end']
    assert sha(text[start:end].encode()) == e['sha256']
    if e['kind'] == 'function':
        marker = text.rfind('// Address range:', 0, start)
        assert marker >= 0 and not code[marker:start].strip()
        start = marker
    else:
        kind = re.match(r'struct (vtable_\w+_type)', text[start:end])
        if kind: types.add(kind[1])
        end = text.find('\n', end)
        assert end >= 0
    spans.append((start, end))
for match in NAMED_PROTOTYPE.finditer(code):
    if selected & set(NAMED_TOKEN.findall(match[0])):
        assert not any(start <= match.start() < end for start, end in spans)
        spans.append(match.span())
last = len(text)
for start, end in sorted(spans, reverse=True):
    assert end <= last
    text = text[:start] + '/* RETIRED_LIBRARY_REMOVED */' + text[end:]
    last = start
removed_types = []
for name in sorted(types):
    match = re.search(r'^struct ' + re.escape(name) + r'\s*\{[^}]*\};', text, re.M)
    assert match
    if len(re.findall(r'\b' + re.escape(name) + r'\b', mask(text))) == 1:
        text = text[:match.start()] + '/* RETIRED_LIBRARY_REMOVED */' + text[match.end():]
        removed_types.append(name)
assert not set(NAMED_TOKEN.findall(mask(text))) & selected
text = re.sub(r'(?:\s*/\* RETIRED_LIBRARY_REMOVED \*/\s*)+', '\n\n', text)
assert sha(text.encode()) == '696ac4931b10802d51948a6a5210d7e98c58467a70bc00e208ce9e0b8369d854'
path.write_text(text)
report['after_sha256'] = sha(text.encode())
report['removed_table_types'] = removed_types
report['artifacts'] = [dict(id=i, variant=v, sha256=d) for i, v, d in ARTIFACTS]
Path('docs/legacy-library-audit-20260920/superseded-field-and-cache-islands.json').write_text(json.dumps(report, indent=2)+'\n')
print('retired', report['functions'], report['data'], 'post SHA256', report['after_sha256'])

# commit: refactor: remove orphaned locale and exception CRT adapters
import io, json, re, subprocess, sys, zipfile
from pathlib import Path
sys.path.insert(0, 'tools')
from audit_unused_crt import audit, definitions, mask, sha, TARGET
BASE = 'ef6236d0bf1a6f67b0141632446b513c8a4351cb'
ARTIFACTS = [
 (10590171061, 'quiet', 'c3e8ef5cd7358b0406fd814278e47ea8645747ee4f14d7cc2df94c11598fba66'),
 (10590310975, 'diagnostic', '071b9a00c794856234eede62e11601c3084185bd89219cd3712d852e86132834')]
maps = []
for artifact, variant, digest in ARTIFACTS:
    local = Path('/mnt/data/thirdparty-island-' + variant + '.zip')
    data = local.read_bytes() if local.exists() else subprocess.check_output(
        ['gh', 'api', f'repos/Poker-sang/6kinoko-rebuild/actions/artifacts/{artifact}/zip'])
    assert sha(data) == digest
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        parent = Path('build-runs/orphan-crt-evidence') / variant
        parent.mkdir(parents=True, exist_ok=True)
        for name in ['source-commit.txt', 'kinoko.map', 'stage-contract.map']:
            (parent/name).write_bytes(archive.read(f'build-runs/ci-{variant}/{name}'))
            if name.endswith('.map'): maps.append(parent/name)
report = audit(BASE, maps)
assert len(report['candidates']) == 21
path = Path(TARGET); text = path.read_text()
assert sha(text.encode()) == report['implementation_sha256']
names = {e['name']: e for e in report['candidates']}
for entry in reversed(list(definitions(text))):
    if entry['name'] in names:
        body = text[entry['start']:entry['end']]
        assert sha(body.encode()) == names[entry['name']]['body_sha256']
        text = text[:entry['start']] + '/* ORPHAN_REMOVED */' + text[entry['end']:]
helper = next(e for e in definitions(text) if e['name'] == 'retdec_valid_text')
# The remaining apparent reference is only the helper definition itself. All
# outside references and link-map appearances must also be absent at BASE.
kept = next(e for e in report['retained'] if e['name'] == helper['name'])
assert not kept['source_references'] and not kept['linked']
assert len(re.findall(r'\bretdec_valid_text\b', mask(text))) == 1
report['dependency_cleanup'] = dict(name=helper['name'],
    body_sha256=sha(text[helper['start']:helper['end']].encode()),
    old_local_references=kept['local_references'], remaining_local_references=0)
text = text[:helper['start']] + '/* ORPHAN_REMOVED */' + text[helper['end']:]
text = re.sub(r'(?:\s*/\* ORPHAN_REMOVED \*/\s*)+', '\n\n', text)
assert not any(re.search(r'\b'+re.escape(name)+r'\b', mask(text)) for name in names)
assert sha(text.encode()) == '2a2163db260e86fc9c8e30804c2b72e9f84dc42f95355f6bbe467f1930111f04'
path.write_text(text)
report['after_sha256'] = sha(text.encode())
Path('docs/legacy-library-audit-20260920/orphan-crt.json').write_text(json.dumps(report, indent=2)+'\n')
print('Removed 21 orphan entries and their private text validator:', report['after_sha256'])

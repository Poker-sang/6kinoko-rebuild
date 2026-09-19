# commit: refactor: retire unreferenced CRT startup, math and binding-cache implementations
import io, json, re, subprocess, sys, zipfile
from pathlib import Path
sys.path.insert(0, 'tools')
from audit_legacy_islands import audit, MAIN, NAMED_PROTOTYPE, NAMED_TOKEN
from audit_unused_crt import mask, sha
BASE = 'ed9ec1bd4d7844d226dc9bdc107468b0cf6e06ca'
ARTIFACTS = [
 (10590107193, 'quiet', 'bb257259e7d4c8d298c80369520cd9ac7fd9c625eedae4b42769a05ce69a3983'),
 (10590097124, 'diagnostic', '2fb0d951fca2bcd07acedca19412308ee41f7065897274f898fc7d6d838612f4')]
seeds = ['_3f__Incref_40_facet_40_locale_40_std_40__40_QAEXXZ', '_3f__Decref_40_facet_40_locale_40_std_40__40_QAEPAV123_40_XZ', '_3f__3f_1locale_40_std_40__40_QAE_40_XZ', 'function_419d10', 'function_442780', 'function_44d940', '_3f__Tidy_40__3f__24__Mpunct_40_D_40_std_40__40_AAEXXZ', '_3f__CallMemberFunction0_40__40_YGXPAX0_40_Z', '_40___security_check_cookie_40_4', '_3f__3f_1exception_40_std_40__40_UAE_40_XZ', 'entry_point', '__acos_pentium4', '__lockexit', '__unlockexit', '__cexit', '__c_exit', '__initp_misc_purevirt', '__initp_eh_hooks', '__initp_heap_handler', '__encoded_null', '___crtTlsAlloc_40_4', '__initp_misc_invarg', '___iob_func', '__pow_pentium4', '__acos_pentium4_', '__flushall', '___sys_nerr', '___sys_errlist', '___get_sigabrt', '__initp_misc_rand_s', '__acos_pentium4__', '__NLG_Notify1', '__NLG_Call', '__crt_debugger_hook', '__matherr', '_RtlUnwind_40_16', '__acos_pentium4___', '__acos_pentium4____', '__exp_pentium4', '__acos_pentium4_____', 'g240']
maps = []
for artifact, variant, digest in ARTIFACTS:
    local = Path('/mnt/data/thirdparty-control-head-' + variant + '.zip')
    data = local.read_bytes() if local.exists() else subprocess.check_output(
        ['gh', 'api', f'repos/Poker-sang/6kinoko-rebuild/actions/artifacts/{artifact}/zip'])
    assert sha(data) == digest
    parent = Path('build-runs/crt-bindings-retirement-01/evidence') / variant
    parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for name in ['source-commit.txt', 'kinoko.map', 'stage-contract.map']:
            (parent/name).write_bytes(archive.read(f'build-runs/ci-{variant}/{name}'))
            if name.endswith('.map'): maps.append(parent/name)
report = audit(BASE, maps, seeds, include_named_functions=True)
entries = report['candidates']; selected = {e['name'] for e in entries}
assert (report['functions'], report['data']) == (55, 117)
assert sha('\n'.join(sorted(selected)).encode()) == '17c74b9727d866b23c199942d2c19c29d83c039e456de8650a37b4dd50013ae2'
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
assert sha(text.encode()) == '6cdbbcf428b84ff80433b6ce9082de8f1aed3586966fdc8b5afb1129b14a5400'
path.write_text(text)
report['after_sha256'] = sha(text.encode())
report['removed_table_types'] = removed_types
report['artifacts'] = [dict(id=i, variant=v, sha256=d) for i, v, d in ARTIFACTS]
Path('docs/legacy-library-audit-20260920/retired-crt-and-binding-islands.json').write_text(json.dumps(report, indent=2)+'\n')
print('retired', report['functions'], report['data'], 'post SHA256', report['after_sha256'])

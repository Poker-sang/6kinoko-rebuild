# commit: refactor: remove 17 orphan CRT adapters after library retirement
import io,json,re,subprocess,sys,zipfile
from pathlib import Path
sys.path.insert(0,'tools')
from audit_unused_crt import audit,definitions,sha,TARGET
BASE='44d1f41c468690263143e89dc3593de67fe69042'
ARTIFACTS=[(10589904537,'quiet','b1f1f94d8f4205f93d9a865a09eb0b8998a53852fd0b4dd484e2aaf2a418ab52'),(10590264071,'diagnostic','3882249142ebb2f253e253ac87077d7e347eee42b24c7177ce2d9df293dc4769')]
maps=[]
for artifact,variant,digest in ARTIFACTS:
    local=Path('/mnt/data/thirdparty-callback-'+variant+'.zip')
    data=local.read_bytes() if local.exists() else subprocess.check_output(['gh','api',f'repos/Poker-sang/6kinoko-rebuild/actions/artifacts/{artifact}/zip'])
    assert sha(data)==digest
    parent=Path('build-runs/post-callback-crt-evidence')/variant;parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        for name in ['source-commit.txt','kinoko.map','stage-contract.map']:
            (parent/name).write_bytes(z.read(f'build-runs/ci-{variant}/{name}'))
            if name.endswith('.map'):maps.append(parent/name)
r=audit(BASE,maps);assert len(r['candidates'])==17
path=Path(TARGET);text=path.read_text();assert sha(text.encode())==r['implementation_sha256']
names={e['name']:e for e in r['candidates']}
for entry in reversed(list(definitions(text))):
    if entry['name'] in names:
        assert sha(text[entry['start']:entry['end']].encode())==names[entry['name']]['body_sha256']
        text=text[:entry['start']]+'/* REMOVED_CRT */'+text[entry['end']:]
text=re.sub(r'(?:\s*/\* REMOVED_CRT \*/\s*)+','\n\n',text)
assert sha(text.encode()) == '478539a87c452fdc2e6c32de2123b9645c701d2af9c99d157effc450c9e7699e'
path.write_text(text);r['after_sha256']=sha(text.encode())
r['artifacts']=[dict(id=a,variant=v,sha256=d) for a,v,d in ARTIFACTS]
Path('docs/legacy-library-audit-20260920/post-callback-orphan-crt.json').write_text(json.dumps(r,indent=2)+'\n')
print('17 orphan CRT definitions removed',r['after_sha256'])

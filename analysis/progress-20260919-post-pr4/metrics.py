import ast,json,re,subprocess
from pathlib import Path
from collections import Counter
root=Path.cwd(); out=root/'analysis/progress-20260919-post-pr4'
p=root/'analysis/function-inventory-20260918-postmerge/metrics.py'
tree=ast.parse(p.read_text(encoding='utf-8')); nodes=[n for n in tree.body if isinstance(n,(ast.Import,ast.ImportFrom,ast.FunctionDef)) or isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='LEX' for t in n.targets)]
ns={};exec(compile(ast.Module(body=nodes,type_ignores=[]),'metrics','exec'),ns)
mask,defs,lines=ns['mask'],ns['definitions'],ns['code_lines']
cmake=(root/'CMakeLists.txt').read_text(); paths=sorted(set(re.findall(r'\bsrc/[\w/.-]+\.(?:c|cpp)\b',cmake)))
files=[]
for p in paths:
 s=(root/p).read_text(encoding='utf-8'); ds=defs(s)
 files.append(dict(path=p,physical_lines=len(s.splitlines()),code_lines=lines(s),address_definitions=len(ds),address_body_lines=sum(d['lines'] for d in ds),legacy_local_tokens=len(re.findall(r'\bv\d+\b',mask(s))),field_accesses=len(re.findall(r'\bfield\s*<',mask(s)))))
main=(root/'src/decompiled/6kinoko_rebuilt.c').read_text(encoding='utf-8'); ds=defs(main)
history=[]
for rev in ['3cffc80','955aee6','HEAD']:
 s=subprocess.check_output(['git','show',rev+':src/decompiled/6kinoko_rebuilt.c']).decode(); d=defs(s)
 history.append(dict(revision=rev,physical_lines=len(s.splitlines()),code_lines=lines(s),address_definitions=len(d),address_body_lines=sum(x['lines'] for x in d)))
cpp=sum(x['code_lines'] for x in files if x['path'].endswith('.cpp')); c=sum(x['code_lines'] for x in files if x['path'].endswith('.c'))
checks=[]
for p in (root/'third_party/squirrel-2.2.2').rglob('*'):
 if p.is_file():
  rel=p.relative_to(root/'third_party/squirrel-2.2.2'); ref=root.parent/'squirrel-2.2.2/SQUIRREL2'/rel
  if ref.is_file():checks.append(dict(path=rel.as_posix(),same=p.read_bytes()==ref.read_bytes()))
summary=dict(files=files,cpp_code_lines=cpp,c_code_lines=c,cpp_share=round(100*cpp/(cpp+c),2),history=history,largest_remaining=sorted(ds,key=lambda d:d['lines'],reverse=True)[:20],squirrel_comparison=checks)
(out/'metrics.json').write_text(json.dumps(summary,indent=2))
print(json.dumps({k:v for k,v in summary.items() if k not in ('files','squirrel_comparison')},indent=2))
print('Squirrel files:',len(checks),'different:',[x['path'] for x in checks if not x['same']])

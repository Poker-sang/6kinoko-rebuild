import re,json,sys
from pathlib import Path
p=Path('src/decompiled/6kinoko_rebuilt.c');s=p.read_text()
mask=re.sub(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:' '*len(m[0]),s)
defs={}
for m in re.finditer(r'(?m)^int32_t (function_([0-9a-f]+))\([^;{}]*\)\s*\{',mask):
 if not 0x49c9e0<=int(m[2],16)<0x4a15b0:continue
 i=m.end();depth=1
 while depth:
  if mask[i]=='{':depth+=1
  elif mask[i]=='}':depth-=1
  i+=1
 defs[m[1]]=(m.start(),i)
external=mask
for a,b in sorted(defs.values(),reverse=True):external=external[:a]+' '*(b-a)+external[b:]
external=re.sub(r'(?m)^int32_t function_[0-9a-f]+\([^;{}]*\);','',external)
for folder in ('src','include','tests'):
 for other in Path(folder).rglob('*'):
  if other.suffix not in ('.c','.cpp','.h','.hpp'):continue
  if other==p or other.name=='6kinoko.exe.c':continue
  external+='\n'+other.read_text(encoding='utf-8',errors='replace')
roots={n for n in defs if re.search(r'\b'+n+r'\b',external)}
live=set(roots)
while True:
 more={n for parent in live for n in defs if re.search(r'\b'+n+r'\b',mask[slice(*defs[parent])])}
 if more<=live:break
 live|=more
removed=sorted(defs.keys()-live)
audit={'range':'49c9e0 <= address < 4a15b0','external_roots':sorted(roots),'retained':sorted(live),'removed':removed,'basis':'No references from outside candidate bodies, then transitive closure from external roots; includes other src/include/tests files except preserved baseline 6kinoko.exe.c.'}
print(json.dumps(audit,indent=2))
if '--apply' in sys.argv:
 spans=[]
 for n in removed:
  a,b=defs[n];comment=s.rfind('// Address range:',0,a)
  if comment>=0 and not mask[comment:a].strip():a=comment
  spans.append((a,b))
 for a,b in sorted(spans,reverse=True):s=s[:a]+s[b:]
 for n in removed:s=re.sub(r'(?m)^int32_t '+n+r'\([^;{}]*\);\n','',s)
 p.write_text(s)
 Path('analysis/receiver-continuation-20260915/compiler-removal.json').write_text(json.dumps(audit,indent=2))

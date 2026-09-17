import re,json,sys
from pathlib import Path
p=Path('src/decompiled/6kinoko_rebuilt.c');s=p.read_text()
mask=re.sub(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:' '*len(m[0]),s)
defs={}
for m in re.finditer(r'(?m)^(?:static )?int32_t (function_([0-9a-f]+)(?:_legacy)?)\([^;{}]*\)\s*\{',mask):
 i=m.end();depth=1
 while depth:
  if mask[i]=='{':depth+=1
  elif mask[i]=='}':depth-=1
  i+=1
 if m[1] in ('function_489f30','function_4a9d70'):continue
 if re.search(r'\bfunction_(?:489f30|4a9d70)\s*\(',mask[m.end():i]):defs[m[1]]=(m.start(),i)
external=mask
for a,b in sorted(defs.values(),reverse=True):external=external[:a]+' '*(b-a)+external[b:]
external=re.sub(r'(?m)^(?:static )?int32_t function_[\w]+\([^;{}]*\);','',external)
for folder in ('src','include','tests'):
 for other in Path(folder).rglob('*'):
  if other.suffix not in ('.c','.cpp','.h','.hpp'):continue
  if other==p or other.name=='6kinoko.exe.c':continue
  external+='\n'+other.read_text(encoding='utf-8',errors='replace')
tokens=set(re.findall(r'\bfunction_\w+\b|\b0x[0-9a-fA-F]+\b',external))
roots={n for n in defs if n in tokens or '0x'+n.split('_')[1] in tokens}
edges={n:set(re.findall(r'\bfunction_\w+\b',mask[slice(*span)]))&defs.keys() for n,span in defs.items()}
live=set(roots)
while True:
 more=set().union(*(edges[n] for n in live)) if live else set()
 if more<=live:break
 live|=more
removed=sorted(defs.keys()-live)
audit={'candidates':len(defs),'external_roots':sorted(roots),'retained':sorted(live),'removed':removed,'basis':'External symbol/address references plus transitive closure; scanned rebuilt source and other src/include/tests C/C++ files, excluding reference-only 6kinoko.exe.c.'}
print('Candidates',len(defs),'retained',len(live),'removed',len(removed))
if '--apply' in sys.argv:
 spans=[]
 for n in removed:
  a,b=defs[n];comment=s.rfind('// Address range:',0,a)
  if comment>=0 and not mask[comment:a].strip():a=comment
  spans.append((a,b))
 for a,b in sorted(spans,reverse=True):s=s[:a]+s[b:]
 for n in removed:s=re.sub(r'(?m)^(?:static )?int32_t '+n+r'\([^;{}]*\);\n','',s)
 assert len(re.findall(r'\bfunction_489f30\s*\(',s))==2
 s=re.sub(r'(?m)^int32_t function_489f30\(void\);\n','',s)
 start=s.index('// Remaining generated callers have no receiver information.');end=s.index('\n}',start)+2
 assert 'int32_t function_489f30(void)' in s[start:end]
 s=s[:start]+s[end:]
 s=re.sub(r'\n{4,}','\n\n\n',s)
 assert not re.search(r'\bfunction_489f30\b',s)
 p.write_text(s)
 Path('analysis/receiver-continuation-20260915/destructor-removal.json').write_text(json.dumps(audit,indent=2))

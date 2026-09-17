import re,json,sys,subprocess,hashlib
from pathlib import Path
p=Path('src/decompiled/6kinoko_rebuilt.c')
if '--revision' in sys.argv:
 assert '--apply' not in sys.argv, 'Historical input is audit-only'
 revision=sys.argv[sys.argv.index('--revision')+1]
 s=subprocess.check_output(['git','show',revision+':'+p.as_posix()]).decode('utf-8')
else:
 s=p.read_text()
input_sha256=hashlib.sha256(s.encode('utf-8')).hexdigest()
def blank(s):
 return re.sub(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:' '*len(m[0]),s)
m=blank(s);defs={};addresses={}
pattern=r'(?m)^((?:static\s+)?(?:__declspec\([^\n]*?\)\s+)?(?:[\w]+\s+)+\**\s*)(\w+)\([^;{}]*\)\s*\{'
for f in re.finditer(pattern,m):
 a=f.start();i=f.end();depth=1
 while depth:
  if m[i]=='{':depth+=1
  elif m[i]=='}':depth-=1
  i+=1
 name=f[2]
 defs.setdefault(name,[]).append((a,i))
 previous=s.rfind('// Address range:',0,a)
 if previous>=0 and not m[previous:a].strip():
  addr=re.match(r'// Address range: (0x[0-9a-f]+)',s[previous:])[1]
  addresses[name]=addr
# Includes symbol use (calls, callbacks, vtables) and literal original addresses.
to_name={}
for name,addr in addresses.items():to_name.setdefault(addr,set()).add(name)
def references(text):
 words=set(re.findall(r'\b[A-Za-z_]\w*\b',text)) & defs.keys()
 for a in re.findall(r'\b0x[0-9a-fA-F]+\b',text):words |= to_name.get(a.lower(),set())
 return words
edges={n:set().union(*(references(m[a:b]) for a,b in spans))-{n} for n,spans in defs.items()}
seeds={n for n,e in edges.items() if 'function_4a9d70' in e}
ancestors=set(seeds)
while True:
 more={n for n,e in edges.items() if e&ancestors}
 if more<=ancestors:break
 ancestors|=more
external=m
for a,b in sorted((span for spans in defs.values() for span in spans),reverse=True):external=external[:a]+' '*(b-a)+external[b:]
external=re.sub(r'(?m)^[^;{}\n]*\b(?:'+ '|'.join(map(re.escape,defs))+r')\([^;{}]*\);','',external)
for folder in ('src','include','tests'):
 for other in Path(folder).rglob('*'):
  if other.suffix not in ('.c','.cpp','.h','.hpp') or other==p or other.name=='6kinoko.exe.c':continue
  external+='\n'+other.read_text(encoding='utf-8',errors='replace')
roots=references(external)
live=set(roots)
while True:
 more=set().union(*(edges[n] for n in live)) if live else set()
 if more<=live:break
 live|=more
print('definitions',len(defs),'seed callers',len(seeds),'ancestors',len(ancestors),'root reachable',len(live))
print('seed live',sorted(seeds&live))
print('seed dead',sorted(seeds-live))
print('ancestor live',sorted(ancestors&live))
audit={'input_sha256':input_sha256,'basis':'Symbol/address references, external roots, transitive reachability; conditional duplicate definitions and address aliases retained conservatively. Only destructor caller ancestors are removal candidates.','seeds':sorted(seeds),'dead_ancestors':sorted(ancestors-live),'live_ancestors':sorted(ancestors&live),'roots':sorted(roots),'addresses':{n:addresses.get(n) for n in ancestors},'spans':{n:defs[n] for n in ancestors-live}}
Path('analysis/receiver-continuation-20260915/final-destructor-reachability.json').write_text(json.dumps(audit,indent=2))

# Apply only the audited ancestor set, never all globally unreferenced code.
if '--apply' in __import__('sys').argv:
 removed=ancestors-live
 spans=[]
 for name in removed:
  for a,b in defs[name]:
   previous=s.rfind('// Address range:',0,a)
   if previous>=0 and not m[previous:a].strip():a=previous
   spans.append((a,b))
 for a,b in sorted(spans,reverse=True):s=s[:a]+s[b:]
 for name in removed:
  s=re.sub(r'(?m)^[^;{}\n]*\b'+re.escape(name)+r'\([^;{}]*\);\n','',s)
 remaining=blank(s)
 assert not any(re.search(r'\b'+re.escape(name)+r'\b',remaining) for name in removed)
 s=re.sub(r'\n{4,}','\n\n\n',s)
 p.write_text(s)
 print('Removed',len(removed),'definitions / chains')

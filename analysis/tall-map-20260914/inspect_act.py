from pathlib import Path
import struct,json,sys
b=Path(sys.argv[1]).read_bytes(); b=bytes(x^(b[0]^65) for x in b); p=12
def read(n):
 global p
 v=b[p:p+n]; p+=n; assert len(v)==n; return v
def num(f='I'): return struct.unpack('<'+f,read(struct.calcsize('<'+f)))[0]
def string(): return read(num()).decode('cp932',errors='replace')
def props():
 if not num('B'): return {}
 fields=[(string(),num()) for _ in range(num())]
 return {k: string() if t==3 else num({0:'i',1:'f',2:'B'}[t]) for k,t in fields}
def script():
 d=props(); d['raw']=read(num()).decode('cp932',errors='replace'); return d
out={'properties':props(),'script':script(),'layers':[]}
for _ in range(num()):
 assert num()==0x2618cf18
 layer={'properties':props(),'keys':[]}
 for i in range(num()):
  assert num()==0xd933304d
  key={'properties':props()}
  if num('B'):
   t=num(); key['layout']=props()
   if t==0xc9ca5c20:
    n,size=num(),num(); rows=[read(size) for _ in range(n)]
    key['count']=n
    key['records']=[struct.unpack('<Iii',r[:12]) for r in rows] if n<20 else []
  layer['keys'].append(key)
 assert num()==0
 layer['script']=script(); out['layers'].append(layer)
print(json.dumps(out,ensure_ascii=False,indent=2))

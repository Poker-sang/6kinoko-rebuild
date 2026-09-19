import json,urllib.request
from pathlib import Path
out=Path('analysis/remaining-mapping-20260920')
addresses=[0x40a5f0,0x40a8d0,0x40a9a0,0x46b450,0x46f9f0,0x489ef0,0x489f20,0x48c080,0x490040,0x495360,0x49c350,0x4a1760,0x4a1850,0x4a8d60]
code='import idautils, ida_funcs, ida_name, json\naddresses='+repr(addresses)+'''\nprint(json.dumps({hex(a):[{'from':hex(x.frm),'code':bool(x.iscode),'owner':hex(f.start_ea) if (f:=ida_funcs.get_func(x.frm)) else None,'text':ida_name.get_name(x.frm)} for x in idautils.XrefsTo(a)] for a in addresses}))'''
def query(tool,args):
 data={'jsonrpc':'2.0','id':1,'method':'tools/call','params':{'name':tool,'arguments':dict(database='kinoko-final-map-20260920',**args)}}
 return json.load(urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:13337/mcp',data=json.dumps(data).encode(),headers={'Content-Type':'application/json'}),timeout=60))
r=query('py_eval',dict(code=code));(out/'original-xrefs.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
v=r['result'];s=v.get('structuredContent') or json.loads(next(c['text'] for c in v['content'] if c['type']=='text'));x=json.loads(s['stdout']);(out/'xref-summary.json').write_text(json.dumps(x,indent=2));print(json.dumps(x))
for a in ['48a8d0','49c350']:
 r=query('disasm',dict(addr='0x'+a,max_instructions=100));(out/('original-'+a+'-asm.json')).write_text(json.dumps(r,indent=2))

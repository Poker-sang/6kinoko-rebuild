import json,urllib.request
from pathlib import Path
out=Path('analysis/remaining-mapping-20260920')
for addr in ['40a5f0','40a8d0','40a9a0','46b450','46f9f0','489ef0','489f20','48c080','490040','4a1760','4a1850','4a8d60']:
 data={'jsonrpc':'2.0','id':1,'method':'tools/call','params':{'name':'decompile','arguments':{'database':'kinoko-final-map-20260920','addr':'0x'+addr}}}
 r=json.load(urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:13337/mcp',data=json.dumps(data).encode(),headers={'Content-Type':'application/json'}),timeout=60))
 (out/('original-'+addr+'.json')).write_text(json.dumps(r,indent=2),encoding='utf-8')
 v=r.get('result',{}); print(addr, 'error' if v.get('isError') or 'error' in r else 'saved')

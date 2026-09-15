import re,json,csv
from pathlib import Path
root=Path('C:/WorkSpace/6kinoko-rebuild')
out=root/'analysis/function-audit-20260915'
lex=re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
def functions(path):
 s=path.read_text(encoding='utf-8')
 masked=lex.sub(lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]),s)
 result={}
 for m in re.finditer(r'^[^\n;{}]*\b(function_[0-9a-f]+)\s*\([^;{}]*\)\s*\{',masked,re.M):
  start=m.end(); depth=1; end=start
  while depth and end<len(masked):
   depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
  body=s[start:end-1]
  body=lex.sub(lambda x: '' if x[0].startswith(('//','/*')) else x[0],body)
  norm=''.join(re.findall(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s]',body))
  result.setdefault(m[1],[]).append(dict(body=norm,line=s.count('\n',0,m.start())+1))
 return result
original=functions(root/'src/decompiled/6kinoko.exe.c'); current=functions(root/'src/decompiled/6kinoko_rebuilt.c')
rows=[]
for name,ob in original.items():
 old=ob[0]['body']; variants=current.get(name,[])
 # Preserve every conditional variant for this source inventory. An implemented
 # x86 branch must not be counted as missing because a portable fallback is empty.
 same=any(x['body']==old for x in variants)
 nontrivial=[x for x in variants if not re.fullmatch(r'(?:return(?:0|1|NULL|a\d+|source_ptr)?;)?',x['body'])]
 if not variants: cat='未找到同名入口，待核对替代'
 elif same and re.fullmatch(r'(?:return[^;{}]*;)?',old) and len(old)<80:cat='原版即为简单返回／空函数'
 elif same:cat='保留原反编译函数体'
 elif nontrivial:cat='已修改／替换的非空实现'
 else:cat='简化返回／占位候选，待核对'
 rows.append(dict(function=name,category=cat,original_line=ob[0]['line'],current_line=variants[0]['line'] if variants else '',variants=len(variants)))
# Confirmed replacements for every formerly missing exact-name entry.
replacement_groups={
 'src/reconstructed/squirrel_object.cpp':['403e50'],
 'src/reconstructed/act_clone.cpp':['427950'],
 'src/reconstructed/sprite_color.cpp':['42b280','42b2a0','42b2d0'],
 'src/reconstructed/map_render.cpp':['434b40','434b60','434f40','46eed0'],
 'src/reconstructed/actor_methods.cpp':['45dbc0','45f760','45f780','45f7a0','45f7e0','45f800','45f810','45f820'],
 'src/reconstructed/actor_animation.cpp':['45fe80'],
 'src/reconstructed/script_callbacks.cpp':['45fb90','45fcd0','45fd80','4663c0','466470'],
 'src/squirrel/squirrel_array_bridge.cpp; analysis/crystal-bgm-20260913/report.md':['48dac0'],
 'src/decompiled/6kinoko_rebuilt.c explicit _this entry':['462280','4665e0','46f9f0','48c080','49c350','4a8d60'],
}
replacement_map={'function_'+addr:evidence for evidence,addrs in replacement_groups.items() for addr in addrs}
for row in rows:
 row['replacement_evidence']=''
 if row['category']=='未找到同名入口，待核对替代' and row['function'] in replacement_map:
  row['category']='已确认改名／跨文件替代'
  row['replacement_evidence']=replacement_map[row['function']]
from collections import Counter
counts=Counter(r['category'] for r in rows)
result=dict(total=len(rows),categories=[dict(label=k,count=v,percent=round(v/len(rows)*100,2)) for k,v in counts.items()],method='Source inventory by original function address. Comments and whitespace ignored; all conditional variants checked. Not semantic equivalence or runtime reachability. External CRT/instruction helpers excluded.')
(out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
with (out/'functions.csv').open('w',encoding='utf-8-sig',newline='') as f:
 w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
print(json.dumps(result,ensure_ascii=False,indent=2))
print('candidates',[r for r in rows if r['category']=='简化返回／占位候选，待核对'][:20])


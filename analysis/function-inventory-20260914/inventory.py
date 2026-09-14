import re,json,csv
from pathlib import Path
root=Path('C:/WorkSpace/6kinoko-rebuild')
out=root/'analysis/function-inventory-20260914'
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
from collections import Counter
counts=Counter(r['category'] for r in rows)
result=dict(total=len(rows),categories=[dict(label=k,count=v,percent=round(v/len(rows)*100,2)) for k,v in counts.items()],method='Source inventory by original function address. Comments and whitespace ignored; all conditional variants checked. Not semantic equivalence or runtime reachability. External CRT/instruction helpers excluded.')
(out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
with (out/'functions.csv').open('w',encoding='utf-8-sig',newline='') as f:
 w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
print(json.dumps(result,ensure_ascii=False,indent=2))
print('candidates',[r for r in rows if r['category']=='简化返回／占位候选，待核对'][:20])

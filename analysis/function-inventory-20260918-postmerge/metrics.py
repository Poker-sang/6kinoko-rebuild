"""Reproducible source-shape metrics; not a reachability or equivalence proof."""
import re, json, csv, subprocess, bisect
from pathlib import Path
from collections import Counter

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
LEX = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
def mask(s):
    return LEX.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), s)
def code_lines(s):
    return sum(bool(x.strip()) for x in mask(s).splitlines())
def definitions(s):
    masked = mask(s)
    newlines = [m.start() for m in re.finditer('\n', s)]
    result = []
    for m in re.finditer(r'^[^\n;{}]*\b(function_[0-9a-f]+(?:_\w+)?)\s*\([^;{}]*\)\s*\{', masked, re.M):
        end = m.end(); depth = 1
        while depth and end < len(masked):
            depth += (masked[end] == '{') - (masked[end] == '}'); end += 1
        body = masked[m.end():end-1]
        result.append(dict(name=m[1], line=bisect.bisect_left(newlines,m.start())+1,
                           lines=sum(bool(x.strip()) for x in body.splitlines()),
                           tokens=len(re.findall(r'\w+|[^\s]',body))))
    return result

cmake = (ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
paths = sorted(set(re.findall(r'\bsrc/[\w/.-]+\.(?:c|cpp)\b', cmake)))
files = []
for rel in paths:
    s = (ROOT/rel).read_text(encoding='utf-8')
    files.append(dict(path=rel, lines=len(s.splitlines()), code_lines=code_lines(s),
                      address_definitions=len(definitions(s)),
                      inline_asm=len(re.findall(r'\b__asm\s*\{',mask(s)))))
main = (ROOT/'src/decompiled/6kinoko_rebuilt.c').read_text(encoding='utf-8')
defs = definitions(main)
rows = list(csv.DictReader((OUT/'functions.csv').open(encoding='utf-8-sig')))
main_tokens = Counter(re.findall(r'\bfunction_[0-9a-f]+\b',mask(main)))
candidates = [r['function'] for r in rows if r['category'].startswith('简化返回')]
candidate_counts = {name:main_tokens[name] for name in candidates}
cpp_defs = {}
for row in files:
    if row['path'].endswith('.cpp'):
        for d in definitions((ROOT/row['path']).read_text(encoding='utf-8')):
            cpp_defs.setdefault(re.match(r'function_[0-9a-f]+',d['name'])[0],[]).append(dict(file=row['path'],**d))
missing = [r['function'] for r in rows if r['category'].startswith('未找到')]
aliases = {}
for m in re.finditer(r'^\s*#define\s+(function_[0-9a-f]+)\s+([^\n]+)',main,re.M):
    aliases[m[1]] = m[2]
explicit_this = {re.match(r'function_[0-9a-f]+', d['name'])[0] for d in defs if '_this' in d['name']}
deleted = set()
for name in ['compiler-removal.json','compiler-additional-removal.json','destructor-removal.json','final-destructor-reachability.json']:
    data=json.loads((ROOT/'analysis/receiver-continuation-20260915'/name).read_text(encoding='utf-8-sig'))
    for key in ['removed','deleted']:
        for item in (data if isinstance(data,list) and key=='removed' else data.get(key,[]) if isinstance(data,dict) else []):
            if isinstance(item,str): deleted.add(item)
            elif isinstance(item,dict) and isinstance(item.get('removed'),str): deleted.add(item['removed'])
missing_breakdown=Counter()
for n in missing:
    if n in cpp_defs: missing_breakdown['explicit_cpp_definition']+=1
    elif n in explicit_this: missing_breakdown['explicit_this_in_c']+=1
    elif n in deleted: missing_breakdown['historical_removal_manifest']+=1
    else: missing_breakdown['needs_full_replacement_mapping']+=1
project_tokens=Counter()
for f in files:
    project_tokens.update(re.findall(r'\bfunction_[0-9a-f]+\b',mask((ROOT/f['path']).read_text(encoding='utf-8'))))
source_check=[]
reference=ROOT.parent/'squirrel-2.2.2/SQUIRREL2'
for path in (ROOT/'third_party/squirrel-2.2.2').rglob('*'):
    if path.is_file():
        rel=path.relative_to(ROOT/'third_party/squirrel-2.2.2')
        if (reference/rel).is_file():
            source_check.append(dict(path=str(rel),same=path.read_bytes()==(reference/rel).read_bytes()))
history=[]
for revision in ['d807bd0','06ad4bf','0f92ecb','3cffc80']:
    s=subprocess.check_output(['git','show',revision+':src/decompiled/6kinoko_rebuilt.c'],cwd=ROOT).decode('utf-8')
    ds=definitions(s)
    history.append(dict(revision=revision,physical_lines=len(s.splitlines()),code_lines=code_lines(s),
                        address_definitions=len(ds),function_body_code_lines=sum(d['lines'] for d in ds)))
cpp=sum(f['code_lines'] for f in files if f['path'].endswith('.cpp'))
c=sum(f['code_lines'] for f in files if f['path'].endswith('.c'))
summary=dict(revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
             source_units=files,cpp_code_lines=cpp,c_code_lines=c,cpp_source_share=round(cpp/(c+cpp)*100,2),
             history=history,remaining_address_definitions=len(defs),
             missing_with_cpp_address_definition={n:cpp_defs[n] for n in missing if n in cpp_defs},
             missing_with_macro_alias={n:aliases[n] for n in missing if n in aliases},
             simplified_candidate_token_counts=candidate_counts,
             simplified_candidate_project_frequency=dict(Counter(project_tokens[n] for n in candidates)),
             missing_breakdown=dict(missing_breakdown),squirrel_reference_comparison=source_check,
             largest_remaining=sorted(defs,key=lambda x:x['lines'],reverse=True)[:25],
             limits='CMake explicit project source units; excludes headers, third party and textual includes. Physical source is not linked/live coverage. Function definition regex is address-named only. C++ source share is not maintainability completion.')
(OUT/'metrics.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in summary.items() if k not in ['source_units','missing_with_cpp_address_definition','missing_with_macro_alias','simplified_candidate_token_counts']},ensure_ascii=False,indent=2))
print('Missing with explicit CPP definitions:',len(summary['missing_with_cpp_address_definition']))
print('Missing with aliases:',len(summary['missing_with_macro_alias']))
print('Simplified candidate source token frequency:',dict(Counter(candidate_counts.values())))

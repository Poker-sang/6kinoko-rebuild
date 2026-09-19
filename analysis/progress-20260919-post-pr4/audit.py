"""Static recovery inventory. No build, execution, or equivalence claim."""
import ast
import csv
import hashlib
import json
import re
import subprocess
import urllib.request
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
PREVIOUS = ROOT / 'analysis/function-inventory-20260918-postmerge'

# Reuse the previous lexical parser without executing its output-writing code.
tree = ast.parse((PREVIOUS / 'inventory.py').read_text(encoding='utf-8'))
nodes = [n for n in tree.body if isinstance(n, (ast.Import, ast.ImportFrom, ast.FunctionDef))
         or isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'lex' for t in n.targets)]
ns = {}
exec(compile(ast.Module(body=nodes, type_ignores=[]), '<inventory-parser>', 'exec'), ns)
functions, lex = ns['functions'], ns['lex']

def mask(s):
    return lex.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), s)

def defs(s):
    return list(re.finditer(r'^[^\n;{}]*\b(function_[0-9a-f]+(?:_\w+)?)\s*\([^;{}]*\)\s*\{', mask(s), re.M))

original = functions(ROOT / 'src/decompiled/6kinoko.exe.c')
current = functions(ROOT / 'src/decompiled/6kinoko_rebuilt.c')
main = (ROOT / 'src/decompiled/6kinoko_rebuilt.c').read_text(encoding='utf-8')
cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
paths = sorted(set(re.findall(r'\bsrc/[\w/.-]+\.(?:c|cpp)\b', cmake)))
sources = {p: (ROOT / p).read_text(encoding='utf-8') for p in paths}
cpp = {}
for p, s in sources.items():
    if p.endswith('.cpp'):
        for m in defs(s):
            cpp.setdefault(re.match(r'function_[0-9a-f]+', m[1])[0], []).append(p + ':' + str(s.count('\n', 0, m.start()) + 1))
explicit_this = {re.match(r'function_[0-9a-f]+', m[1])[0] for m in defs(main) if '_this' in m[1]}
deleted = set()
for filename in ['compiler-removal.json', 'compiler-additional-removal.json', 'destructor-removal.json', 'final-destructor-reachability.json']:
    data = json.loads((ROOT / 'analysis/receiver-continuation-20260915' / filename).read_text(encoding='utf-8-sig'))
    for key in ['removed', 'deleted']:
        items = data if isinstance(data, list) and key == 'removed' else data.get(key, []) if isinstance(data, dict) else []
        for item in items:
            if isinstance(item, str): deleted.add(item)
            elif isinstance(item, dict) and isinstance(item.get('removed'), str): deleted.add(item['removed'])

pr4_deleted = {x['name'] for x in json.loads((ROOT / 'docs/no-inline-asm-20260919/removed-definitions.json').read_text())['definitions']}
rows = []
for name, bodies in original.items():
    old = bodies[0]['body']
    variants = current.get(name, [])
    same = any(v['body'] == old for v in variants)
    if not variants:
        category = ('cpp_address_replacement' if name in cpp else 'explicit_receiver_in_c' if name in explicit_this
                    else 'pr4_disconnected_removal' if name in pr4_deleted else 'historical_removal_manifest' if name in deleted else 'replacement_mapping_unresolved')
    elif same and re.fullmatch(r'(?:return[^;{}]*;)?', old) and len(old) < 80:
        category = 'original_short_body'
    elif same:
        category = 'unchanged_decompiled_body'
    elif any(not re.fullmatch(r'(?:return(?:0|1|NULL|a\d+|source_ptr)?;)?', v['body']) for v in variants):
        category = 'modified_nonempty_body'
    else:
        category = 'simplified_fragment_candidate'
    rows.append(dict(function=name, category=category, original_line=bodies[0]['line'],
                     current_line=variants[0]['line'] if variants else '', cpp_locations=';'.join(cpp.get(name, []))))
with (OUT / 'functions.csv').open('w', encoding='utf-8-sig', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)

stubs = re.findall(r'\bX\((__asm_\w+)\)', (ROOT / 'src/decompiled/retdec_asm_stubs.h').read_text())
stub_rows = []
masked_sources = {p: mask(s) for p, s in sources.items()}
for stub in stubs:
    hits = []
    for p, s in sources.items():
        clean = masked_sources[p]
        for m in re.finditer(r'\b' + re.escape(stub) + r'\s*\(', clean):
            hits.append(p + ':' + str(s.count('\n', 0, m.start()) + 1))
    stub_rows.append(dict(name=stub, source_occurrences=len(hits), locations=hits))
(OUT / 'instruction-stubs.json').write_text(json.dumps(stub_rows, indent=2), encoding='utf-8')

summary = dict(revision=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
    total=len(rows), categories=dict(Counter(r['category'] for r in rows)),
    instruction_stub_definitions=len(stubs), instruction_stubs_with_source_occurrences=sum(bool(r['source_occurrences']) for r in stub_rows),
    instruction_stub_source_occurrences=sum(r['source_occurrences'] for r in stub_rows),
    main_lines=len(main.splitlines()), main_inline_assembly=len(re.findall(r'\b__asm\s*\{', mask(main))),
    source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},
    limitations='Address-entry source classification, not recovered-feature counts. No preprocessor, link reachability or dynamic coverage analysis. Stub occurrences are lexical, not proven live calls.')

# Optional read-only original IDA ownership check; do not reopen or mutate the database.
import sys
if len(sys.argv) > 1:
    code = ('import ida_funcs, json\n' + 'addresses=' + repr([int(r['function'][9:], 16) for r in rows]) + '\n'
            + "print(json.dumps([{'address': hex(a), 'owner': hex(f.start_ea) if (f := ida_funcs.get_func(a)) else None, 'is_start': bool(f and f.start_ea == a)} for a in addresses]))")
    payload = dict(jsonrpc='2.0', id=1, method='tools/call', params=dict(name='py_eval', arguments=dict(database=sys.argv[1], code=code)))
    request = urllib.request.Request('http://127.0.0.1:13337/mcp', data=json.dumps(payload).encode(), headers={'Content-Type':'application/json'})
    response = json.load(urllib.request.urlopen(request, timeout=60))
    (OUT/'ida-ownership-response.json').write_text(json.dumps(response, indent=2), encoding='utf-8')
    result = response['result']
    if result.get('isError'): raise RuntimeError(result)
    structured = result.get('structuredContent') or json.loads(next(c['text'] for c in result['content'] if c['type']=='text'))
    download = result.get('_meta', {}).get('ida_mcp', {}).get('download_url')
    if download:
        structured = json.load(urllib.request.urlopen(download, timeout=30))
        (OUT/'ida-full.json').write_text(json.dumps(structured, indent=2), encoding='utf-8')
    owners = json.loads(structured['stdout'])
    owner_by_address = {int(o['address'],16): o for o in owners}
    summary['ida_entry_kind_by_category'] = {}
    for row in rows:
        owner = owner_by_address[int(row['function'][9:],16)]
        kind = 'function_start' if owner['is_start'] else 'internal_fragment' if owner['owner'] else 'no_function_owner'
        counts = summary['ida_entry_kind_by_category'].setdefault(row['category'], {})
        counts[kind] = counts.get(kind,0)+1
    (OUT/'ida-ownership.json').write_text(json.dumps(owners, indent=2), encoding='utf-8')
(OUT / 'summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
print(json.dumps({k:v for k,v in summary.items() if k != 'source_sha256'}, indent=2))

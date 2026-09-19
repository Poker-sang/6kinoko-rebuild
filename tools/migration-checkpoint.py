"""Apply the reviewed direct Squirrel API migration to the pinned source.
Temporary reproducible transport, removed after the content hash checks.
"""
import hashlib, subprocess
from pathlib import Path
import re
LEX = re.compile(r'R"([A-Za-z0-9_]*)\([\s\S]*?\)\1"|//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
def mask(s): return LEX.sub(lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]), s)
def matching(s, pos, left='(', right=')'):
    depth=0
    for i in range(pos,len(s)):
        if s[i]==left: depth+=1
        elif s[i]==right:
            depth-=1
            if not depth:return i
    raise ValueError(pos)
def args(s,start,end,m=None):
    out=[]; depth=0; last=start; m=mask(s) if m is None else m
    for i in range(start,end):
        c=m[i]
        if c in '([{': depth+=1
        elif c in ')]}': depth-=1
        elif c==',' and not depth:out.append(s[last:i].strip()); last=i+1
    if last<end:out.append(s[last:end].strip())
    return out

def funcs(s):
    m=mask(s); depth=0; ranges=[]
    for b in re.finditer(r'[{}]',m):
        if b[0]=='{':
            if depth==0:
                endpar=b.start()-1
                while endpar>=0 and m[endpar].isspace(): endpar-=1
                if endpar>=0 and m[endpar]==')':
                    d=1; j=endpar-1
                    while d:
                        if m[j]==')':d+=1
                        elif m[j]=='(':d-=1
                        j-=1
                    prefix_start=max(0,j-160)
                    match=re.search(r'\b(\w+)\s*$',m[prefix_start:j+1])
                    if match:
                        name=match[1]; name_start=prefix_start+match.start(); start=m.rfind('\n',0,name_start)+1
                        while start and not s[start:name_start].strip():
                            start=m.rfind('\n',0,start-1)+1
                        if not m[start:name_start].strip():
                            prev=m.rfind('\n',0,start-1)+1; start=prev
                        current=(name,start,b.start(),j+1,endpar)
                    else: current=None
                else:current=None
            depth+=1
        else:
            depth-=1
            if depth==0 and current:
                name,start,body,params,endpar=current
                ranges.append(dict(name=name,start=start,body=body,end=b.end(),signature=s[start:body].strip(),lines=s.count('\n',start,b.end())+1,line=s.count('\n',0,start)+1))
    return ranges

assert hashlib.sha256(Path('src/decompiled/6kinoko_rebuilt.c').read_bytes()).hexdigest() == '1cc66cd79920e04c6938fd5ae6a209e864847db51e4d689c2fd7e2c692d1d15a'
ROOT=Path.cwd()
api=ROOT/'src/squirrel/squirrel_legacy_api.cpp'
V=lambda x:f'kinoko_vm({x})'
P=lambda x:f'kinoko_pointer({x})'
I=lambda x:f'(int32_t)(intptr_t)({x})'
B=lambda x:f'(({x}) != 0)'
M={}
def add(n,f,void=False):M['function_'+n]=(f,void)
# Preserve receiver scopes and stack-underflow policy.
add('48a230',lambda a:f'kinoko_sq_create_thread({a[0]}, {a[1]})')
add('48ace0',lambda a:f'kinoko_sq_call({", ".join(a)})')
add('48aa30',lambda a:f'kinoko_sq_pop({", ".join(a)})')
add('48aa50',lambda a:f'kinoko_sq_pop({a[0]}, 1)')
add('491880_this',lambda a:f'kinoko_sq_get_up({", ".join(a)})')
add('4918a0_this',lambda a:f'kinoko_sq_get_at({", ".join(a)})')
for n,sq in [('48a2d0','getvmstate'),('48aa20','gettop'),('48ada0','suspendvm'),('48c620','setroottable'),('48c690','setconsttable')]:
    add(n,lambda a,sq=sq:f'sq_{sq}({V(a[0])})')
for n,sq in [('48a6f0','gettype'),('48b1a0','setattributes'),('48b320','getattributes'),('48b410','getclass'),('48b490','createinstance'),('48b5a0','getweakrefval'),('48b630','next'),('48c400','arrayreverse'),('48c700','getsize'),('48cb10','rawset'),('48cc70','setdelegate'),('48cd50','getdelegate'),('48ce00','get'),('48ce70','rawget'),('48d7e0','arrayappend')]:
    add(n,lambda a,sq=sq:f'sq_{sq}({V(a[0])}, {a[1]})')
for n,sq in [('48a300','seterrorhandler'),('48a370','setdebughook'),('48a460','pushnull'),('48a600','newtable'),('48a670','pushroottable'),('48a690','pushregistrytable'),('48ac70','reseterror'),('48acc0','getlasterror'),('48c1c0','close')]:
    add(n,lambda a,sq=sq:f'sq_{sq}({V(a[0])})',True)
for n,sq in [('48a4f0','pushinteger'),('48a6b0','push'),('48a720','tostring'),('48aa60','remove'),('48b510','weakref'),('48d770','newarray')]:
    add(n,lambda a,sq=sq:f'sq_{sq}({V(a[0])}, {a[1]})',True)
add('48a3e0',lambda a:f'sq_enabledebuginfo({V(a[0])}, {B(a[1])})',True)
add('48a480',lambda a:f'sq_pushstring({V(a[0])}, (const SQChar*){P(a[1])}, {a[2]})',True)
add('48a530',lambda a:f'sq_pushbool({V(a[0])}, {B(a[1])})',True)
add('48a580',lambda a:f'sq_pushfloat({V(a[0])}, kinoko_float_bits({a[1]}))',True)
add('48a5c0',lambda a:f'sq_pushuserpointer({V(a[0])}, {P(a[1])})',True)
add('48ab90',lambda a:f'sq_pushobject({V(a[0])}, kinoko_borrowed_object({a[1]}, {a[2]}))',True)
add('48abe0',lambda a:f'sq_resetobject((HSQOBJECT*){P(a[0])})',True)
add('48a790',lambda a:f'sq_tobool({V(a[0])}, {a[1]}, (SQBool*)({a[2]}))',True)
for n,sq,t in [('48a7d0','getinteger','SQInteger'),('48a830','getfloat','SQFloat'),('48a890','getbool','SQBool'),('48a8d0','getstring','const SQChar*'),('48a9e0','getuserpointer','SQUserPointer'),('48ab40','getstackobj','HSQOBJECT')]:
    add(n,lambda a,sq=sq,t=t:f'sq_{sq}({V(a[0])}, {a[1]}, ({t}*)({a[2]}))')
add('48a920',lambda a:f'sq_getuserdata({V(a[0])}, {a[1]}, (SQUserPointer*)({a[2]}), (SQUserPointer*){P(a[3])})')
add('48a980',lambda a:f'sq_getobjtypetag((HSQOBJECT*){P(a[0])}, (SQUserPointer*){P(a[1])})')
add('48ac00',lambda a:f'sq_throwerror({V(a[0])}, {a[1]})')
add('48adb0',lambda a:f'sq_wakeupvm({V(a[0])}, {B(a[1])}, {B(a[2])}, {B(a[3])})')
add('48af30',lambda a:f'sq_setreleasehook({V(a[0])}, {a[1]}, (SQRELEASEHOOK){P(a[2])})',True)
add('48afa0',lambda a:f'sq_setcompilererrorhandler({V(a[0])}, (SQCOMPILERERROR){P(a[1])})',True)
add('48afc0',lambda a:f'sq_writeclosure({V(a[0])}, (SQWRITEFUNC){P(a[1])}, {P(a[2])})')
add('48b050',lambda a:f'sq_readclosure({V(a[0])}, (SQREADFUNC){P(a[1])}, {a[2]})')
add('48b160',lambda a:I(f'sq_getscratchpad({V(a[0])}, {a[1]})'))
add('48b870',lambda a:f'sq_move({V(a[0])}, {V(a[1])}, {a[2]})',True)
add('48b8b0',lambda a:f'sq_setprintfunc({V(a[0])}, (SQPRINTFUNCTION){P(a[1])})',True)
add('48b8f0',lambda a:I(f'sq_malloc((SQUnsignedInteger)({a[0]}))'))
add('48c2f0',lambda a:I(f'sq_newuserdata({V(a[0])}, {a[1]})'))
add('48c350',lambda a:f'sq_newclass({V(a[0])}, {B(a[1])})')
add('48c580',lambda a:f'sq_setnativeclosurename({V(a[0])}, {a[1]}, (const SQChar*){P(a[2])})')
for n,sq in [('48c780','settypetag'),('48c840','setinstanceup')]:
    add(n,lambda a,sq=sq:f'sq_{sq}({V(a[0])}, {a[1]}, {P(a[2])})')
add('48c7f0',lambda a:f'sq_gettypetag({V(a[0])}, {a[1]}, (SQUserPointer*){P(a[2])})')
add('48c890',lambda a:f'sq_getinstanceup({V(a[0])}, {a[1]}, (SQUserPointer*)({a[2]}), {P(a[3])})')
add('48c910',lambda a:f'sq_settop({V(a[0])}, (SQInteger)({a[1]}))',True)
for n,sq in [('48c950','newslot'),('48ca10','deleteslot'),('48aa80','rawdeleteslot'),('48dd10','arraypop')]:
    add(n,lambda a,sq=sq:f'sq_{sq}({V(a[0])}, {a[1]}, {B(a[2])})')
add('48d0b0',lambda a:f'sq_compilebuffer({V(a[0])}, (const SQChar*){P(a[1])}, {a[2]}, (const SQChar*)({a[3]}), {B(a[4])})')
add('48d850',lambda a:f'sq_newclosure({V(a[0])}, (SQFUNCTION){P(a[1])}, {a[2]})',True)
add('48dda0',lambda a:f'sq_setparamscheck({V(a[0])}, {a[1]}, (const SQChar*){P(a[2])})')
for n,sq in [('4c7c90','iolib'),('4c73a0','bloblib'),('4c6c20','mathlib'),('4c6670','stringlib')]:
    add(n,lambda a,sq=sq:f'sqstd_register_{sq}({V(a[0])})')
add('4c5c80',lambda a:f'sqstd_seterrorhandlers({V(a[0])})',True)
pat=re.compile(r'\b('+'|'.join(M)+r')\s*\(')
stats={}; unresolved=[]
def rewrite_argument(text):
    masked=mask(text); replacements=[]
    for hit in pat.finditer(masked):
        fn,only_ignored=M[hit[1]]
        if only_ignored:continue
        start=hit.start(); left=masked.find('(',start); end=matching(masked,left)
        if replacements and start<replacements[-1][1]:continue
        parts=[rewrite_argument(a) for a in args(text,left+1,end,masked)]
        replacements.append((start,end+1,fn(parts)))
        stats[hit[1]]=stats.get(hit[1],0)+1
    for start,end,replacement in reversed(replacements):text=text[:start]+replacement+text[end:]
    return text
for p in sorted((ROOT/'src').rglob('*')):
    if p.suffix not in ('.cpp','.c') or p in (api,ROOT/'src/decompiled/6kinoko.exe.c'):continue
    s=p.read_text();m=mask(s); edits=[]
    for hit in pat.finditer(m):
        start=hit.start(); left=m.find('(',start); end=matching(m,left); name=hit[1]
        if edits and start<edits[-1][1]:continue
        line_start=m.rfind('\n',0,start)+1
        prefix=m[line_start:start].strip()
        if re.fullmatch(r'(?:extern\s+\s*)?(?:static\s+)?(?:int32_t|void|int|SQInteger)\s*(?:__fastcall\s*)?',prefix):continue
        nextpos=re.compile(r'\s*').match(m,end+1).end(); tail=m[nextpos:nextpos+1]
        if tail=='{':continue
        segment=m[max(m.rfind(';',0,start),m.rfind('{',0,start),m.rfind('}',0,start))+1:start].strip()
        ignored=tail==';' and (not prefix or prefix=='(void)' or not segment or segment=='(void)')
        fn,only_ignored=M[name]
        if only_ignored and not ignored:
            unresolved.append((str(p.relative_to(ROOT)),s.count('\n',0,start)+1,name,prefix));continue
        a=[rewrite_argument(part) for part in args(s,left+1,end,m)]
        edits.append((start,end+1,fn(a)))
        stats[name]=stats.get(name,0)+1
    for one,two in zip(edits,edits[1:]):assert one[1]<=two[0],(p,one,two)
    if edits:
        for start,end,text in reversed(edits):s=s[:start]+text+s[end:]
        s='#include "kinoko/squirrel_api_types.h"\n'+s
        p.write_text(s)
sources=[p for d in ('src','tests','include') for p in (ROOT/d).rglob('*') if p.suffix in ('.c','.cpp','.h','.hpp') and p.name not in ('6kinoko.exe.c','squirrel_legacy_api.cpp','squirrel_legacy_api.h')]
all='\n'.join(mask(p.read_text()) for p in sources)
references=set(re.findall(r'\bfunction_[a-z0-9_]+\b',all))
removed=[];s=api.read_text()
for f in reversed(funcs(s)):
    if not f['name'].startswith('function_'):continue
    if f['name'] in references:continue
    s=s[:f['start']]+s[f['end']:];removed.append(f['name'])
s=re.sub(r'\n{3,}','\n\n',s);api.write_text(s)
h=ROOT/'include/kinoko/squirrel_legacy_api.h';text=h.read_text()
for n in removed:text=re.sub(r'(?m)^int32_t '+n+r'\([\s\S]*?\);\n','',text)
h.write_text(text)
print('Replaced',sum(stats.values()),'calls across',len(stats),'entry names; removed',len(removed),'unreferenced source-backed ABI definitions')
# Exact outputs verified locally before this checkpoint.
CHECKS={'include/kinoko/squirrel_legacy_api.h':'d05ddc1879ca04685e68f47302496811549bec5b5ffb7a96f45b7a92ff661297','src/decompiled/6kinoko_rebuilt.c':'7f28908e3b77102324736a32b6e09d12883d105b1b1cb008595be1782ce97bfe','src/reconstructed/act_lifetime.cpp':'51c8f8d2ed1a9e0c9c546bf6c028d4a85e1d8840ac3589c4fc41efb117dd09c0','src/squirrel/act_binding.cpp':'f9d2d9f637bff9f6e298eee571985057cceacd364a7b77e03d8caeb257e22917','src/squirrel/squirrel_csv.cpp':'698c61f271e1c9f2b5ffca5dff7c3f9e226b4db3cb10c7403f898fa309ae3e6f','src/squirrel/squirrel_legacy_api.cpp':'44d86b0205cf4209afccaa40ff52068ee144a35b9b336cdac0bce33a4dadb248','src/squirrel/squirrel_vm_bootstrap.cpp':'035d215511f9103733a28e59117db54637c4ceb630552e3eddc1c24981a2b565'}
for name, expected in CHECKS.items():
    assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==expected,name
p=Path('CMakeLists.txt');s=p.read_text()
s=s.replace('option(KINOKO_RETDEC_DISABLE_TRACE','# Generated C callers use the public API; only representation conversions\n# remain in squirrel_api_types.h. Do not expose private VM layouts to C.\ntarget_include_directories(kinoko_retdec_rebuild PRIVATE "${KINOKO_SQUIRREL2_ROOT}/include")\ntarget_include_directories(kinoko_native_methods PRIVATE "${KINOKO_SQUIRREL2_ROOT}/include")\noption(KINOKO_RETDEC_DISABLE_TRACE',1)
s=s.replace('add_executable(kinoko_legacy_method_entries_contract tests/legacy_method_entries_contract.cpp)','add_executable(kinoko_squirrel_c_api_contract\n    tests/squirrel_c_api_contract.c tests/squirrel_c_api_driver.cpp)\ntarget_include_directories(kinoko_squirrel_c_api_contract PRIVATE include\n    "${KINOKO_SQUIRREL2_ROOT}/include")\ntarget_link_libraries(kinoko_squirrel_c_api_contract PRIVATE kinoko_squirrel_cpp_vm)\ntarget_link_options(kinoko_squirrel_c_api_contract PRIVATE /OPT:REF /OPT:ICF)\nadd_test(NAME squirrel_c_api_contract COMMAND kinoko_squirrel_c_api_contract)\nset_tests_properties(squirrel_c_api_contract PROPERTIES TIMEOUT 60)\n\nadd_executable(kinoko_legacy_method_entries_contract tests/legacy_method_entries_contract.cpp)',1)
s=s.replace('set(KINOKO_TOOL_TARGETS\n','set(KINOKO_TOOL_TARGETS\n    kinoko_squirrel_c_api_contract\n',1)
p.write_text(s)
assert hashlib.sha256(p.read_bytes()).hexdigest()=='bed8277af878fa33ee56f24ad20aefcd9138d667de3cae562ceea33f3cbee030'
Path(__file__).unlink()

# commit: fix(squirrel): register recovered native callbacks by compiled symbol
from pathlib import Path
import sys,re,json
sys.path.insert(0,'tools')
from audit_unused_crt import sha
from audit_legacy_islands import REFERENCE
path=Path('src/decompiled/6kinoko_rebuilt.c');text=path.read_text()
assert sha(text.encode())=='6cdbbcf428b84ff80433b6ce9082de8f1aed3586966fdc8b5afb1129b14a5400'
typed={
'0x4a1760':('kinoko_sqrat_noop',9),
'0x42e370':('kinoko_sqrat_get_int',9),'0x42e3d0':('kinoko_sqrat_set_int',9),
'0x43e640':('kinoko_sqrat_get_float',7),'0x43a460':('kinoko_sqrat_set_float',7),
'0x454c20':('kinoko_sqrat_get_bool',2),'0x423c90':('kinoko_sqrat_set_bool',2),
'0x454cf0':('kinoko_sqrat_get_pointer_float',1),'0x423cf0':('kinoko_sqrat_set_pointer_float',1),
'0x423d60':('kinoko_sqrat_get_pointer_int',1),'0x454c80':('kinoko_sqrat_set_pointer_int',1),
'0x433020':('kinoko_sqrat_get_short',1),'0x433080':('kinoko_sqrat_set_short',1)}
existing={'0x41e260':9,'0x41e2c0':9,'0x431650':9,'0x44ba90':4,'0x44bb10':4,'0x4330e0':1,'0x433160':1,'0x43ab00':1}
original=Path(REFERENCE).read_text();ranges=[(int(m[1],16),int(m[2],16),original.count('\n',0,m.start())+1) for m in re.finditer(r'// Address range: (0x[0-9a-f]+) - (0x[0-9a-f]+)',original)]
entries=[]
for value,(name,count) in {**typed,**{a:('function_'+a[2:],n) for a,n in existing.items()}}.items():
    old='(SQFUNCTION)kinoko_pointer('+value+')'
    actual=len(re.findall(r'\bsq_newclosure\([^;\n]*'+re.escape(old),text))
    assert actual==count,(value,actual,count)
    new=name if value in typed else '(SQFUNCTION)'+name
    text=text.replace(old,new)
    owners=[dict(begin=hex(lo),end=hex(hi),line=line) for lo,hi,line in ranges if lo<=int(value,16)<hi]
    assert len(owners)==1,(value,owners)
    entries.append(dict(original=value,target=name,registrations=count,original_body=owners[0],
        change='typed source field callback' if value in typed else 'compiled symbol; callee body unchanged'))
text=text.replace('#include "kinoko/native_property_bridge.h"','#include "kinoko/native_property_bridge.h"\n#include "kinoko/native_property_callbacks.h"')
assert sum(e['registrations'] for e in entries)==89
assert not re.search(r'\bsq_newclosure\([^;\n]*kinoko_pointer\(0x',text)
assert sha(text.encode()) == '77527c46ee03967c90cce6f1effae8fcd38d76db214c8532df0b2691b7bc960c'
path.write_text(text)
report=dict(before_sha256='6cdbbcf428b84ff80433b6ce9082de8f1aed3586966fdc8b5afb1129b14a5400',after_sha256=sha(text.encode()),reference_sha256=sha(original.encode()),entries=entries)
Path('docs/legacy-library-audit-20260920/native-callback-addresses.json').write_text(json.dumps(report,indent=2)+'\n')
p=Path('tools/check_migration_boundaries.py');s=p.read_text();assert sha(s.encode())=='5b33086a38d3a2e0579cca8d6edb1c8497f264adf0fa259a0c9e7c3cb8cea59a'
s=s.replace('def masked(text: str)', '''# Original EXE addresses are not relocated C function pointers. A retained
# generated registration must name a compiled callback, even when the original
# decompiler merged that callback into another function's error branch.
CALLBACK_LITERAL = re.compile(r'\\bsq_newclosure\\s*\\([^;]*,\\s*\\(\\s*SQFUNCTION\\s*\\)\\s*(?:kinoko_pointer\\s*\\(\\s*)?(?:0x[0-9A-Fa-f]+|\\d+)\\b')

def masked(text: str)''')
needle="                errors.append(f'{path.relative_to(ROOT)}:{line}: handwritten assembly/naked entry')"
s=s.replace(needle,needle+'''
            for match in CALLBACK_LITERAL.finditer(text):
                line = text.count('\\n', 0, match.start()) + 1
                errors.append(f'{path.relative_to(ROOT)}:{line}: original-image native callback address')''')
s=s.replace('zero inline assembly/naked entries; original reference intact.', 'zero inline assembly/naked entries and literal native-closure addresses; original reference intact.')
assert sha(s.encode())=='0ec3ec62bc74fb4f8e5ad10fea4c60fff14427c167324a2693a8987ec63b902c'
p.write_text(s)
p=Path('tests/test_legacy_islands.py');s=p.read_text();assert sha(s.encode())=='feddd681fa3ff387733d0bbb94840688bb366a35908da15d8a7a9f5c34555a07'
pos=s.index('    def test_duplicate_definition')
s=s[:pos]+'''    def test_original_image_callbacks_are_rejected(self):
        from check_migration_boundaries import CALLBACK_LITERAL, masked
        for code in ('sq_newclosure(vm, (SQFUNCTION)kinoko_pointer(0x01020304), 1);',
                     'sq_newclosure(kinoko_vm(vm),\\n (SQFUNCTION)0x01020304, 0);',
                     'sq_newclosure(vm, (SQFUNCTION)kinoko_pointer(16909060), 1);'):
            with self.subTest(code=code):
                self.assertIsNotNone(CALLBACK_LITERAL.search(masked(code)))
        for code in ('sq_newclosure(vm, kinoko_sqrat_get_float, 1);',
                     'sq_newclosure(vm, (SQFUNCTION)function_callback, 1);',
                     'sq_newclosure(vm, (SQFUNCTION)kinoko_pointer(callback), 1);',
                     '// sq_newclosure(vm, (SQFUNCTION)0x01020304, 1);'):
            with self.subTest(code=code):
                self.assertIsNone(CALLBACK_LITERAL.search(masked(code)))

'''+s[pos:]
assert sha(s.encode())=='87793b025e1fbc977dacb7154c762cdf92b94b397ac255201bfdde47e104d70d'
p.write_text(s)
print('callback addresses:',sum(e['registrations'] for e in entries),report['after_sha256'])

# commit: refactor: isolate native strong and weak ownership from Squirrel references
from pathlib import Path
import re, sys, hashlib
sys.path.insert(0, 'tools')
from audit_unused_crt import definitions, LEX
root = Path.cwd()
before={
  'CMakeLists.txt': '6b211722eef39a9da445f2523073018ea8ad8011bb1a93afe2b9caff88aabd56',
  'src/decompiled/6kinoko_rebuilt.c': '2f4b0190bb172a2f09c5b3a0e67ff8efcb28bebbcc6a0c4bb80840173377ef50',
  'src/squirrel/actor_lifecycle.cpp': 'a07205187338100c79dd2edd1aa846050857a6985d408b870f41ebdce8b0bb78',
  'include/kinoko/actor_lifecycle.h': 'db1d706f87f797707a8b14ff404e5dba6fbf98ef1bb789a761a10ff4eefa4de1',
  'include/kinoko/actor_records.hpp': '357ccfddb7191f5e76f3f0cb9c5ac2938bc8531013f7b6bd444ff8df11b33597',
  'tests/actor_lifecycle_contract.cpp': '66c55d3984d54666c629490e10ba0059b2831bb0fb960bc302182d4bb3563469',
  'tests/stage_contract.c': '4f531c75b78831d9ccef104f6f761fb98017b28c152ba33145d9230758e400ba'
}
for name,digest in before.items(): assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
p=root/'src/squirrel/actor_lifecycle.cpp';s=p.read_text()
a=s.index('// Boost-style native strong/weak counts');b=s.index('void raw_set_step',a);s=s[:a]+s[b:]
a=s.index('extern "C" void retdec_actor_release_weak');b=s.index('extern "C" int32_t function_45e460_this',a);s=s[:a]+s[b:]
s=s.replace('if (next) ControlView(next).add_weak(1);','kinoko_native_add_weak(next);')
p.write_text(s)
p=root/'include/kinoko/actor_lifecycle.h';s=p.read_text().replace('#include <stdint.h>','#include <stdint.h>\n#include "kinoko/native_control.h"');s=s.replace('void retdec_actor_release_weak(int32_t control);\n','').replace('void retdec_release_squirrel_object(int32_t control);\n','');p.write_text(s)
p=root/'include/kinoko/actor_records.hpp';s=p.read_text().replace('#include "kinoko/native_record_view.hpp"','#include "kinoko/native_record_view.hpp"\n#include "kinoko/native_control.hpp"');a=s.index('struct ControlRecord {');b=s.index('struct AnimationRecord {',a);s=s[:a]+'using native::ControlRecord;\nusing native::ControlTable;\n'+s[b:];p.write_text(s)
p=root/'src/decompiled/6kinoko_rebuilt.c';s=p.read_text()
names={'function_45dac0','function_45dac0_this','function_45e270_this','function_45e410_this'}
count=0
for e in reversed(list(definitions(s))):
 if e['name'] in names:s=s[:e['start']]+s[e['end']:];count+=1
assert count==4,count
for name in names:
 s,n=re.subn(r'^(?:static )?int32_t\s*\*?\s*'+name+r'\([^;{}]*\);\n','',s,flags=re.M);assert n==1,(name,n)
s=s.replace('if (control != 0)\n        InterlockedIncrement((volatile LONG *)(intptr_t)(control + 8));','kinoko_native_add_weak(control);')
p.write_text(s)
mapping={'retdec_actor_release_weak':'kinoko_native_release_weak','retdec_release_squirrel_object':'kinoko_native_release_strong','function_45e270_this':'kinoko_native_control_create','function_45e410_this':'kinoko_native_weak_pair_lock'}
pat=re.compile(r'\b('+'|'.join(mapping)+r')\b')
for folder in ('src','include','tests'):
 for p in (root/folder).rglob('*'):
  if p.suffix not in ('.c','.cpp','.h','.hpp') or p.name=='6kinoko.exe.c':continue
  s=p.read_text();parts=[];end=0
  for m in LEX.finditer(s):parts.extend((pat.sub(lambda m:mapping[m[0]],s[end:m.start()]),m[0]));end=m.end()
  parts.append(pat.sub(lambda m:mapping[m[0]],s[end:]));new=''.join(parts)
  if new!=s:p.write_text(new)
p=root/'CMakeLists.txt';s=p.read_text();marker='add_library(kinoko_legacy_abi STATIC'
s=s.replace(marker,'add_library(kinoko_native_control STATIC src/reconstructed/native_control.cpp)\ntarget_include_directories(kinoko_native_control PUBLIC include)\n\n'+marker)
marker='target_include_directories(kinoko_squirrel_cpp_vm'
pos=s.index(marker);s=s[:pos]+'target_link_libraries(kinoko_squirrel_cpp_vm PUBLIC kinoko_native_control)\n'+s[pos:]
marker='add_executable(kinoko_actor_lifecycle_contract'
pos=s.index(marker);s=s[:pos]+'''add_executable(kinoko_native_control_contract tests/native_control_contract.cpp)
target_link_libraries(kinoko_native_control_contract PRIVATE kinoko_native_control)
add_test(NAME native_control_contract COMMAND kinoko_native_control_contract)
set_tests_properties(native_control_contract PROPERTIES TIMEOUT 120)

'''+s[pos:]
s=s.replace('set(KINOKO_TOOL_TARGETS\n','set(KINOKO_TOOL_TARGETS\n    kinoko_native_control_contract\n')
p.write_text(s)
after={
  'CMakeLists.txt': 'ec46f447b546d7b005ed52de4e13df20f5e6be3685e5f3f197346dba94180279',
  'src/decompiled/6kinoko_rebuilt.c': '0a73978b6f1735067b0fde71ef0f7ae190e64611184ae428c59fbae6d4883a8e',
  'src/squirrel/actor_lifecycle.cpp': '8e8a6a890c13aa5527acaa210a87e93dae6d1806018b2209901790e4e16eaeda',
  'include/kinoko/actor_lifecycle.h': '1926d85fabde3201f879c70372710f2050d7544eb62000506cac5788ee481004',
  'include/kinoko/actor_records.hpp': 'b74e932c655d978669440f9670b3a04323ca9807aa8fa9a54dbbcaa20bd52dfb',
  'tests/actor_lifecycle_contract.cpp': 'cb67b72dab3933b884fdb717001f059a0f8994361763994b5ca309d8c5467a81',
  'tests/stage_contract.c': '5dab61d3137ec71c60ca2a3b80f896d56820b39a12373a2cf9b36e7bdca6a51a'
}
for name,digest in after.items(): assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
new={
  'include/kinoko/native_control.h': '8cf40795fdcb76475d557094a97f3fa8538f2128588ed11e80a2c1c817c87f45',
  'include/kinoko/native_control.hpp': '85c6c5a69210a37356742e0e0e51ee16a94bd4530a1656cde6a18d5e73e2a5e8',
  'src/reconstructed/native_control.cpp': '8690d4e14d91086bf32cdf91dfdb5390a6afb0edf08d5a82db0e73c12ad13e4c',
  'tests/native_control_contract.cpp': 'd04b39c10b44d45d6c42bf9f7c6419613f48b7498e142dd2de88602a6969ecab'
}
for name,digest in new.items(): assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name

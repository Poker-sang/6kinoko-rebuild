from pathlib import Path
import json,re,subprocess,hashlib
r=Path('.'); p=r/'src/decompiled/6kinoko_rebuilt.c'; s=p.read_text()
# Reviewed extraction; all eleven generated files must match local SHA256s.
pattern=re.compile(r'(?m)^(?:(?:static|inline|__declspec\(noinline\)|__declspec\(noreturn\))[ \t]+)*(?:(?:unsigned|signed|const|struct)[ \t]+)*[A-Za-z_]\w*(?:[ \t]+[A-Za-z_]\w*)?[ \t*]+(?:(?:__fastcall|__cdecl|__stdcall)[ \t]+)?([A-Za-z_]\w*)[ \t]*\([^;{}]*?\)[ \t\r\n]*\{')
fs=[]
for match in pattern.finditer(s):
 end=s.find('\n}',match.end())+2
 if end<2 or match[1] in ('if','while','switch'):continue
 fs.append(dict(name=match[1],body=s[match.start():end],line=s.count('\n',0,match.start())+1,lines=s.count('\n',match.start(),end)+1))
byname={f['name']:f for f in fs}
groups={"lifetime": ["retdec_destroy_cact_script", "retdec_destroy_cact_list", "retdec_destroy_cact_layer", "retdec_destroy_cact_resource", "retdec_destroy_cact_object", "retdec_destroy_cact_with_flags"], "document": ["retdec_act_free_properties", "retdec_act_read_u8", "retdec_act_read_u32", "retdec_act_read_properties", "retdec_act_property_integer", "retdec_act_property_float", "retdec_act_assign_string", "retdec_act_apply_cact", "retdec_act_apply_script", "retdec_act_apply_layer", "retdec_act_apply_layout", "retdec_act_apply_map_layout", "retdec_act_apply_resource", "retdec_act_apply_chip_resource", "retdec_act_load_script", "retdec_act_make_list", "retdec_act_append_list", "retdec_act_make_layer", "retdec_act_make_layout", "retdec_act_make_map_layout", "retdec_act_free_map_records", "retdec_act_read_map_records", "retdec_act_load_key", "retdec_act_make_key", "retdec_act_load_layer", "retdec_mcd_u32", "retdec_mcd_i16", "retdec_mcd_find_chip", "retdec_mcd_find_texture", "retdec_mcd_free", "retdec_act_load_mcd", "retdec_act_make_resource", "retdec_c2dmaplayout_set_layer_impl", "retdec_act_bind_layouts", "retdec_act_prepare_vector", "retdec_act_load"], "map": ["retdec_map_sprite_init", "kinoko_map_update", "kinoko_map_draw"], "binding": ["retdec_publish_cact_layer_property", "retdec_publish_cact_layer_members", "retdec_publish_c2dlayout_properties", "retdec_publish_c2dlayout_class", "retdec_cact_associate_resource", "retdec_publish_cact_resource2d_class", "retdec_publish_cact_layer_class", "retdec_publish_acting_player_properties", "retdec_publish_acting_player_class", "retdec_publish_acting_player", "retdec_execute_act_source_script", "retdec_execute_act_callback", "retdec_publish_act_script_constants", "retdec_prepare_cact_layer_objects", "retdec_publish_c2dlayout_values", "retdec_map_chip_count", "retdec_map_get_chip_layout", "retdec_map_layout_argument", "retdec_map_record_at", "retdec_map_chip_data", "retdec_map_get_chip_by_position", "retdec_map_set_chip_rect", "retdec_map_set_chip_layout", "retdec_map_set_chip_id", "retdec_map_get_chip_id", "retdec_map_compare_records", "retdec_map_prearrangement", "retdec_publish_map_view_class", "retdec_publish_c2dmaplayout_class", "retdec_resource_get_chip_info", "retdec_get_act_resource_class", "retdec_publish_act_resource_values", "retdec_publish_act_resource_pairs", "retdec_publish_act_layers", "retdec_bind_act_resource_object", "retdec_register_runtime_act_script", "retdec_begin_stage_this", "retdec_root_table_register_resource", "retdec_root_table_construct_this"], "containers": ["retdec_string_length32", "retdec_string_data32", "retdec_compare_bytes32", "retdec_compare_strings32", "retdec_string_hash32", "retdec_vector_insert32", "retdec_erase_node32", "function_44e780", "function_458090", "function_458200", "function_4583a0"]}
groups['lifetime'].insert(0,'retdec_construct_cact_script')
groups['layout']=[f['name'] for f in fs if 59562<=f['line']<60285]
selected={n for ns in groups.values() for n in ns}
assert len(selected)==106
text='\n'.join(byname[n]['body'] for n in selected)
structs=['retdec_act_property','retdec_mcd_chip','retdec_mcd_texture','retdec_mcd_data','retdec_native_view_property']
structtext='\n\n'.join(re.search(r'(?m)^struct '+n+r' \{[\s\S]*?\n};',s)[0] for n in structs)
header_names=set()
headers=['sqrat_object_bridge.h','native_property_bridge.h','squirrel_binding.h','squirrel_native_calls.h','squirrel_game_objects.h','squirrel_native_arguments.h','squirrel_host_compat.h','squirrel_legacy_api.h','squirrel_source_runtime.h','squirrel_compile_bridge.h','squirrel_value_bridge.h','actor_methods.h','actor_animation.h','actor_cleanup.h','act_clone.h','act_resource.h','actor_lifecycle.h','legacy_method_entries.h','script_callbacks.h','squirrel_object.h','texture_store.h','map_render.h','sprite.h','game_math.h']
for h in headers:
 header_names.update(re.findall(r'\b((?:retdec_|kinoko_|function_)[\w]+)\s*\(', (r/'include/kinoko'/h).read_text()))
allids=set(re.findall(r'\b(?:retdec_|kinoko_|function_)[A-Za-z0-9_]+',text))
needed=allids-selected-header_names
host_funcs={n:byname[n] for n in needed if n in byname}
signature=lambda f: re.sub(r'^static\s+','',f['body'].split('{',1)[0].strip()).replace('float32_t','float')
vtables={'g231':'script_vtable','g252':'layer_vtable','g285':'act_vtable','g313':'chip_resource_vtable','g327':'map_layout_vtable','g277':'key_vtable','g299':'layout_vtable','g300':'layout_sprite_vtable','g328':'map_view_vtable','g365':'texture_resource_vtable','g251':'layer_ref_vtable','g253':'layer_layout_vtable','g39':'sq_object_vtable','g40':'sq_root_vtable'}
vtablekeys=sorted(set(re.findall(r'&\s*(g\d+)\b',text))&set(vtables))
globals=[]
for n in sorted(set(re.findall(r'\bg\d+\b',text))-set(vtables)):
 m=re.search(r'(?m)^((?:char|int32_t|uint32_t|float|float32_t)\s*\*?)\s*'+n+r'\b',s)
 if not m: raise ValueError('missing global '+n)
 globals.append('extern '+m[1]+' '+n+';')
watch=['retdec_primary_shared_state','retdec_release_watch_data','retdec_release_watch_count']
for n in watch:
 if n in allids:
  m=re.search(r'(?m)^static (int32_t '+n+r'(?:\[\d+\])?);',s)
  assert m,n
  globals.append('extern '+m[1]+';')
  s=s[:m.start()]+m[1]+';'+s[m.end():]
api='''#pragma once
#include <stdint.h>
#include <stddef.h>

/* Recovered ACT/MCD records shared by the loader, native renderer and Sqrat
 * bindings. Pointer-bearing records require the original Win32 ABI. */
'''+structtext+'''

#ifdef __cplusplus
extern "C" {
#endif

'''+ '\n'.join(signature(byname[n])+';' for n in sorted(selected))+'''

#ifdef __cplusplus
}
#endif
'''
(r/'include/kinoko/act_runtime.h').write_text(api)
host='''#pragma once
#include <stdint.h>
#include <stddef.h>

/* Temporary host ports. These preserve original object identities and I/O;
 * the C++ ACT implementation does not choose another resource directory. */
struct KinokoActHostSymbols {
'''+''.join('    const void* '+vtables[n]+';\n' for n in vtablekeys)+'''};

#ifdef __cplusplus
extern "C" {
#endif
const struct KinokoActHostSymbols* kinoko_act_host_symbols(void);
'''+ '\n'.join(globals)+'\n\n'+ '\n'.join(signature(f)+';' for n,f in sorted(host_funcs.items()))+'''
#ifdef __cplusplus
}
#endif
'''
(r/'include/kinoko/act_host.h').write_text(host)
for n in selected:
 body=byname[n]['body']
 assert s.count(body)==1,n
 s=s.replace(body,'/* '+n+' is implemented in native C++ (kinoko/act_runtime.h). */',1)
for n in structs:
 s=re.sub(r'(?m)^struct '+n+r' \{[\s\S]*?\n};','',s,count=1)
for n in selected|set(host_funcs):
 s=re.sub(r'(?m)^static\s+((?:[^;{}\n]+\n?){0,2}?\b'+re.escape(n)+r'\s*\()',r'\1',s)
s='#include "kinoko/act_runtime.h"\n#include "kinoko/act_host.h"\n'+s
s+='\n/* Read-only original vtable identities for the extracted ACT subsystem. */\nconst struct KinokoActHostSymbols* kinoko_act_host_symbols(void)\n{\n    static const struct KinokoActHostSymbols symbols = {\n'+''.join('        &'+n+', /* '+vtables[n]+' */\n' for n in vtablekeys)+'    };\n    return &symbols;\n}\n'
p.write_text(s)
(r/'include/kinoko/legacy_memory.hpp').write_text('''#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <type_traits>

namespace kinoko::legacy {
static_assert(sizeof(void*) == 4, "Legacy object addresses are Win32 values");

template<class T = void> T* pointer(std::int32_t value) noexcept {
    return reinterpret_cast<T*>(static_cast<std::uintptr_t>(static_cast<std::uint32_t>(value)));
}
template<class T> std::int32_t address(T* value) noexcept {
    return static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(value));
}
// These fields belong to native byte-layout records, not Squirrel internals.
// Do not use this accessor to reproduce the VM implementation: use sq_*.
template<class T> T& field(std::int32_t base, std::uint32_t offset = 0) noexcept {
    return *reinterpret_cast<T*>(static_cast<std::uintptr_t>(
        static_cast<std::uint32_t>(base) + offset));
}
template<class T> T load(const void* source) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    T value; std::memcpy(&value, source, sizeof value); return value;
}
template<class T> void store(void* destination, const T& value) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memcpy(destination, &value, sizeof value);
}
struct Free {
    void operator()(void* value) const noexcept { std::free(value); }
};
template<class T> using Allocation = std::unique_ptr<T, Free>;
}
''')
cast=re.compile(r'(?P<deref>\*)?\(\s*(?P<type>(?:const\s+)?(?:unsigned\s+char|signed\s+char|struct\s+\w+|\w+)\s*\*+)\s*\)\s*\((?:u?intptr_t)\)\s*(?:\(uint32_t\)\s*)?')
def atom_end(t,start):
 if t[start]=='(':
  depth=1;i=start+1
  while depth:
   if t[i]=='(':depth+=1
   if t[i]==')':depth-=1
   i+=1
  return i
 if t[start]=='*':return atom_end(t,start+1)
 m=re.match(r'[\w:]+(?:<[^<>]*>)?',t[start:])
 if not m:raise ValueError('notatom '+t[start:start+70])
 i=start+m.end()
 while i<len(t) and t[i] in '([':
  close=')' if t[i]=='(' else ']';opening=t[i];dep=1;i+=1
  while dep:
   if t[i]==opening:dep+=1
   if t[i]==close:dep-=1
   i+=1
 return i

def transform(t):
 t=re.sub(r'^static\s+','',t)
 t=t.replace('float32_t','float').replace('struct retdec_RTL_CRITICAL_SECTION','CRITICAL_SECTION')
 t=t.replace('g_retdec_act_texture_slots','kinoko_texture_slots').replace('RETDEC_ACT_TEXTURE_SLOT_COUNT','KINOKO_TEXTURE_CAPACITY')
 for n in vtablekeys:t=re.sub(r'&\s*'+n+r'\b','kinoko_act_host_symbols()->'+vtables[n],t)
 for _ in range(20):
  matches=list(cast.finditer(t))
  if not matches:break
  for m in reversed(matches):
   try:end=atom_end(t,m.end())
   except (ValueError,IndexError):continue
   expr=t[m.end():end]
   if cast.search(expr):continue
   ty=re.sub(r'\bstruct\s+','',m['type'].strip()[:-1].strip())
   kind='field' if m['deref'] else 'pointer'
   if expr.startswith('(') and atom_end(expr,0)==len(expr):expr=expr[1:-1]
   t=t[:m.start()]+kind+'<'+ty+'>('+expr+')'+t[end:]
 ac=re.compile(r'\(int32_t\)\s*\((?:u?intptr_t)\)\s*')
 for m in reversed(list(ac.finditer(t))):
  try:
   start=m.end();end=atom_end(t,start+1) if t[start]=='&' else atom_end(t,start)
  except (ValueError,IndexError):continue
  expr=t[start:end]
  t=t[:m.start()]+'address('+expr+')'+t[end:]
 t=re.sub(r'\bNULL\b','nullptr',t)
 for n in ['memcpy','memmove','memset','memcmp','malloc','calloc','realloc','free','strcmp','strncmp','strlen','qsort','fabs','cos','sin','sqrt']:
  t=re.sub(r'(?<![\w:])'+n+r'\s*\(', 'std::'+n+'(',t)
 t=re.sub(r'device->lpVtbl->(\w+)\(\s*device\s*,\s*',r'device->\1(',t)
 t=t.replace('device != nullptr && device->lpVtbl != nullptr &&','device != nullptr &&')
 return t
for group,ns in groups.items():
 location='src/squirrel/act_binding.cpp' if group=='binding' else 'src/reconstructed/act_'+group+'.cpp'
 intro='''// Native C++ continuation of the recovered ACT path. Original function names
// remain C ABI ports until the surrounding decompiled host is migrated.
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/legacy_memory.hpp"
'''+''.join('#include "kinoko/'+h+'"\n' for h in headers)+'''#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::field;

'''
 (r/location).write_text(intro+'\n\n'.join(transform(byname[n]['body']) for n in ns)+'\n')
cm=r/'CMakeLists.txt';c=cm.read_text()
c=c.replace('add_library(kinoko_squirrel_cpp_vm STATIC\n','add_library(kinoko_squirrel_cpp_vm STATIC\n'+''.join('    src/reconstructed/act_'+g+'.cpp\n' for g in groups if g!='binding')+'    src/squirrel/act_binding.cpp\n')
cm.write_text(c)
# Match the concrete C++ API signatures; no change to field offsets or I/O.
h=Path('include/kinoko/act_host.h');t=h.read_text().replace('float80_t','long double')
t=t.replace('int32_t function_402d40(', 'int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);\nint32_t function_402d40(');h.write_text(t)
for p in [*Path('src/reconstructed').glob('act_*.cpp'),Path('src/squirrel/act_binding.cpp')]:
 t=p.read_text()
 if 'Native C++ continuation' not in t:continue
 t=t.replace('#include "kinoko/act_host.h"', '#include "kinoko/act_host.h"\n#include "kinoko/diagnostics.h"')
 t=re.sub(r'address\(kinoko_act_host_symbols\(\)\)->(\w+)',r'address(kinoko_act_host_symbols()->\1)',t)
 t=t.replace('float80_t','long double').replace('device->lpVtbl != nullptr','field<void*>(address(device)) != nullptr')
 if p.name=='act_binding.cpp':
  t=t.replace('#include <d3d9.h>','#include <d3d9.h>\n#include <mmsystem.h>')
  t=t.replace('retdec_sqrat_get(', 'get_pair(')
  helper='\nnamespace {\n// Typed wrapper around the existing source-backed Sqrat C boundary.\nint32_t get_pair(int32_t object, const char* name, int32_t* output) {\n    return retdec_sqrat_get(object, name, address(output));\n}\n}\n'
  t=t.replace('using kinoko::legacy::field;\n','using kinoko::legacy::field;\n'+helper)
  t=t.replace('address(chip)->bytes','address(chip->bytes)')
  t=t.replace('_free(pointer<void>(old_link))','std::free(pointer<void>(old_link))')
  t=t.replace('function_402d40(address(path),','function_402d40(const_cast<char*>(path),')
  t=t.replace('function_402d40(address(script_path),','function_402d40(const_cast<char*>(script_path),')
  a=t.index('int32_t retdec_root_table_register_resource(');b=t.index('int32_t retdec_root_table_construct_this(',a)
  q=t[a:b].replace('address(root_object)','root_object')
  q=q.replace('retdec_bind_act_resource_root(resource_ptr, vm, root_object + 8)','retdec_bind_act_resource_root(resource_ptr, vm, pointer<const int32_t>(root_object + 8))')
  t=t[:a]+q+t[b:]
 p.write_text(t)
expected={'CMakeLists.txt': '314649863934ecbeeaaaddd67c1cd06d7d6e80a89238c43365815cffdd76c21b', 'include/kinoko/act_host.h': 'b5e54d4800e4032da06452bb12bc6073330387ac248328fcc8a60109a38cd4f5', 'include/kinoko/act_runtime.h': '0e5875b9757ed0dd2a3e3860e2dbd5b4c797f31afbd1da83a921468a2bbcd130', 'include/kinoko/legacy_memory.hpp': '33af0564b2e18a0cec580d561e7114b95755dc675d3fa8b46661ca44222bef1b', 'src/decompiled/6kinoko_rebuilt.c': '51bb6f14d81379649b86e589ae7151567d55114c59989356bf7b4ded9d4242af', 'src/reconstructed/act_containers.cpp': '2ae8bfc08d726ff8faab32353896d7d4f53e69f10b4ad0ab568a288efa73cc86', 'src/reconstructed/act_document.cpp': '457210ceb45dbcec1f600623ba35256742d727f133b63103c2002e53aadcecb7', 'src/reconstructed/act_layout.cpp': '7a75a6e9f48498a175470a1a95f27cde6dc35db908aad94f9cc5e5413a39734d', 'src/reconstructed/act_lifetime.cpp': 'e64e53fa6608769c859824f4744198a28fe05a2cf885cfb7de91526a6faf21d9', 'src/reconstructed/act_map.cpp': 'f6c614682e176e167403bbc51749acf4f73abd5ff8ad2b7065ab4ac71a5158f8', 'src/squirrel/act_binding.cpp': 'ec944957cde3a3906662903ca11c51e307c6cd8023bd91f5ff5696d7c0f585d1'}
for name,digest in expected.items():
 assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==digest,name
print('PASS: 106 live functions, eleven files match reviewed source hashes')
Path(__file__).unlink()

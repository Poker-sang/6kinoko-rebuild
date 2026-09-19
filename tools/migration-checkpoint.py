"""One-shot build wiring for the already committed, reviewed C++ sources."""
from pathlib import Path
import hashlib, json, subprocess
p = Path('CMakeLists.txt')
s = p.read_text()
assert 'src/platform/legacy_frame_copy.cpp' not in s
s = s.replace('    src/platform/retdec_memory.cpp\n', '    src/platform/retdec_memory.cpp\n    src/platform/legacy_frame_copy.cpp\n')
assert s.count('    src/decompiled/retdec_runtime_compat.c\n') == 2
s = s.replace('    src/decompiled/retdec_runtime_compat.c\n', '')
s = s.replace('target_compile_options(kinoko_retdec_support PRIVATE /Gy)', 'target_compile_options(kinoko_retdec_support PRIVATE /Gy)\n# Only this transitional frame adapter needs fixed frames, never the callers.\nset_source_files_properties(src/platform/legacy_frame_copy.cpp\n    PROPERTIES COMPILE_OPTIONS "/Oy-;/GL-")')
extra = '''# Both frame-pointer policies must agree with the register captured by Windows.
foreach(policy IN ITEMS checked optimized)
    add_executable(kinoko_legacy_frame_copy_${policy}_contract tests/legacy_frame_copy_contract.cpp)
    target_include_directories(kinoko_legacy_frame_copy_${policy}_contract PRIVATE include)
    target_link_libraries(kinoko_legacy_frame_copy_${policy}_contract PRIVATE kinoko_retdec_support)
    if(policy STREQUAL "checked")
        target_compile_options(kinoko_legacy_frame_copy_${policy}_contract PRIVATE /Od /RTC1 /Oy-)
    else()
        target_compile_options(kinoko_legacy_frame_copy_${policy}_contract PRIVATE /O2 /Oy)
    endif()
    target_link_options(kinoko_legacy_frame_copy_${policy}_contract PRIVATE /OPT:REF /OPT:ICF)
    add_test(NAME legacy_frame_copy_${policy}_contract COMMAND kinoko_legacy_frame_copy_${policy}_contract)
    set_tests_properties(legacy_frame_copy_${policy}_contract PROPERTIES TIMEOUT 120)
endforeach()

'''
s = s.replace('set(KINOKO_TOOL_TARGETS', extra+'set(KINOKO_TOOL_TARGETS')
s = s.replace('    kinoko_actor_lifecycle_contract kinoko_legacy_copy_entries_contract', '    kinoko_actor_lifecycle_contract kinoko_legacy_copy_entries_contract\n    kinoko_legacy_frame_copy_checked_contract kinoko_legacy_frame_copy_optimized_contract')
p.write_text(s)
p = Path('src/decompiled/6kinoko_rebuilt.c')
s = p.read_text()
assert hashlib.sha256(s.encode()).hexdigest() == '054256e300192c646e3ad5eab850111f149570324d8f683ae9558060965e403a'
s = s.replace('// int32_t __ftol(void);\n', '')
p.write_text(s)
Path('src/decompiled/retdec_runtime_compat.c').unlink()
p = Path('docs/no-inline-asm-20260919/migrated-definitions.json')
x = json.loads(p.read_text())
x['parent'] = 'e34e555388ecf28c01530186660222bef8090573'
x['source'] = 'src/decompiled/6kinoko_rebuilt.c'
old = subprocess.check_output(['git','show',x['parent']+':'+x['source']],text=True).splitlines()
for d in x['definitions']:
    d['sha256'] = hashlib.sha256('\n'.join(old[d['line']-1:d['line']-1+d['body_lines']]).encode()).hexdigest()
x['classification'] = '11 distinct C entry names replaced by typed C++ (15 conditional definitions; 199 function-body lines)'
p.write_text(json.dumps(x,indent=2)+'\n')
Path(__file__).unlink()

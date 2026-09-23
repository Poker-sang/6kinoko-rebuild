"""Approximate lexical inventory, not an AST or a semantic-completion score."""
import collections
import json
import re
import subprocess
from pathlib import Path
from audit_unused_crt import mask

ROOT = Path(__file__).resolve().parents[1]
SIGNATURE = re.compile(r'^[ \t]*(?:[\w:*&<>~,]+[ \t]+)*([\w:~]+)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?\{', re.M)

def main():
    files = sorted(set(re.findall(r'src/[\w/.-]+\.(?:cpp|c)\b', (ROOT/'CMakeLists.txt').read_text())))
    entries = []
    for file in files:
        source = (ROOT/file).read_text(encoding='utf-8-sig')
        code = mask(source, strings=True)
        consumed = 0
        for match in SIGNATURE.finditer(code):
            if match.start() < consumed or match[1] in ('if','for','while','switch','catch'):
                continue
            end, depth = match.end(), 1
            while end < len(code) and depth:
                depth += (code[end] == '{') - (code[end] == '}')
                end += 1
            consumed = end
            body = code[match.end():end-1]
            address_name = bool(re.fullmatch(r'function_[0-9a-fA-F]+(?:_\w+)?', match[1]))
            thin = (body.count(';') <= 2 and not re.search(r'\b(if|for|while|switch|goto)\b', body)
                    and bool(re.search(r'\bkinoko_\w+\s*\(', body)))
            legacy = bool(re.search(r'\b(?:function_[0-9a-fA-F]+|v\d+|g\d+)\b|\bgoto\b|\([^\n]*intptr_t\)[^\n]*\+\s*\d+', body))
            compatibility = (
                match[1] in {'__declspec', 'RETDEC_ASM_STUBS',
                             '_3f__3f_0_3f__24_basic_string_40_DU_3f__24_char_traits_40_D_40_std_40__40_V_3f__24_allocator_40_D_40_2_40__std_40__40_QAE_40_PBD_40_Z',
                             '_3f__3f_3_40_YAXPAX_40_Z',
                             'kinoko_sqrat_object_vtable', 'kinoko_sqrat_root_vtable',
                             'kinoko_actor_vtable', 'kinoko_actor_step_key',
                             'kinoko_squirrel_object_vtable',
                             'retdec_msvc_Finitlocks__YAXXZ7',
                             'retdec_msvc_Finitlocks__YAXXZ8',
                             'retdec_msvc_Finitlocks__YAXXZ9',
                             'kinoko_construct_layer_global_vm', 'kinoko_color_destroy',
                             'kinoko_script_show_message', 'kinoko_script_sleep',
                             'kinoko_script_close_window', 'kinoko_act_script_output_compiled',
                             'kinoko_act_host_symbols', 'kinoko_audio_host_symbols',
                             'kinoko_application_set_archive_mode',
                             'kinoko_application_open_archives',
                             'kinoko_game_prepare_scripts', 'kinoko_game_register_scripts',
                             'kinoko_game_release_script_reference'}
                or file.endswith('retdec_asm_stubs.c')
                or match[1].startswith('_3f__3f_'))
            if compatibility:
                category = 'thin_bridge'
            elif thin:
                category = 'thin_bridge'
            elif address_name:
                category = 'address_named_legacy'
            # A decompiled translation unit can now contain recovered, named
            # C++ bodies alongside archival compatibility code. Classify by
            # the actual legacy markers in the body instead of penalizing the
            # whole file merely because its path is decompiled/.
            elif legacy:
                category = 'named_mixed_legacy'
            else:
                category = 'named_structured_candidate'
            entries.append(dict(file=file, name=match[1], line=source.count('\n',0,match.start())+1,
                                lines=source[match.start():end].count('\n')+1, category=category))
    counts = collections.Counter(e['category'] for e in entries)
    lines = collections.Counter()
    for e in entries:
        lines[e['category']] += e['lines']
    report = dict(source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                  scope='Unique src C/C++ paths literally listed in top-level CMake; headers, macros, external source, tests and archival decompilation excluded. Lexical estimates; not linked reachability. Categories are heuristics, not evidence of semantic completeness.',
                  files=len(files), functions=len(entries), counts=dict(counts), function_body_lines=dict(lines), entries=entries)
    output = ROOT/'docs/readability-r138/inventory.json'
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k != 'entries'},indent=2))

if __name__ == '__main__':
    main()

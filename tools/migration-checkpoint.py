"""Finish the reviewed audio test migration, with exact source/output hashes."""
from pathlib import Path
import hashlib
INPUTS={'tests/stage_contract.c':'df598e9ba45a09ce905f31034bf211daccbbd084867c229d991123c42321c838','tests/audio_runtime_contract.cpp':'d11a37ae885fa05b377d0a8bcd0b978afa7c18b5782f5e137f15d4d2a6ce342d','CMakeLists.txt':'f03378b8b3034192622385a925b72e0be712f966ccdb0095a98defee993e1c8e'}
for p,h in INPUTS.items(): assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h,p
p=Path('tests/audio_runtime_contract.cpp');s=p.read_text()
a=s.index('class Buffer final : public IDirectSoundBuffer {')
b=s.index('static_assert(!std::is_copy_constructible_v<BgmTrack>);',a)
block=s[a:b].replace('class Buffer final','class AudioTestBuffer final')
h='#pragma once\n// Native SDK interfaces keep the test double ABI identical to production.\n#include <windows.h>\n#include <dsound.h>\n#include <algorithm>\n#include <vector>\n\n'+block
Path('tests/directsound_fixture.hpp').write_text(h)
s=s[:a]+'using Buffer = AudioTestBuffer;\n'+s[b:]
s=s.replace('#include <type_traits>','#include <type_traits>\n#include "directsound_fixture.hpp"')
p.write_text(s)
p=Path('tests/stage_contract.c');s=p.read_text()
s=s.replace('#include "../src/decompiled/6kinoko_rebuilt.c"','#include "../src/decompiled/6kinoko_rebuilt.c"\n#include "stage_audio_contract.h"',1)
a=s.index('static ULONG WINAPI count_sound_release(');b=s.index('static int test_global_sound_cleanup(void)',a);s=s[:a]+s[b:]
s=s.replace('    void *vtable[19] = {0};\n    int32_t buffers[3][3] = {0};\n','',1)
s=s.replace('    vtable[2] = count_sound_release;\n    vtable[18] = count_sound_stop;\n    for (int i = 0; i < 3; ++i) buffers[i][0] = PTR(vtable);\n','',1)
a=s.index('    g_retdec_se_entry_count = 2;',s.index('static int test_global_sound_cleanup'));b=s.index('    g638 = old_head; g639 = old_size;',a)
s=s[:a]+'    CHECK(kinoko_test_sound_cleanup(function_470890) == 0);\n    CHECK(g639 == 0 && head[0] == PTR(head) && head[1] == PTR(head) && head[2] == PTR(head));\n'+s[b:]
a=s.index('/* Actual w3-c02b moving terrain, original player scripts, no game window. */');b=s.index('static int test_moving_map(',a);s=s[:a]+s[b:]
s=s.replace('test_bgm_preserves_game_math()', 'kinoko_test_bgm_preserves_game_math()')
a=s.index('    if (argc == 2 && strcmp(argv[1], "--sound-module") == 0) {');b=s.index('    AddVectoredExceptionHandler(1, contract_exception);',a)
s=s[:a]+'    if (argc == 2 && strcmp(argv[1], "--sound-module") == 0)\n        return kinoko_test_sound_module();\n'+s[b:];p.write_text(s)
p=Path('CMakeLists.txt');s=p.read_text().replace('    tests/stage_contract.c\n','    tests/stage_contract.c\n    tests/stage_audio_contract.cpp\n',1)
s=s.replace('target_include_directories(kinoko_stage_contract PRIVATE\n','target_include_directories(kinoko_stage_contract PRIVATE src/decompiled\n',1);p.write_text(s)
OUTPUTS={'tests/stage_contract.c':'c1d213cd39e0836896e31e2df96814fbb74180cb879b794368e76f686c7773ab','tests/audio_runtime_contract.cpp':'d7a2594734c577d22a7c1dd7b079bf81c92b488080481b4e9ec69aaa11da2838','CMakeLists.txt':'2dc494033ecf444966beb8847b3a5cc817fe3389975c6342695ebca810807942','tests/directsound_fixture.hpp':'ed21feb6bf480069928d98aff64738e0de26fa9b2a9bda15e02de241751c4c46','tests/stage_audio_contract.h':'9124bb0fcfb90c029ac78bc6c4d2ec02514ae75db131bda5ac03fa502cb82c81','tests/stage_audio_contract.cpp':'4dbcbc4b98c6513336cc0be92ba4be82e04ff4ad9d7cdc04541d3737bec4fdb0'}
for p,h in OUTPUTS.items(): assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h,p
Path(__file__).unlink()

"""Apply the reviewed native audio batch to the pinned direct-API baseline."""
from pathlib import Path
import re,runpy,hashlib
parser=runpy.run_path('tools/audio-migration-parser.py')
mask,funcs,matching,args=(parser[x] for x in ('mask','funcs','matching','args'))
REPLACEMENTS=runpy.run_path('tools/audio-migration-bodies.py')['REPLACEMENTS']
ROOT=Path.cwd()
s=(ROOT/'src/decompiled/6kinoko_rebuilt.c').read_text()
assert hashlib.sha256(s.encode()).hexdigest()=='7f28908e3b77102324736a32b6e09d12883d105b1b1cb008595be1782ce97bfe'
fs=funcs(s); allnames={f['name']:f for f in fs}
dead={'function_40a5f0','function_40a7f0','function_40a8d0','function_40a950','function_40a9a0'}
selection={n for n in allnames if n.startswith(('retdec_audio_','retdec_bgm_','retdec_se_'))}
selection.update('function_40a0f0 function_40a3d0 function_40a460 function_40a5d0 function_40a9f0 function_40aaa0 function_40aac0 function_40aae0 function_40adb0 function_40b3a0 function_40b520 function_40b8a0 function_411d80 function_411f90 function_4701e0 function_470220 function_470290 function_470300 function_470320 function_470360 function_470980 function_470ab0 retdec_create_secondary_buffer retdec_decode_bgm retdec_fill_dsound_buffer retdec_loadse_blob retdec_read_asset_bytes retdec_release_dsound_buffer retdec_set_dsound_volume retdec_trace_audio_text'.split())
assert len(selection)==82
exports={n for n in selection if n.startswith('function_')}|{'retdec_bgm_update_fade'}
other='\n'.join(p.read_text() for p in (ROOT/'src').rglob('*') if p.suffix in ('.c','.cpp','.h','.hpp') and p.name not in ('6kinoko_rebuilt.c','6kinoko.exe.c','audio_runtime.cpp'))
masked_other=mask(other)
for name in dead:assert not re.search(r'\b'+name+r'\b',masked_other),name
rest=s
for f in reversed(fs):
 if f['name'] in selection|dead:rest=rest[:f['start']]+rest[f['end']:]
names_pattern='|'.join(re.escape(n) for n in selection|dead)
rest=re.sub(r'(?m)^(?:static[ \t]+)?(?:int32_t|int|void|LONG|uint32_t|const[ \t]+char[ \t]*\*)[ \t]*(?:'+names_pattern+r')\s*\([^;{}]*\);\n','',rest)
masked_rest=mask(rest)
for name in dead| (selection-exports):assert not re.search(r'\b'+name+r'\b',masked_rest),name
rest=rest.replace('static uint32_t retdec_safe_c_string_length(', 'uint32_t retdec_safe_c_string_length(')
a=rest.index('/* The original runtime embeds a Vorbis decoder.');b=rest.index('#include "stb_vorbis.c"',a)+len('#include "stb_vorbis.c"');rest=rest[:a]+rest[b:]
a=rest.index('// The Windows SDK\'s dsound.h');b=rest.index('#pragma pack(pop)',a)+len('#pragma pack(pop)');rest=rest[:a]+rest[b:]
a=rest.index('/* The audio manager is a single object');b=rest.index('int32_t g805 =',a);rest=rest[:a]+rest[b:]
rest=re.sub(r'(?m)^#define RETDEC_BGM_\w+[^\n]*\n','',rest)
rest='#include "kinoko/audio_runtime.h"\n#include "kinoko/audio_host.h"\n'+rest
rest+='''
/* Immutable host identities; audio code owns no retdec global VM state. */
const struct KinokoAudioHostSymbols* kinoko_audio_host_symbols(void) {
    static struct KinokoAudioHostSymbols symbols;
    symbols.critical_section_vtable = &g190;
    symbols.handle_table_vtable = &g198;
    symbols.decoder_vtable = &g208;
    symbols.device_error_message = g209;
    return &symbols;
}
'''
assert hashlib.sha256(rest.encode()).hexdigest()=='b11e4d3e4d6a6fe197d331d8324326ce7c6f3006001304cdb9901c0933ce590a'
methods={'lock':'Lock','unlock':'Unlock','set_volume':'SetVolume','play':'Play','get_status':'GetStatus','get_current_position':'GetCurrentPosition','stop':'Stop','set_position':'SetCurrentPosition'}
pat=re.compile(r'\(\(retdec_dsound_buffer_(\w+)_fn\)vtable\[\d+\]\)\s*\(')
owner=r'(?:track->(?:buffer|encoded_data|decoded_samples|decode_scratch|decoder)|g_retdec_bgm_track\.(?:buffer|encoded_data|decoded_samples|decode_scratch|decoder)|g_retdec_bgm_fading_tracks\[[^\]]+\]\.buffer|g_retdec_se_(?:entries\[[^\]]+\]|pool\.stream_slots\[[^\]]+\])\.buffer)'
handle=r'g_retdec_audio_(?:queue_event|stop_event|update_thread|loader_thread)'
BUFFER_FIELDS={0x1c:'playback_state',0x20:'ready',0x134c:'looping',0x1350:'start_time',0x1354:'fade_started',0x1358:'fade_duration',0x135c:'gain',0x1360:'fade_from',0x1364:'fade_to',0x1368:'volume',0x136c:'fade_pending',0x1370:'successor'}
texts=[];proto=[]
for f in fs:
 name=f['name']
 if name not in selection:continue
 text=REPLACEMENTS.get(name,s[f['start']:f['end']])
 if name not in REPLACEMENTS:
  m=mask(text);edits=[]
  for hit in pat.finditer(m):
   left=m.find('(',hit.end()-1);end=matching(m,left)
   a=args(text,left+1,end,m);method=methods[hit[1]]
   edits.append((hit.start(),end+1,f'({a[0]})->{method}({", ".join(a[1:])})'))
  for a,b,v in reversed(edits):text=text[:a]+v+text[b:]
  text=re.sub(r'(?m)^\s*void \*\*vtable;\n','\n',text)
  text=re.sub(r'(?m)^\s*(?:void \*\*vtable\s*=|vtable\s*=)\s*\*\(void \*\*\*\)[^;]+;\n','\n',text)
  text=re.sub(r'\s*if \(vtable == NULL(?: \|\| vtable\[\d+\] == NULL)*\)\s*return(?: 0)?;','',text)
  text=re.sub(r'if \(vtable != NULL(?: && vtable\[\d+\] != NULL)*\)', 'if (true)',text)
  text=re.sub(r' && vtable != NULL','',text)
  text=re.sub(r' && vtable\[\d+\] != NULL','',text)
  text=re.sub(r'if \(vtable\[\d+\] != NULL\)', 'if (true)',text)
  text=re.sub(r'hr = vtable != NULL\s*\? ([\s\S]*?)\s*: S_OK;',r'hr = \1;',text)
  text=text.replace('retdec_wave_format','WAVEFORMATEX').replace('retdec_bgm_track_state','BgmTrack')
  text=re.sub(r'\bvoid \*buffer\b', 'IDirectSoundBuffer *buffer',text)
  text=text.replace('g877 == NULL','!g_audio_device.device')
  text=re.sub(r'(?:retdec_release_dsound_buffer|free|stb_vorbis_close)\(\s*('+owner+r')\s*\);',r'\1.reset();',text)
  text=re.sub(r'('+owner+r')\s*=(?!=)\s*([^;]+);',r'\1.reset(\2);',text)
  text=re.sub(r'&('+owner+r')',r'\1.put()',text)
  text=re.sub(r'\b('+owner+r')(?!\w|\.(?:get|reset|put)\()',r'\1.get()',text)
  text=text.replace('ZeroMemory(track, sizeof(*track));','*track = BgmTrack{};')
  text=text.replace('ZeroMemory(&g_retdec_bgm_track, sizeof(g_retdec_bgm_track));','g_retdec_bgm_track = BgmTrack{};')
  text=text.replace('g_retdec_bgm_fading_tracks[index] = g_retdec_bgm_track;', 'g_retdec_bgm_fading_tracks[index] = std::move(g_retdec_bgm_track);')
  text=re.sub(r'g_retdec_bgm_fading_tracks\[index - 1\] =\s*g_retdec_bgm_fading_tracks\[index\];','g_retdec_bgm_fading_tracks[index - 1] =\n            std::move(g_retdec_bgm_fading_tracks[index]);',text)
  text=text.replace('g_retdec_bgm_fading_tracks[30] = g_retdec_bgm_track;','g_retdec_bgm_fading_tracks[30] = std::move(g_retdec_bgm_track);')
  text=text.replace('ZeroMemory(&g_retdec_se_pool, sizeof(g_retdec_se_pool));','g_retdec_se_pool = SoundPool{};')
  text=text.replace('ZeroMemory(g_retdec_se_entries, sizeof(g_retdec_se_entries));','for (auto& entry : g_retdec_se_entries) entry = SoundEntry{};')
  text=text.replace('(LPTHREAD_START_ROUTINE)function_40aaa0','audio_update_worker').replace('(LPTHREAD_START_ROUTINE)function_40aac0','audio_loader_worker')
  text=re.sub(r'CloseHandle\(('+handle+r')\);',r'\1.reset();',text)
  text=re.sub(r'('+handle+r')\s*=(?!=)\s*([^;]+);',r'\1.reset(\2);',text)
  text=re.sub(r'\b('+handle+r')(?!\w|\.(?:get|reset)\()',r'\1.get()',text)
  text=text.replace('(uint32_t)CoInitialize(NULL) >= 0','SUCCEEDED(CoInitialize(NULL))')
  enter='if (g_retdec_audio_lock_initialized)\n        EnterCriticalSection(&g_retdec_audio_lock);'
  leave='if (g_retdec_audio_lock_initialized)\n        LeaveCriticalSection(&g_retdec_audio_lock);'
  if enter in text and leave in text:
   assert text.count(enter)==1 and text.count(leave)==1,name
   text=text.replace(enter,'CriticalLock lock(g_retdec_audio_lock_initialized ? &g_retdec_audio_lock : nullptr);').replace(leave,'')
  for offset,field in BUFFER_FIELDS.items():
   pattern=r'\*\((?:float32_t|unsigned char|int32_t|uint32_t) \*\)\(intptr_t\)\(uint32_t\)\s*\(buffer \+ '+hex(offset)+r'\)'
   text=re.sub(pattern,'buffer->'+field,text)
  for offset,field in [(0x88,'active'),(0x94,'pending'),(0xa0,'retired')]:
   text=re.sub(r'\*\(int32_t \*\)\(g_retdec_audio_manager_state \+ '+hex(offset)+r'\)','g_retdec_audio_manager_state.'+field+'.head',text)
  text=text.replace('int32_t list = g_retdec_audio_manager_state.pending.head;', 'auto* list = g_retdec_audio_manager_state.pending.head;')
  text=text.replace('int32_t handle;','std::uint32_t handle;')
  text=text.replace('retdec_audio_manager_this() + 0x38','&retdec_audio_manager_this()->handles')
  text=text.replace('this_ptr + 0x38','&this_ptr->handles')
  text=re.sub(r'int32_t buffer(?=\s*(?:;|= retdec_audio_handle_lookup))','BufferRecord* buffer',text)
  if name in ('retdec_audio_method_40a6c0','retdec_audio_method_40a7f0','retdec_audio_method_40a950'):
   text=text.replace('int32_t this_ptr','ManagerRecord* this_ptr')
   text=text.replace('int32_t source','const char* source')
   text=text.replace('const char *path = (const char *)(intptr_t)(uint32_t)source;','const char *path = source;')
   text=text.replace('*(float32_t *)(this_ptr + 0xb0)','this_ptr->stream_gain').replace('*(float32_t *)(this_ptr + 0xac)','this_ptr->master_gain')
   text=text.replace('(int32_t *)(intptr_t)(uint32_t)buffer','reinterpret_cast<int32_t*>(&buffer->path)')
   text=text.replace('retdec_trace_i32("470220:buffer", buffer);','retdec_trace_i32("470220:buffer", address(buffer));')
  if name=='function_40a0f0':text=text.replace('return retdec_audio_manager_this();','return address(retdec_audio_manager_this());')
  if name in ('function_470220','function_470290'):
   text=text.replace('int32_t manager = retdec_audio_manager_this();','auto* manager = retdec_audio_manager_this();').replace('int32_t new_handle = 0;','std::uint32_t new_handle = 0;')
   text=text.replace('(int32_t)(intptr_t)&new_handle','&new_handle').replace('manager, g637, a1,','manager, g637, pointer<const char>(a1),')
  assert not re.search(r'\bvtable\b',mask(text)),name
 if name in exports:text=re.sub(r'^static ','',text)
 sig=text[:text.index('{')].strip()
 assert name in sig,name
 proto.append(sig+';');texts.append(text)
preamble=Path('tools/audio-migration-preamble.txt').read_text()
source=preamble+'\n'.join(proto)+'\n\n'+'\n\n'.join(texts)+'\n'
protos='\n'.join(sig.replace('float80_t','long double').replace('float32_t','float') for sig,n in zip(proto,[f['name'] for f in fs if f['name'] in selection]) if n in exports)
header='''#pragma once
#include <stdint.h>
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* C ABI entry points only; state and resources are owned by audio_runtime.cpp. */
'''+protos+'''
#ifdef __cplusplus
}
#endif
'''
(ROOT/'include/kinoko/audio_runtime.h').write_text(header)
(ROOT/'src/reconstructed/audio_runtime.cpp').write_text(source)
(ROOT/'src/decompiled/6kinoko_rebuilt.c').write_text(rest)
runpy.run_path('tools/audio-migration-finish.py')
for p in Path('tools').glob('audio-migration-*'):p.unlink()
Path(__file__).unlink()
print('Applied 82 typed audio functions; original loading/decoder timing retained.')

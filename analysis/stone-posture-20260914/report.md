# Moving-map posture investigation

Scope: supplied original game and this rebuild; restore original behavior. Preserve the previous orange/green platform fixes. No production resource changes.

E-imports: original PE32 SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155. IDA MCP HTTP session b7a894a8, survey.json and imports.json: file IO, Win32 windowing/GDI, D3D9/D3DX, timers, IME and COM; no static network/crypto imports. Desktop MCP has a separate session registry; repository HTTP MCP transport works.

E-assets: w3-c02b.act terrain-kabe has 648 tiles, IDs 1797..1805. Its unmodified inline script moves dst_y by cos(count/300.0*PI)*48, pauses 64 frames and reverses. Baseline fixture loads the original map, PAT and player scripts, invokes that inline script and runs the native manager motion pass. Fixture isolates a rider at (2000, layer offset+463) on the first stone at y=464; no gameplay window. Baseline is committed before execution.


## Follow-up after r3 feedback

The user clarified the animation is swimming/standing, not crouching; r3-quiet still reproduces. User owns live startup/gameplay tests, and asked us to preserve their running game. No game was terminated by the agent. No DAT/save was modified.

Original 40DC0A..40DC20 explicitly sets upward rounding. r1 retained that confirmed difference, but it is not a complete fix. Original 4689D0 collision oracle on identical 300 samples matches the C++ solver; nearest mode loses contact in 40 samples in both, upward loses none. Prior orange lift x87 intermediate arithmetic correction remains intact.

The former moving-map fixture only used a dry USA rider at x2000,y463 and manually copied a layer table output. Follow-up now uses native CActLayer publication and captured ACT Update callback, normal submerged TYPE_2HEAD, the original vector layer and native Actor.GetChipID, and four additional valid stone positions. Three exploratory spawns were invalid: y655 and y175 put the standing 63-pixel actor in 32-pixel gaps; x4300,y559 and x2950,y463 had no supporting stone. Those logs are retained, and these are not asserted as runtime regressions. No production collision logic was changed to accommodate them.

New C++ scope exception test revealed /EHsc's default non-throwing assumption at an extern C boundary. Explicit noexcept(false) now preserves exception propagation and rounding restoration. This does not alter C caller ABI. r5 checks return/throw/thread isolation passed.

E-live: user positioned r3-quiet on the stone. r3-stone-live.dmp was captured read-only before the stage timer expired. Main image base 00480000. Game thread 23772, active kinoko_run_game_math stack frame, CW=027F/MXCSR=1FA0 (nearest). Player 085471B8 at (1984.75,425.636566), take170, hitBottom=0, vy=.348, carry=0, parent=null. Report r3-stone-summary.txt. Snapshot inside native script calls alone cannot exclude transient FPU changes.

E-menu: user reopened r3-quiet to the menu. r3-menu-live.dmp locates thread37356 in PID2752; 300 read-only samples over 5 seconds in r3-menu-samples.json all show nearest rounding in x87 and SSE. Thus the loss is persistent before stage entry. No process memory/register was written. Sampling only briefly suspends and resumes the selected thread in a finally block.

Symbols for r3 were recovered by relinking its unchanged object/library files to r3-symbols (not replacing r3 EXE). The new link map has the same function RVAs; data globals differ by +16 in r3-quiet, verified against every matching .text relocation reference for manager/map/collision/frame globals. The .text hashes differ because of relocations and cannot be claimed identical. read_live_dump.py uses the verified r3 addresses plus the dump module's ASLR base.

Small hidden-window Direct3D9 probes tested device creation, texture creation, D3DXCreateTexture, first use on another thread, BeginScene/Clear/DrawPrimitiveUP/Present, and DirectInput polling. All preserve upward rounding after setup; these limited probes do not identify the application's first reset.

Build r10-audit corresponds to commit736b324. All18 CTests pass (including orange/green lifts, submerged moving map, C++ scope). Three DATs staged with size/SHA256 verification (r10-dat.log). The optional KINOKO_MATH_AUDIT build records the first rounding transition at native-call/frame boundaries and never resets the observed mode. User menu launch pending. This is a diagnostic build, not a claimed fix. Ordinary builds keep this audit disabled.

## r11: scoped BGM environment

E-first-change: r10 live fp-first-change.txt records native-after detail00BB7A40,
image base00B60000. Link map identifies PlayBgm/function_472080. The BGM prepare
implementation contained an unscoped _fpreset(), explaining persistent nearest
rounding. This is a rebuild audio integration defect, not a stage speed rule.
IDA MCP session db17a7af, original-bgm-load-r11.json confirms 40A6C0 branches on
queue flag a5: zero calls4096D0 synchronously; nonzero queues. Both paths remain.

Commit41ca9d4: C++ AudioEnvironmentScope saves/restores fenv_t around BGM preparation,
including early return and C++ exceptions, while preserving default decoder math.
No collision epsilon, posture override, script, velocity or loading-thread change.
The earlier original upward rounding and orange lift precision fixes are retained.

Fresh r11-quiet and r11-diag builds each pass18 CTests. Moving-map fixtures now run
actual archive/Vorbis/SFL/initial-fill code first, substituting only a memory sound
buffer. They compare the first PCM block against default rounding and check x87
and SSE restoration after success, invalid encoded input, device creation failure
and buffer lock failure. C++ tests also cover return/throw and exception flags.
Original orange/green lift and six moving-stone fixtures pass. Tests do not prove
live third-world completion. Both EXEs have all3 DATs staged and hashed; startup
and gameplay remain user-owned. No running game was stopped. r11-diag enables the
opt-in first-change audit; r11-quiet leaves it disabled. Build/test/staging logs
are r11-{quiet,diag}-*.log. Negative-control source is test-only and never shipped.

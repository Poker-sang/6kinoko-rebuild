# Phase 2 animation reconstruction

## Scope

Restore automatic playback of the second opening stage using the original DAT
resources and original executable behavior. The title screen is outside this
change. Preserve existing work and compare it against disassembly before use.
Reference: `C:/WorkSpace/6kinoko/6kinoko.exe`, `opening.mkv`, and the Squirrel
2.2.2 sources in `C:/WorkSpace/squirrel-2.2.2/SQUIRREL2`.

## E-imports / triage

- Original SHA256: `2DB975A408E260499D52126F25ECF2FBC529CAD52D1BFFC2A0B7CA2FF695F155`.
- IDA MCP survey: x86 PE, image base `0x400000`, entry `0x4ACA23`, 3963
  functions, normal `.text`, `.idata`, `.rdata`, `.data` sections. No packing
  evidence in this initial survey.
- Readable imports include file IO (`CreateFileA/W`, `ReadFile`,
  `SetFilePointer`, `GetFileSize`), windowing, Direct3D 9 / D3DX9_33,
  WinMM timing, GDI, IME, COM, threads and heap management.
- No direct network or crypto imports in this view. `LoadLibraryW` and
  `GetProcAddress` mean the static list does not prove absence of dynamic APIs.
- Tools: skill `start.ps1`, `open.ps1`; HTTP MCP session `48577128`.
  Registered MCP transport failed to resolve this session; HTTP MCP works.

## E-baseline-log

Existing `runtime-builds/p2-window-diag/retdec_trace.log` contains an access
violation after stage callbacks near frame 600:
`eip rva=0x755FC`, caller `rva=0x66A0C`, fault address `0x27`.
This is an observation from the previous run, pending fresh reproduction.

## Work Items

- [x] Read project instructions, IDA and reverse-engineering skills.
- [x] Verify original binary and record imports.
- [x] Reproduce the second-stage failure and identify the original behavior.
- [x] Restore the responsible implementation from assembly / Squirrel source.
- [x] Verify the remaining animation in diagnostic and no-log builds.
- [x] Stage and hash-check the three DAT files beside each executable.
- [x] Record final evidence and commit a backup (implementation: 54aed69).

## E-class-lifetime

Fresh Release reproduction stopped in SQTable::Get with receiver `3`.
The stack is `CreateEffect` (data/worldmap/Effect.nut) -> class call ->
4915B0 -> 48BFE0 -> 497A00. The Effect class pointer in the VM stack and
temp register points at memory already reused for a GDI object (PdcP header).
Original assembly at 492B71/492B73 and 4915F6/4915F8 saves the old destination
before assigning the new class/instance. The generated C decremented the
new object instead, leaving its live destination reference uncounted.
Restored assignment through the existing SQObjectPtr helper. Also restored
class attributes assignment and constructor argument placement in the VM's
absolute stack (without adding stackbase a second time).

Cross-check: Squirrel 2.2.2 sqvm.cpp CLASS_OP and CreateClassInstance, plus
`dumpbin /DISASM build-runs/phase2-cppvm/kinoko_squirrel_cpp_vm.dir/Release/sqvm.obj`.
The compiled CreateClassInstance saves the old type/data at +0x4D/+0x4F,
then retains the new instance and releases that saved old value.

## E-compile-environment

After the lifetime fix, CreateEffect ran the world-map implementation and
failed to resolve `effect0001`. The native CompileFile adapter discarded
argument 3, although original 419EA2 converts VM slot 3 to a Sqrat::Object
and 471B30 passes its value to 402D40. Restored this environment and its
reference ownership. The opening now resolves its own CreateEffect and
plays smoke, flying objects, Marisa's exit, and the automatic title handoff.
The title's own errors remain outside scope.

## E-cv2-pitch

`data/actor/item/op-door_0000.cv2` is 16-bit, width=65, height=100,
row_width=68, payload_size=0. Its DAT entry is 13617 bytes, matching the
17-byte header plus 68*100*2 pixels. The old decoder allocated/read only
65*100*2 and also uploaded rows with that wrong stride, causing diagonal
stripes. Original 414169 allocates using row_width; 414485/4144AD uses the
stored row width for source-row advancement. Restored both paths.

## E-opening-script

`tools/inspect_cv4.py` reads the original Squirrel 2.2.2 serialization format.
Generator captures baseTime immediately before PlayBgm(op.ogg), then gates
the door close at 8000 ms, SE 81 at 9415 ms, SE 82/smoke at 10571 ms,
and first flying object at 12200 ms. It emits the original 26 animation IDs,
three yield frames apart, then advances at 14222 ms. No script values changed.
InitFlying uses `vx=(rand()&255)/64.0-0.5`, `vy=-6-(rand()&255)/128.0`;
its callback increments rotate by 4.0 degrees each frame. Original Actor::Render
45F272 calls 405320 -> 404130/4040D0 (degree helpers). Restored those calls
in the actor renderer instead of interpreting degrees as radians.

## Validation

Use Release: RelWithDebInfo exposes an existing translated-code dependency
and produced diagonal shading/missing actors even before these changes.
No-log recording `notrace-pass1.mkv` confirmed full automatic playback after
the class/environment fixes, before the row-stride and rotation fixes.
The Computer Use native pipe is unavailable after retry/reset; FFmpeg gdigrab
is used to inspect the actual no-log window. Only one game instance may run
at a time because the original-compatible named mutex rejects another.

Final artifacts are in `analysis/evidence/raw/phase2-20260906/` (ignored by
Git): `diag-final.mkv`, `diag-final.log`, `notrace-final-repeat.mkv`, contact
sheets, intermediate evidence, original op.cv4, and `original-annotated.i64`.

- Release diagnostic and Release no-log builds both pass `archive_smoke`
  (1/1 each). Both use the three hash-checked DATs beside the executable,
  launched via `tools/run_staged.ps1` from the workspace, without a resource
  working-directory override.
- Final diagnostic log: 26 sound-83/flying-object emissions; zero VEH/SEH
  entries; zero opening-script or world-map Effect execution errors.
- Both final full-playback recordings show first-stage fade, walking/entry,
  correct door image, indoor pause, smoke, gradual flying-object rotation,
  Marisa's exit and the automatic title handoff. No-log process remains
  responsive and creates no `.log`, `.bmp` or `.dmp` files.
- A separate no-log recording (`notrace-final.mkv`) shows an early title skip.
  Keyboard input was not isolated; its cause is unverified. An unchanged-binary
  repeat (`notrace-final-repeat.mkv`) shows the full sequence and is the
  no-log full-playback evidence. The existing async-key fallback reads keys
  globally; input-focus behavior was not expanded in this change.
- Existing `block.nut` initialization and TitleLogo errors are recorded but
  remain outside this second-stage repair. Exact pixel identity and all
  translated VM paths are not claimed; validation covers the observed
  opening sequence in Release.

Final executable SHA256:

- Diagnostic: `AE6EE8F847423F9023D1ADFDC4C8B27290873D3720D33F70DB97E277FBA94587`.
- No-log: `4C6E92C8FA4E6FEEAAA9F47B2B6FD4BF9525F15471F8FB6AE9D30E804F85ADE9`.

## E-diagnostic-timing

The original script measures waits against `timeGetTime()` captured at BGM
start. Per-message file open/write/close made the diagnostic walking segment
take longer than the later absolute deadlines, compressing the indoor wait.
`retdec_entry.c` now buffers trace output behind a lock, preserving every VM
trace call and flushing at frame/error/exit boundaries. Whole lines from audio
and game threads no longer interleave. No animation time or trajectory is
hardcoded in the runtime. Disabling trace also disables render capture only.

Rebuild/replay (PowerShell):

```powershell
cmake --build build-runs/p2-repair-diag --config Release --parallel 4
cmake --build build-runs/p2-repair-notrace --config Release --parallel 4
ctest --test-dir build-runs/p2-repair-diag -C Release --output-on-failure
ctest --test-dir build-runs/p2-repair-notrace -C Release --output-on-failure
.\tools\stage_dat.ps1 -Executable runtime-builds/p2-repair-notrace/kinoko_retdec_rebuild.exe -SourceDir C:/WorkSpace/6kinoko
.\tools\run_staged.ps1 -Executable runtime-builds/p2-repair-notrace/kinoko_retdec_rebuild.exe
```

Reverse-engineering checklist: scope/imports recorded; static claims matched
to original assembly and Squirrel source/object disassembly; failures reproduced
under x32dbg; findings tested dynamically; original IDA comments saved; unrelated
worktree changes preserved. Stage-two fixes validated; title work deferred.

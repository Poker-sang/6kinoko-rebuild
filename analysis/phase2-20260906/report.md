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
- [ ] Reproduce the second-stage failure and identify the original behavior.
- [ ] Restore the responsible implementation from assembly / Squirrel source.
- [ ] Verify the remaining animation in diagnostic and no-log builds.
- [ ] Stage and hash-check the three DAT files beside each executable.
- [ ] Record final evidence and commit a backup.

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

## Validation In Progress

Use Release: RelWithDebInfo exposes an existing translated-code dependency
and produced diagonal shading/missing actors even before these changes.
No-log recording `notrace-pass1.mkv` confirmed full automatic playback after
the class/environment fixes, before the row-stride and rotation fixes.
The Computer Use native pipe is unavailable after retry/reset; FFmpeg gdigrab
is used to inspect the actual no-log window. Only one game instance may run
at a time because the original-compatible named mutex rejects another.

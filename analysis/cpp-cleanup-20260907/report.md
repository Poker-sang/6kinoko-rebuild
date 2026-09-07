# C++ migration and quiet runtime

Scope: improve the existing Windows x86 rebuild while preserving original
DAT/script/game behavior. The user authorizes refactoring, source integration,
confirmed bug fixes, runtime verification, and checkpoint commits. Reference
EXE and DAT files are read-only inputs; test outputs stay in the rebuild.

## E-triage / E-imports

Read reverse-engineering and ida-reverse skills, precedent-reverse, tool-index,
re-agent-workflow and repository AGENTS.md. Skill start.ps1/open.ps1 opened a
temporary original copy, session 53992cd7. The registered connector reports
Session not found; tools/ida_query.ps1 reaches the actual IDA MCP endpoint.
survey_binary and imports confirm PE32, base 0x400000, 3963 functions,
normal .text/.idata/.rdata/.data sections, and readable import tables.
SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Imports include CreateFileA/W, ReadFile, WriteFile, GetFileSize, SetFilePointer,
GetModuleFileNameA and SetCurrentDirectoryA; USER32/GDI32, D3D9/D3DX9,
WinMM, IMM32 and COM implement the Windows runtime. LoadLibraryW and
GetProcAddress allow dynamic imports. No packing obstacle was observed.
SendMessageA is window messaging. No malware/IOC investigation is in scope.

## Baseline

The existing moving-actors-notrace Release build passes CTest 2/2.
tools/exercise_game_window.py launches through run_staged.ps1 with all three
DATs beside the EXE and no working-directory override. A 15-second run with
Z at 2/5/8 seconds reaches the world map. External window capture:
runtime-builds/moving-actors-notrace/baseline-cleanup.png.
Existing user changes/deletions listed by git status are outside this work.

## Work Items

- [x] Read instructions, inspect baseline and original import evidence.
- [x] Replace the seven shared ABI adapters and nine-argument draw call with C++.
- [x] Integrate source-based Squirrel object/error ownership helpers.
- [x] Separate optional diagnostics and stop automatic render captures.
- [x] Remove superseded disabled decompiler bodies without changing live code.
- [x] Run focused contracts, diagnostic/quiet smoke and stage/hash DATs.
- [x] Record limitations and create checkpoint commits.

## E-ownership / Source Integration

IDA 48AC00 saves the old error pair before overwriting it, then releases that
old value. The reconstructed body instead read the newly overwritten pair,
leaking the old error. IDA 48AC70 likewise releases the previous error when
clearing it; the reconstructed branch inspected OT_NULL after replacement.
IDA 499A20 retains an interned string for its local SQObjectPtr before assigning
the last-error field; the reconstructed code omitted this retain and freed
the VM's new error when releasing its supposed temporary.

The C++ bridge uses the supplied SQObjectPtr constructor, assignment, Null
and numeric assignment operators. Original VM size, last-error/shared-state
offsets and reference-count offset are checked at compile time. Allocation
and string interning still use the reconstructed runtime. Virtual release
dispatches through the existing object's original-layout vtable.
Source references: sqobject.h, sqapi.cpp::sq_throwerror/sq_reseterror and
sqdebug.cpp::SQVM::Raise_Error. Also inspected the supplied source-build
analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm, CallNative +0x385:
Raise_Error(_lasterror) occurs after restoring the native call frame.

The include/ and squirrel/ directories, COPYRIGHT and HISTORY were imported
unchanged from ../squirrel-2.2.2/SQUIRREL2. Full source VM execution is still an
explicit experiment; this checkpoint makes no mixed-VM ownership claim.

## E-checkpoint-one

Both cpp-cleanup-quiet and cpp-cleanup-diag build as MSVC Win32 Release and
pass CTest 3/3. Both pass the extended original-resource contract command
documented in ../passage-ledge-20260907/report.md. Logs are in their build
trees (build.log and extended-contract.log). New checks cover 128 distinct
formatted errors, same-string replacement, error self-assignment and reset,
virtual final-release callbacks, and numeric assignment ownership. The ABI
contract makes 10,000 calls for each adapter through a real C++ vtable.

stage_dat.ps1 size/SHA256-checks all three DATs beside each new EXE.
run_staged.ps1 launches without a working-directory or data-directory override.
Diagnostic smoke-first.png shows active first-stage gameplay. Quiet
smoke-input.png shows first-stage gameplay after right/Z/X inputs at 22/23/24
seconds of a 30-second run. No auto-generated BMP, trace or dump appeared in
the quiet directory. That verification run enabled only the crash-dump switch.
An earlier 28-second script stopped with 'Game window lost focus' before its
final capture; no exit status or exception context was collected for that run.
Two subsequent quiet runs completed. Treat the first result as unresolved,
not proof of a fixed crash or proof of a regression.

The Windows entry and optional diagnostics now live in separate C++ modules.
Trace calls are retained; only their output sink defaults to quiet. Automatic
render-target capture code is removed. Normal execution installs no optional
diagnostic exception observer. KINOKO_TRACE and KINOKO_CRASH_DUMP enable
tracing/dumps without replacing the EXE or changing resource resolution.

Remaining work includes migration of the many receiver bridges and remaining
generated VM/native code, plus previously observed intermittent gameplay and
shutdown defects. Startup and contract evidence is not an exhaustive game pass.

## E-disabled-cleanup

The first checkpoint is 88d8a14. The mechanical cleanup removes 74 complete
literal #if 0 blocks with no live alternative, totaling 10,364 lines.
The quiet EXE .text SHA256 before and immediately after only this cleanup is
identical: 51204FD49904D6D283375ECB471A8DD2A3893B5653F637FAD347AB48C18F9E9B.
Original decompiler reference src/decompiled/6kinoko.exe.c remains available.

## E-native-cpp

Original IDA 45F760/45F780/45F7A0/45F7E0/45F800/45F810/45F820 confirms the
seven Actor wrappers now implemented with descriptive names in actor_methods.cpp.
ActorView preserves field widths/offsets, signed 64-bit flag extension,
per-layer chip-ID caching and calls to the existing manager/collision methods.
The original script names, including InterrputCollisionCallback, are unchanged.

Original 404770 and its existing reconstructed implementation anchor sprite.cpp.
Typed vertices retain the 28-byte stride and 148-byte CSprite field layout.
The three original vtable entries now use compiler-generated ABI adapters.
Pivot, scale, rotation and texture/FVF/draw ordering follow the existing path.
The native contract checks all three real vtable entries with fixed expected
corners, rotation, negative scale, and unchanged depth/color/texture coordinates.
Together with the first batch, 18 handwritten assembly blocks were replaced.

The original worker expired before annotation; skill open.ps1 reopened it as
f089ae12. Ownership/migration comments were saved through IDA MCP to
C:/rs-ida/2131fc9c-6kinoko.exe.i64. Reference EXE/DAT contents were not changed.

## Final Verification

Both final Release variants pass CTest 3/3 and the extended resource contracts.
Both final-stage.png window captures show active first-stage gameplay. The
window tool reports alive=true and responding=true for both successful runs.
The quiet executable also passes an explicit KINOKO_TRACE=1 output check;
that generated trace is archived separately from the ordinary runtime.
The imported 39 upstream files match the supplied source tree by SHA256.
All three DATs are staged and size/hash-checked beside each final executable.

During an earlier gameplay run the improved window tool recorded exit code
C0000005. The user explicitly confirmed this gameplay access exception also
occurred before the migration. It is retained as a known unresolved issue,
not classified as a new regression and not claimed fixed. A separate lost-focus
event occurred while the process was alive and responding. x32dbg was briefly
attached to a later test process and then detached; no crash was captured in
that debugging session and no full-path equivalence claim is made.

Final EXE SHA256:
- quiet: F06552B6053C70B05B14BE14E4CC05F6179334E082A743956C53C55C1C0D439F
- diagnostic: 96B643E05BA8C0A79988CC1ABB7D1D1CFE9118F20CB8378E7D7DD5177B07EE52

ADF P0: the migrated behavior has original/source anchors and executable
contracts. Diagnostic and quiet gameplay smoke passed. The remaining C VM,
receiver bridges and intermittent gameplay defects limit the equivalence
claim. No scripted gameplay rule, resource-loading workaround, or symptom
suppression was introduced. After the user requested an end to repeated
attempts, no further runtime tests were performed.

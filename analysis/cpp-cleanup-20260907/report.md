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
- [ ] Remove superseded disabled decompiler bodies without changing live code.
- [ ] Run focused contracts, diagnostic/quiet smoke and stage/hash DATs.
- [ ] Record limitations and create checkpoint commits.

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

# Runtime startup checkpoint

Scope: fix the current startup crash and confirmed related defects using the
original EXE/DAT loading and Squirrel behavior. Preserve existing worktree
changes. Diagnostic and no-log Windows gameplay verification is in scope.

## E-triage / E-imports

Read AGENTS.md, reverse-engineering/ida-reverse skills, precedent-reverse,
tool-index and re-agent-workflow. Skill start.ps1/open.ps1 opened the original
temporary copy as kinoko-startup. Use tools/ida_query.ps1 for the local IDA MCP
endpoint because the registered connector reports Session not found.
survey_binary (standard) confirms native PE32, base 0x400000, 3963 functions,
normal code/data sections and readable imports; no packing obstacle.
Original SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Imports include CreateFileA/W, ReadFile, WriteFile, SetFilePointer,
GetModuleFileNameA, SetCurrentDirectoryA, USER32/GDI32, D3D9/D3DX, WinMM,
IMM32 and COM. LoadLibraryW/GetProcAddress allow dynamic resolution.
SendMessageA and RegisterClassExA are window APIs, despite survey categories.
No IOC analysis applies to this local game reconstruction.

## Initial Evidence

- runtime-builds/stomp-asan/asan.31180 reports a startup heap-use-after-free:
  function_499b00 releases a string still referenced by VM last-error; the
  next missing-slot error calls function_499a20 and releases the stale pointer.
- runtime-builds/moving-actors-diag/retdec_trace.log contains a separate
  gameplay crash in the userdata destructor, with original object-type bits
  used as a pointer. Both observations require original assembly comparison.
- Existing uncommitted changes include sort restoration, diagnostic reads,
  a complete SQObject temporary and tests. They predate this investigation.

## Work Items

- [x] Read skills and record original survey/imports and existing evidence.
- [x] Reproduce the ASAN startup failure and distinguish build configurations.
- [x] Validate the existing Squirrel sort restoration and focused regressions.
- [x] Verify diagnostic/no-log startup and second-phase game screen.
- [x] Stage/hash all three DATs and record the checkpoint's limits.
- [ ] Finish the separate ASAN/last-error ownership investigation.

## Validated Changes In This Checkpoint

The existing uncommitted source changes restore the complete SQObjectPtr
delegate temporary in 493310 and remove writes below unrelated C locals.
473010 now reserves the full 12-byte SquirrelObject temporary. Array sort
4A25C0/4A3120/4A3530 uses explicit VM receivers, complete type/value pairs,
reference-counted swaps, and a retained receiver across stack relocation.
The pivot, comparison order and recursive partitions match the supplied
Squirrel 2.2.2 sqbaselib.cpp. Existing tests exercise default/script/native
comparators, instances, reference ownership, nesting, relocation and errors.

Diagnostic changes avoid post-release reads and fault-prone exception-stack
inspection, correct native-closure name offsets, and retain sound/userdata
diagnostics for the unfinished investigation. No gameplay or DAT rule was
introduced during this turn. Production source changes predate this turn;
the current turn validates and backs up that work at the user's request.

## Verification

Release builds moving-actors-diag and moving-actors-notrace both pass CTest
2/2 and the extended original-resource stage contract. Logs are in each
build tree's startup-final-contract.log; no-log build output is in
build-runs/moving-actors-notrace/startup-final-build.log.

stage_dat.ps1 copied and SHA256-verified all three original DATs beside both
EXEs. run_staged.ps1 launched each from the repository with no working/data
directory override. Computer Use screenshots showed the second-phase game
scene in both builds. The diagnostic process was also observed Responding=True.
The user confirmed that the latest ordinary build runs. This does not claim
an exhaustive gameplay or memory-safety pass.

EXE SHA256:
- moving-actors-diag: E2E9044361D9F6C0E29738D07EB0B9ABB93DEB6EAA41189D7316E9D132D30827
- moving-actors-notrace: 77E6E39552ED81A866A3B3D34E9A6DC47DE37F3C137CCC38C55C3FEC57D4B04D

## Remaining ASAN Evidence

Running cmake --build build-runs/stomp-asan --config Release --target
kinoko_retdec_rebuild confirmed that its target was already current for this
source. Therefore its failure cannot simply be attributed to an old EXE.
The unmodified ASAN launch exits with code 1. New logs asan-current.16948 and
asan-current.35992 beside that EXE report global-buffer-overflow in
retdec_string_assign_n at line 13606, called by WinMain at line 153250.
ASAN detects access beyond the four-byte generated global g554. The generated
split-object layout and this build's instrumentation require further work.
An earlier run, asan.31180, reached a separate last-error string use-after-free.
Neither finding is marked fixed in this checkpoint.

Original IDA 48AC00 reads the previous error type/data before replacement and
releases that old value. The current generated sq_throwerror body instead
reads the replacement through its aliased pointers. Original 499A20 holds
a temporary string reference before assigning last-error and releasing the
temporary; the current body lacks that first retain. These are static leads
for the next investigation, not changes included in this backup. IDA 499B00
confirms addref-before-release for error self-assignment. The supplied source
assembly in ../evidence/raw/phase3-20260906/squirrel-sqvm.asm, CallNative +385,
confirms Raise_Error(_lasterror) after restoring the native call frame.

P0 synthesis: ordinary startup and contract tests passed; ASAN failed.
The user redirected the task to checkpointing the running version. Preserve
the remaining evidence rather than claiming all crash paths are resolved.

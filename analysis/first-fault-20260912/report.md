# First-fault diagnostics for shutdown-fix diagnostic build

The user requests investigation only of shutdown-fix-20260912-diag and will
reproduce gameplay crashes after instrumentation. Preserve that EXE and its
existing trace. Baseline source is 99af33e1dcf4016cbc5c3651b72918686af9adfe,
baseline EXE SHA256 FFAF63CBFFD439FA7978DEC95229E33A6CB260EDA99D39E14D909FC0A49F614B.
The trace ends at 20:28:42 without seh/veh records or a local dump. No matching
Windows 1000/1001 event for this build was found. Older-build events are outside
this follow-up scope. Faster restart after exit is user feedback, not a newly
verified final exit code.

## Instrumentation

Optional KINOKO_CAPTURE_FIRST_CHANCE defaults off. The dedicated diagnostic
build enables it by default. Environment KINOKO_CAPTURE_FIRST_CHANCE overrides
the build default; capture defaults KINOKO_CRASH_DUMP to on, still overridable.
The observer records the first access violation, illegal instruction, integer
divide-by-zero or stack overflow and always returns EXCEPTION_CONTINUE_SEARCH.
It neither swallows the fault nor changes original script/VM error handling.

The independent pre-opened fault log records PID, TID, exception code/flags,
module base, EIP/ESP/EBP and other x86 registers, plus memory access operation
and target. WriteFile and FlushFileBuffers bypass the trace buffer/lock. A
first-fault interlocked gate prevents recursive capture. Stack overflow gets a
summary only at first chance because in-process dumping requires stack space.
Full-memory dumps for first-chance and final reported faults are separate.
Each name includes UTC timestamp, milliseconds and PID and uses CREATE_NEW;
existing captures are never overwritten. Logs record dump success/error.

First-chance capture may observe a fault later handled by the program. Its
existence is evidence of a fault, not proof of process termination. Fail-fast,
external termination or severe corruption can bypass in-process handlers;
this is not a universal guarantee of a dump. Game logic and VM trace call sites
are unchanged from the baseline. No routine README change is made.

## Validation

- [x] Implement optional first-fault summary/dump and unique artifact names.
- [x] Commit source before build/tests in a new directory.
- [x] Build diagnostic variant and run CTest including synthetic capture checks.
- [x] Stage three original DAT files, record provenance and hand over for user reproduction.

The capture test raises a handled access violation twice and reports a later
illegal instruction. It checks that ordinary handling resumes, exactly one
first-fault dump plus a distinct final dump exist, and MiniDumpReadDumpStream
contains the expected thread, code and access parameters. All outputs remain.
New directories: build-runs/shutdown-fix-20260912-capture and
runtime-builds/shutdown-fix-20260912-capture. Gameplay is delegated to the user
per their request; no autonomous repeated gameplay is planned for this batch.

## Result

Source checkpoint 01da5dc41292afdade736031f1a2a8ffefb5673a was committed before
building. Win32 Release with filtered trace and first-chance capture enabled
passes CTest 4/4. The capture test log records both saved=1/error=0 dumps,
correct access target 12345678, and normal test shutdown. MiniDumpReadDumpStream
verified the recorded thread, codes and parameters. Synthetic test artifacts
remain under the runtime tools directory, separate from future game captures.
All three DAT archives pass size/SHA256 staging. No game was launched in this
instrumentation batch; user reproduction is the pending next action.

EXE SHA256: 7D49FE99ED8C8F8B566BFAE40634928845159098376E34B9A3C29540FEBC09E3.
Game/decompiled/reconstructed source and both READMEs are unchanged from 99af33e.
This artifact adds diagnostic evidence collection; it does not claim to fix
the unresolved gameplay crash. Existing shutdown-fix diagnostic files remain.

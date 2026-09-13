# Exit cleanup restoration

Scope: user's explicit follow-up after road-unlock confirmation. Road repair
and its evidence were committed first as 258a32c. Preserve that behavior,
existing user deletions, original DATs/saves, and every earlier test artifact.
Carry E-imports and IDA MCP session f9ff9241 from ../road-unlock-20260913/report.md.

## Original fault and first correction

Road diagnostic PID 29720's WER dump reaches function_450020 from 465F70/469680.
450020 and 465F70 lost receiver/free arguments and contain writes through
fabricated stack temporaries. IDA 465F70 confirms payload-data delete,
source object's virtual destructor at +16 with flag 1, runtime destructor,
payload delete, list reset, then node deletion. The restored C++ stage_cleanup
module preserves this sequence. The runtime destructor uses the already
restored explicit-receiver implementation shared with map unloading.

Compatibility ownership detail: current BeginStage borrows its source ACT;
the original can own a separate ACT. Detach an identical borrowed runtime ACT
pointer before source destruction, preserving independent owned ACT cleanup.
This prevents reading/destroying an already freed owner during global clear.
Tests cover two payloads, real runtime construction, distinct receiver values,
source/data ordering, shared ACT detachment, list sentinel and repeated clear.

First candidate 893c7ae, cleanup-20260913-r1-diag: CTest 4/4. Actual game exit
passed the previous fault and exposed the next one: first-chance dump
fault-20260913-024916-753-p31900-first.dmp records c0000005 in
function_470890+0x84, writing 0x760d0174 through a lost audio receiver.
The candidate and its staged DATs, dump and trace remain intact.

## Sound and script cleanup

IDA 470890 -> 40B840 / 40B6F0 / 40A5F0 / 40A460 describes sample owners,
streaming pool, BGM handle and worker teardown. Current runtime ownership is
the repaired SE pool/BGM track records. Reuse their existing 40B3A0 cleanup,
which joins workers before releasing buffers, then free/reset the non-owning
sound ID lookup tree. Tests check every fake COM buffer receives one Release,
SE buffers receive Stop, pool/lookup state empties, and repeat cleanup is safe.

IDA 470F30 loads four distinct SquirrelObject receivers at 513C58, 514484,
513C88 and 5144A8. Restore these calls rather than clearing the same unrelated
global four times. IDA 4A8C50 deletes the lazily allocated root wrapper,
conditionally clears 5149EC, then zeros the VM global. The rebuilt 4A8CC0
constructs that wrapper as a 12-byte SquirrelObject; restore its explicit
reference release and free. Tests retain an independent root, populate all
five wrappers, run shutdown twice and check each pair is null.

## Validation in progress

Candidate 654c903: cleanup-20260913-r2-diag and r2-quiet, independent build/run
directories. Final build/contract/game-exit results will be appended here.
No VM execution-backend switch, gameplay rule, DAT edit or trace-call stripping.

r2 contracts and original smoke bytecode passed in both modes. Live diagnostic
exit advanced past ACT and audio cleanup but faulted in the inlined
function_45da40+0x90 writing 0x00655000, the relocated g644 address. Its startup
retdec_guard_g644 unconditionally VirtualProtect'ed that page PAGE_READONLY.
This leftover diagnostic trap rejects the original legitimate g644=null
shutdown operation. Remove the guard, its initialization and bookkeeping;
leave VM trace call sites and optional output/dump tooling intact. Retain r2
first-chance dump fault-20260913-025537-472-p24872-first.dmp. Next batch r3.

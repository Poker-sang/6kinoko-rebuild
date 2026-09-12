# GC chain corruption from gameplay dumps

Scope: continue fixing the user's shutdown-fix-20260912-diag/capture gameplay
crash. No sound suppression, stack-size workaround, resource-loading rule or
README change. User authorized source-based Squirrel replacements and C++
migration. Carry forward read skills, original hash/import evidence and
instructions from ../first-fault-20260912/report.md and prior reports.

## Dump Evidence

The capture EXE hash remains
7D49FE99ED8C8F8B566BFAE40634928845159098376E34B9A3C29540FEBC09E3.
Two retained first-fault dumps are in runtime-builds/shutdown-fix-20260912-capture:
fault-20260912-132551-410-p32704-first.dmp and
fault-20260912-132915-349-p24340-first.dmp. Both record c0000005.

The first faults in dsound.dll+5e520 while PlaySE (function_470980) calls the
buffer's Play method. A DirectSound internal object points to 0x0d99a2d0,
which is a recovered SQClosure (_g66 vtable) with GC links to a DirectSound
object. The second faults executing heap address 0x0d845e78 from a DirectSound
thread; that address contains a dsound.dll+2384 vtable, not code. Its shared
state at 0x080662a0 links this DirectSound object into the Squirrel GC chain
after six normal nodes. Adjacent finalized Squirrel object 0x0d8b5470 has
_g65 vtable, ref bits 0x80000001 and links to that same DirectSound object.
These dumps establish foreign objects in the GC links; PlaySE ABI inspection
did not identify a calling-convention error in the game's public COM call.

## Confirmed Source Defects

retdec_gc_mark_value takes int32_t *chain_head but passed &chain_head to
function_49a140. This updates a local parameter and links to the caller's stack
storage instead of updating the actual live head. The isolated --gc-link-probe
fails on unmodified runtime code: head_storage[0] is not the marked object.
Pre-fix test commit ca7d688, artifacts in build-runs/gc-link-before-20260912 and
runtime-builds/gc-link-before-20260912. The test does not need graphics/audio.

Additional differences from supplied Squirrel 2.2.2 sqstate.cpp CollectGarbage:
the rebuild omitted _gc_chain = tchain after unmarking, saved _next before
Finalize could release/relink neighbors, and counted only zero-ref releases
instead of all visited garbage nodes. SQVM also shadows _sharedstate at +140,
whereas the old generic mark splice always read the base field at +20.

Upstream sqobject.cpp END_MARK and sqstate.cpp AddToChain/RemoveFromChain define
the correct splice. Supplied source-build squirrel-sqvm.asm shows the same
source chain operations in VM constructors/destructors. IDA skill scripts
opened sessions 0d741dd5 and 8ac3020a, but workers became unreachable before
the requested GC queries completed. No new binary annotation is claimed;
the supplied Squirrel source is the authoritative implementation reference
for this already-identified library component.

## Implementation and Validation

squirrel_gc_bridge.cpp uses the actual upstream SQCollectable link operations
and typed SQSharedState/SQVM layouts with offset assertions. It restores the
original sweep/next/unmark/head-publication ordering. Type-specific marking
and finalization remain the current C compatibility helpers; no full C++ VM
backend is enabled. Existing trace call sites remain.

- [x] Inspect both user dumps and isolate a deterministic mark-head failure.
- [x] Implement typed source-based chain splice and sweep in C++.
- [x] Commit before a new diagnostic/capture build and run regression contracts.
- [x] Stage DATs, preserve all outputs and hand over for gameplay reproduction.

Tests will verify caller-head/canary integrity, 32 collections of cyclic tables,
arrays and closures, preservation of rooted values, root VM presence, cleared
mark bits, and both directions of the GC chain after allocation and collection.
Use build-runs/gc-fix-20260913-diag and runtime-builds/gc-fix-20260913-diag.
The link defect is proven; game-level stability still requires user rerun.

First candidate ba74093 compiled successfully and passed the mark-head probe,
but the multi-cycle contract script failed to compile at its second same-line
local declaration before running GC. Change the fixture to use the established
root-slot/delete syntax. Keep all initial artifacts; next test batch uses
gc-fix-20260913-r2-diag directories. No runtime change is made for this syntax
adjustment. CTest's other three tests passed in the first candidate.

The r2 fixture reached real GC and produced heap-corruption failure c0000374.
The retained WER dump kinoko_stage_contract.exe.20168.dmp resolves the failure
to function_491bf0 called by the sweep's virtual Release. Its generated C body
passes an uninitialized local to free instead of receiving ECX. Replace only
the SQVM Release vtable slot with a compiler-generated entry that invokes the
supplied SQVM::Release/destructor; source layout is asserted. Preserve r2
artifacts. The next batch uses gc-fix-20260913-r3-diag directories.

The r3 run completed collection cycle 0, then the next cycle exposed the same
lost-ECX free in SQArray::Release (48D430), confirmed by WER dump
kinoko_stage_contract.exe.22364.dmp. The recovered array virtual release slot
now calls upstream SQArray::Release with an explicit receiver; size/vector
offset assertions cover its layout. Other collectable release slots already
have receiver-bearing adapters. The next batch uses gc-fix-20260913-r4-diag.

The r4 WER stack (kinoko_stage_contract.exe.28040.dmp) exposed the companion
SQArray scalar-deleting destructor slot: upstream sq_delete's destructor call
still dispatches virtually to that old C entry. Restore both VM and array
destructor slots as well as Release, explicitly bypassing old destructor
dispatch. SQArray's private destructor is reproduced using its source unlink,
vector destruction and qualified base destruction; SQVM uses its qualified
upstream destructor. Next batch: gc-fix-20260913-r5-diag.

## Final Result

Final source checkpoint b16a9a8423d0773d0cbe5b855baa2cef73a7981f passes CTest 4/4 in
gc-fix-20260913-r5-diag. This includes the previously failing isolated head
probe, 32 rounds of cyclic garbage collection, rooted array/closure preservation,
root VM membership, mark-bit clearing, bidirectional GC link consistency, all
existing native contracts, repeatable shutdown and diagnostic capture checks.
The root/head defect was reproduced before fixing it; the VM/array release and
destructor failures were reproduced in intermediate candidates before correction.

Final EXE SHA256: 50FE40D635C63325EFD48316D2A7D0196F1A50714CE182D1DFBBE690DE717203.
Every candidate directory contains a source/hash/test validation manifest;
all five candidate game EXEs have verified DAT copies beside them. Earlier
build trees, logs, dumps and test failures remain. The final EXE retains
first-chance exception capture and sound. No game was launched in this batch,
in accordance with the user's offer to reproduce gameplay. Stability under
actual gameplay remains pending user validation; do not equate contract passes
with an exhaustive crash fix.

ADF P0: the user dumps show foreign DirectSound objects linked with Squirrel
objects; deterministic contracts confirm concrete GC and destructor defects.
Upstream Squirrel source anchors the corrections. The connection to every
possible gameplay failure remains unproven until rerun. README and resource
loading are unchanged, and no original binary/dump is committed.

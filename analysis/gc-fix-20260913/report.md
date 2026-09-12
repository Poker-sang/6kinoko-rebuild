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
- [ ] Commit before a new diagnostic/capture build and run regression contracts.
- [ ] Stage DATs, preserve all outputs and hand over for gameplay reproduction.

Tests will verify caller-head/canary integrity, 32 collections of cyclic tables,
arrays and closures, preservation of rooted values, root VM presence, cleared
mark bits, and both directions of the GC chain after allocation and collection.
Use build-runs/gc-fix-20260913-diag and runtime-builds/gc-fix-20260913-diag.
The link defect is proven; game-level stability still requires user rerun.

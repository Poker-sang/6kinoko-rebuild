# ACT resource C++ migration

Scope: continue the readability/assembly cleanup after 7848407. Preserve the
original ACT/script loading and state transitions. Carry forward the read
reverse-engineering and ida-reverse skill instructions and E-triage/E-imports
from ../cpp-cleanup-20260907/report.md, including original SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
The supplied Squirrel source and source-build assembly remain reference inputs.

The user limits each game smoke run to entering stage one, attempting a jump,
and exiting once an enemy is visible. Do not continue playing or investigate
the previously known gameplay access exception as part of this smoke check.
Each validation batch must have a committed source checkpoint before testing.
Use distinct build/runtime directories, record the source commit and preserve
all EXEs, build files, logs and captures, including failed attempts.

- [x] Inspect the current ACT implementations and call sites.
- [x] Confirm original method arguments and field access through IDA MCP.
- [x] Move ACT resource control methods to C++ without changing their behavior.
- [ ] Build/test and perform one bounded smoke run per diagnostic/quiet variant.
- [ ] Record results and commit a checkpoint.

## Original Evidence

Skill start.ps1/open.ps1 opened original session 9d42ef70. IDA MCP was reached
through tools/ida_query.ps1. Original 451610 writes time at resource+4;
451620 adds the ACT resolution from **resource+4; 451630 divides time by that
resolution. GetCurrentTime is registered to 4A9A30, a read of resource+4.
451590 forwards milliseconds to Sleep; 4515A0 stores timeGetTime()+delay at +100.
450D80 checks the active byte at +8, locks the critical section at +20, clears
the eleven words at +108..+148, clears pending draw/layout entries, then unlocks.
The C++ methods preserve the existing null/zero-resolution compatibility guards.
Clock addition uses explicit unsigned arithmetic for the original x86 wrap.

4515C0 disassembly confirms virtual calls through the optional object at +12
and the holder at +0; 4515F0 calls the optional object's vtable+32 after clearing
the wake/suspend fields. Existing C comments incorrectly described these as
pure field operations. This batch corrects those comments and replaces only
the incoming assembly adapters for Suspend/Resume. Their existing compatibility
bodies remain in C until the object dispatch is reconstructed and validated.
No new original-equivalence claim is made for those two bodies.

Existing VM trace call sites are retained. The ACT increment diagnostics call
the same out-of-line retdec_trace_i32 helper. Original scripts, resource loading,
and stage rules are unchanged. Squirrel is not modified; the supplied source
assembly CallNative +0x385 was consulted for the native/VM call boundary.

## Pre-Test Checkpoint

Commit this source before building or running the new contracts. The commit
is recorded in the build/runtime validation manifests after committing.
Both directories are new and must not reuse the cpp-cleanup output paths:

- build-runs/act-cpp-20260908-quiet and runtime-builds/act-cpp-20260908-quiet
- build-runs/act-cpp-20260908-diag and runtime-builds/act-cpp-20260908-diag

The focused contract exercises real thiscall-compatible entries, signed frame
division, counter wraparound, missing ACT/resolution guards, deferred wake time
and active-stage cleanup. Run the existing CTest suite after building, then one
bounded game smoke check per variant. Retain all successful and failed outputs.

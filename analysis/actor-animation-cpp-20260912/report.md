# Actor animation C++ migration

Scope: animation selection, bounds, frame progression and synchronization.
Carry forward instructions and E-triage/E-imports from
../actor-state-cpp-20260912/report.md. Original hash remains
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Reference inputs are read-only. No DAT or script loading changes are intended.

- [x] Inspect animation selection, ticking and existing contracts.
- [x] Verify original SetTake/Step and field layouts through IDA MCP.
- [x] Consolidate the animation state operations into a C++ module.
- [ ] Commit before building and validate both variants in fresh directories.
- [ ] Retain artifacts and record bounded smoke results.

The user accepts larger related call-chain migrations and requests no routine
README updates. Preserve prior outputs and stop each game smoke at stage one,
jump attempt and first visible enemy; user feedback may close validation.

## E-animation

Previous IDA service had stopped; skill start.ps1/open.ps1 reopened the original
as session 0d6ad0ef. analyze_batch inspected complete disassembly for 462280
and 45E120, including call/data xrefs. SetTake is registered at 460FE6 and
Actor.Step is called from 4641D0 at 464278. Comments saved to
C:/rs-ida/be5d2829-6kinoko.exe.i64.

462280 resets take/index/timer before lookup, including missing take IDs. Bounds
are signed integers at animation+28..40, with -0.5/+0.5/+1 edge adjustments.
Actor+168/172/176 supply scales, +272 facing, +240/+244 position. No-bounds
animations clear local bounds and anchors before deriving point world bounds.
45E120 snapshots the take before invoking the script, then only advances if
the take remains the same and a current frame exists. Frame duration is signed
int16 at +240 in a 248-byte frame; looping triggers on equality with frame count,
otherwise the last frame is held. 462250 updates both frame pointer aliases.

actor_animation.cpp now owns these state operations and the previously migrated
SyncAnimation state copy. Partial layout structs have size/offset assertions;
SetTake's original ECX/ret-4 entry uses a compiler-generated fastcall bridge.
Tree lookup and script callback/error diagnostics remain in their existing C
helpers. Existing missing/empty animation guards and double intermediates in
the bounds calculation are preserved. This is not a claim of full x87 precision
equivalence for arbitrary non-game floating-point inputs. Shared frame selection
uses explicit uint32 arithmetic for original x86 wrap. No Squirrel VM changes;
carry forward supplied source/assembly ownership evidence from the prior batch.

Contracts cover positive/zero/negative durations, loop and terminal hold,
changed take, absent current frame, absent/empty animation, counter wrap and
the original equality-only loop boundary. Real SetTake ABI, signed/scaled/facing
bounds, missing IDs and no-bounds script calls extend existing native contracts.
The previous real Actor SyncAnimation/RefTable tests now exercise this module.

## Pre-Test Checkpoint

Commit before builds/tests. Use build-runs/actor-animation-cpp-20260912-{quiet,diag}
and runtime-builds/actor-animation-cpp-20260912-{quiet,diag}. Keep all previous
and new artifacts, with source commit and EXE hashes in validation manifests.
README stays unchanged.

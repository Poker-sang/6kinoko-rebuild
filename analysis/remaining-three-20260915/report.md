# Receiver recovery batch — 2026-09-15

Scope: continue the three receiverless compatibility entries under the user's
authorization; recover original behavior and migrate verified paths to C++.
Preserve R11 BGM floating environment and existing lift behavior. Do not close
the user's game. Retain all old artifacts; fresh test directories for each batch.

E-imports / original SHA256 and PE32 identification carry forward from
../stone-posture-20260914/report.md. Original and DAT/save files are unchanged.
IDA MCP transport initially closed; the skill start/open scripts restored the
HTTP service and opened a temporary original copy in session e0959742.
Original pseudocode, xrefs and instruction listings are retained here.

## Verified changes

- 491500 GETVARGV_OP: restore explicit VM/target/index/CallInfo at the C++
  boundary. Use Squirrel 2.2.2 source operation for numeric conversion, negative
  and upper bounds, error messages and owned assignment. The live interpreter
  now uses this operation too, instead of losing the specific error message.
- 494990 Clone: restore the instance temporary assignment and array target's
  previous-value release. C++ SQObjectPtr temporaries implement balanced
  acquire/release. Preserve reconstructed table/instance allocation adapters,
  original _cloned order/status handling, and the existing game dispatcher.
- 494DA0 FOREACH_OP: restore `refpos = key = iterator`, temporary destruction,
  and proper string/class/generator receivers. Source Next methods retain
  original iteration and jump semantics. Metamethods still use the game VM.
  Output stack slots are reacquired after callbacks can relocate VM storage.

Source references: supplied ../squirrel-2.2.2/SQUIRREL2 and vendored matching
2.2.2 sqvm.cpp::GETVARGV_OP/Clone/FOREACH_OP, sqarray.h and sqclass.h.
No full C++ Execute backend switch. Existing operation trace calls retained.
Both sqvm.cpp files have SHA256
40B508DF323452D3C5722A2CD457A1273650ECD22A0EF037FBCC589BFE164F1D.

The assembly confirms 491583 sets ECX to the output argument before 489F50;
494A35 sets ECX to the local cloned-value pair. 494DA0's two assignments are
`refpos = key = itr`, followed by one local iterator destruction on each exit.
Seven receiverless call expressions were replaced across these three bodies
(six physical source lines because two assignments shared one line).

## Remaining scope

This batch does not remove or rename the three simplified compatibility entries.
Their other old generated callers still require auditing. In particular the
thread-wakeup chain also reaches the old Execute implementation; correcting just
one assignment there would not repair that chain. No claim that all three
entries or all their callers are finished.
4A9D70's old calls include the CSV binding (403000), obsolete actor/class
helpers and compiler-generated exception cleanup thunks. Restoring those
requires the containing object's layout and each unwind frame, not a guessed
global VM/object. This batch investigated that boundary but does not claim a
new 4A9D70 caller repair. The three original candidates remain honestly listed.

## Validation

r1 at commit 840a1e8 built both variants; each passed 17/18 tests. New operation
cases passed, but GC chain validation rejected source-created SQArray vtables.
This was a migration defect, not a flaky test. Keep that failed batch for audit.

r2 at commit 0d50c66 retains the reconstructed array constructor/vtable and
uses C++ vector copy plus SQObjectPtr ownership. Both quiet and diagnostic
builds pass all 18 CTests, including GC collection/chain integrity, original
stone/dry/submerged placements, orange/green lifts and floating environment.
New cases cover array/instance/table clone and callbacks, overwritten target
release, aliasing, repeated instance iteration, strings, vararg conversion and
all three vararg error messages plus invalid _nexti output.

Artifacts:

- build-runs/receiver-20260915-r2-quiet and -diag (independent build trees).
- runtime-builds/receiver-20260915-r2-quiet and -diag (EXEs and DAT files).
- r2-{quiet,diag}-{configure,build,tests,dat}.log in this report directory.
- r2-exe-hashes.json records both EXE SHA256 values.

All three DATs copied beside each EXE and verified by size/SHA256. No user
game was started, stopped, attached or switched; user retains startup/gameplay
validation as previously requested. Thus no interactive smoke or complete
game-equivalence claim. Prior r5 feedback does not validate this new build.

Reproduce with top-level CMake, Win32, Release, KINOKO_REFERENCE_DIR pointing
to C:/WorkSpace/6kinoko, KINOKO_RUNTIME_DIR pointing to a fresh run directory,
and KINOKO_RETDEC_DISABLE_TRACE=ON (quiet) or OFF (diag); build and run CTest.
stage_dat.ps1 copies/verifies runtime resources. The migration script documents
the targeted source transformation from pre-batch commit 09e9626; it is not
intended to be rerun on an already migrated source file.

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

## Remaining scope

This batch does not remove or rename the three simplified compatibility entries.
Their other old generated callers still require auditing. In particular the
thread-wakeup chain also reaches the old Execute implementation; correcting just
one assignment there would not repair that chain. No claim that all three
entries or all their callers are finished.

## Validation

Pending fresh quiet/diagnostic builds and offline checks. Added cases cover
array/instance/table clone and callbacks, overwritten target release, aliasing,
repeated instance iteration, strings, vararg conversion and errors.
User retains control of startup/gameplay validation.

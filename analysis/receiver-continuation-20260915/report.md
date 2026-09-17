# Receiver continuation — 2026-09-15

Scope and E-imports carry forward from ../remaining-three-20260915/report.md.
Original game/DAT/save files unchanged; original PE32 SHA256 remains
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA skill start/open restored the expired session as 4798af8a on a temp copy.
Evidence: original-0x*.json from IDA MCP; supplied Squirrel 2.2.2
sqvm.cpp::Execute/Suspend, sqapi.cpp::sq_wakeupvm and sqbaselib.cpp thread APIs.

## Thread chain

Restore the explicit VM throughout suspend, newthread, thread.call/wakeup/status.
Use C++ local SQObjectPtr ownership and assignment to the parent's _lasterror.
Retain the reconstructed VM constructor/vtable and live interpreter; do not
enable the source Execute backend. Reacquire parent VM after nested child calls.

Original Execute stores _suspended_root at +152, target at +156, traps at +160,
varargs at +164, and immediately returns on native suspension. Recovered code
had stored target at +152, cleared +156, and continued execution. Resume now
restores the suspended frame in the same current dispatcher. Remove the old
495360 implementation only after confirming its last caller was replaced and
there are no remaining function references (baseline source retains original).

Validation completed in fresh thread-20260917-r6 quiet/diag builds; see below. Existing
stone/lift/BGM fixes unchanged. Added coroutine result/error/vararg/trap tests.
All older artifacts preserved. User controls interactive game startup.

## Validation and remaining work — 2026-09-17

Final code/test commit: cbb9f6e. Both Win32 Release variants in
build-runs/thread-20260917-r6-{quiet,diag} pass all 18 CTests.
Runtime EXEs in runtime-builds/thread-20260917-r6-{quiet,diag} each have
three DATs staged beside them, verified by size/SHA256. See r6 logs and
r6-exe-hashes.json. Stage contract output/capture is disabled in both builds;
these tests do not replace interactive startup of the two runtime EXEs.

Failure history:
- 20260915 r1 (32f53d3): child stack corruption from main-VM selection.
  f885d8f scopes explicit child receivers and restores the previous VM.
- 20260915 r2 (f885d8f): second thread script failed; diag fastfailed 0xc0000409.
- 20260917 r3 (600a328): diagnostic revealed missing newthread binding.
- r4 (a674ec6): root still contained newthread, but its string refcount was -1.
- Original 48C580 retains the new name and releases the saved OLD name.
  RetDec released the newly overwritten fields instead, acquiring no net
  reference for a fresh name. Child Init re-registration destroys old closures
  and therefore invalidates names still held by the root table.
- 1ff1a08 delegates 48C580 to source C++ sq_setnativeclosurename. IDA evidence:
  original-native-name-20260917.json and original-init-20260917.json, session
  0dea0d99. Child Init really registers base functions; do not suppress it.
- r5 stage contract passed. r6 adds direct name ownership, same-name assignment
  and replacement checks; both full suites pass. Former fastfail not observed.

Tests also cover suspend/wakeup values, varargs, traps, child error transfer,
idle wakeup errors and forbidden suspension through nested native calls.
Existing stone/water, orange/green lifts, math and GC contracts pass.

NOT COMPLETE: receiverless 489F30, 489F50 and 4A9D70 still exist. Current
source has 396, 18 and 243 call-like text occurrences respectively, including
prototypes/definitions; these are NOT reachable-call or correctness counts.
The thread chain and obsolete 495360 dispatcher have been addressed, but
ReadCSV's by-value receiver, old compiler paths and cleanup thunks remain.
Do not cosmetically rename/delete stubs to obtain zero counts.

No game started, closed, switched or attached. Original game/resources and
prior stone R11/lift/BGM fixes preserved; all older test artifacts retained.
Interactive testing remains with the user: both variants, first stage, jump,
observe an enemy and exit. Pause after handing over paths as requested.

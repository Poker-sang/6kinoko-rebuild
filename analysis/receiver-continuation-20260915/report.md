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

Validation pending in fresh thread-20260915-r1 quiet/diag builds. Existing
stone/lift/BGM fixes unchanged. Added coroutine result/error/vararg/trap tests.
All older artifacts preserved. User controls interactive game startup.

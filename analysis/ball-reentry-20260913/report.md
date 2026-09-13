# Enemy reentry and death investigation (in progress)

User scope: restore original behavior for FairyOtedama ball reentry and ordinary
enemy death, particularly stomps and rolling kedama hits. Latest instruction:
do not perform further live gameplay; hand a concrete build to the user for testing.

Read ida-reverse and reverse-engineering skills, precedent-reverse, tool-index
and re-agent-workflow. IDA was started/opened with the supplied skill scripts.
The connector and local HTTP supervisor differ: use tools/ida_query.ps1 with
local MCP database 2280133a. Original SHA256 from survey is
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155, x86,
3963 functions. Carry forward E-imports in ../evidence/E-triage.md; this session
rechecked readable KERNEL32 imports through MCP. The target is the same local
game, with file/window/graphics/input/COM imports and dynamic DLL loading.

## Evidence and limits

Original DAT bytecode has FairyOtedama.OnReset release four old balls; its first
update creates four new actors, with weak references to the parent. The normal
EnemyCollision_Damage initializes vy=-5, selects EnemyUpdate_Dead, and that
update adds GRAVITY and caps MAX_FALL_SPEED. There is also an original
EnemyUpdate_BlowOff_Dead that intentionally moves up; do not erase that path.
Supplied Squirrel source and the source-build assembly in
../evidence/raw/phase3-20260906/squirrel-sqvm.asm are available for VM comparison.

IDA inspected Actor Init 45E5E0, Reset 45EB00, Clear 45FB90, Step 45E120,
SquirrelFunction call 45DFB0, rawset 48CB10, instance Set 489FA0, IsFalse 490D50,
and ActorManager Update 4641D0. Reset recreates the script instance and clears
the old user, and Step clears failed callbacks. Neither original manager loop
adds a special exclusion for actors flagged for deferred release during the loop.
Do not invent that exclusion as a fix.

Offline candidates preserve every build/run directory:

| Commit | Directory prefix | Result |
| --- | --- | --- |
| 3d2b734 | enemy-20260913-before | Fixture omitted player.direction |
| 2f0e595 | enemy-20260913-before2 | Fixture omitted PlaySE |
| 46e815a | enemy-20260913-before3 | Four delayed-reset generations passed |
| 13c305b | enemy-20260913-before4 | Camera teleport triggers immediate-reset error |
| 664936a | enemy-20260913-before5 | Reset hook runs; single gravity step passes; same immediate-reset error |
| c5bc303 | enemy-20260913-before6 | Death fixture omitted root PR_FRONT |

The immediate-reset error occurs at EnemyUpdateProc instruction 665, writing
hitBottomPrev through the old instance's cleared user. Pending old ball updates
then fail reading parent.user.take. Original native reset and bytecode contain
that same immediate-reset/post-reset access sequence. This artificial camera
teleport case is not sufficient evidence of a reconstruction-specific defect.
The delayed-reset fixture does not reproduce the user's gameplay issue yet.

No gameplay rule, original DAT, save, or VM execution-backend change is made.
The old unused translated dispatcher contains more broken placeholders, but
the active clean dispatcher implements these cases; do not claim those dead
code sites explain the observed game symptom.

## User test checkpoint

Provide an opt-in C++ output-sink snapshot of the first script failure, before
unwind clears the relevant VM state. It captures a synthetic labeled context
(E04B0001) without raising an exception or changing game error handling. This
is a diagnostic checkpoint, not a confirmed gameplay fix. Ordinary builds
remain quiet. No automatic screenshots or mass VM logs are enabled.

The original executable was briefly started under x32dbg for comparison;
module-entry breakpoints delayed startup and no gameplay finding was obtained.
The debugger stop command was issued after the user reserved live testing.

Checklist: static anchors and original bytecode reviewed; offline evidence
preserved; original-like boundary failure distinguished from proven defect;
gameplay fix and live validation remain pending. Next action after user test:
inspect the first script snapshot or native crash, restore the implicated
function from original assembly/source, then rerun offline regressions.

## Diagnostic candidate validation

Source a4d5b56, build/run prefix enemy-20260913-capture. Win32 Release built
and CTest passed 4/4. The diagnostics contract validates independent first-fault,
unhandled and synthetic script-error minidumps; repeated script errors continue
logging while only one script dump is created. Capturing works with trace off.
DAT copies were size/SHA256 checked by stage_dat.ps1. No live game was run.
The experimental enemy probe still needs its native PR_FRONT binding (not a
field of global.cv4), and is not counted among passing CTest coverage.
No claimed ball/death gameplay fix. User replay is needed for a real first-error
snapshot rather than speculative changes to original actor/reset behavior.

Quiet counterpart enemy-20260913-quiet also passed CTest 4/4 and DAT staging.
Both executables correspond to source a4d5b56 (subsequent commits are evidence
only). All artifacts retained. Live smoke explicitly deferred to the user.

## User reproduction and callback repair, 2026-09-13

The user reproduced both ball disappearance and upward stomp death in candidate
 a4d5b56. Retained snapshot: runtime-builds/enemy-20260913-capture/
fault-20260913-074843-528-p34516-script.dmp. The log shows FairyOtedama failing
at frame 990, EnemyUpdateProc instruction 665, followed by four EnemyUpdate_Ball
failures at instruction 14. A walking fairy fails the same way at frame 1130.
At the first error the VM is 081A9BE8, STK(0) is old Actor instance 0DCE0300
with user=null; native Actor 0823AC80 has already published new instance
0D9FC4C8 with valid user 0D9FE8F8. The old invocation and new actor are distinct.

The unconditional error retirement clears the newly initialized update in this
case. A later damage callback can still set vy=-5, but the retired update never
adds gravity again. This explains a shared mechanism for both symptoms; the
snapshot itself precedes the stomp and does not independently capture it.

Source 33bff37 moves invocation ownership/retirement into C++ with retained
SqPlus environment/function objects. Failure retires an update only if its VM,
environment and function still match the failed invocation. A newly installed
callback survives. Ordinary permanently failing callbacks still retire once.
This is a narrow reentrant callback-lifetime correction, not a literal copy of
the original unconditional catch at 45E180. Original gameplay parity remains
an explicit question to the user; do not misrepresent this as verified exact
original exception behavior. No original resource, reset initializer, velocity,
gravity, damage selection or intentional blow-off behavior is patched.

Offline diagnostic candidate enemy-reset-20260913-r1-diag passed CTest 4/4,
including a callback that installs its successor then throws (successor runs
8 times) and the existing broken/healthy callback isolation. The original DAT
enemy contract now supplies native PR_FRONT=65535 in its isolated fixture.
It synchronizes native/script cameras and uses original map flag 0x20000 for
activation after reset. Four generations preserve all four ball references,
parent identity, callbacks, visibility and motion. After reset, OnHitStep and
normal damage execute original death scripts: initial vy=-5, positive velocity
later, and y below the initial position after 90 updates. Known old-instance
errors during reset remain errors; they no longer cancel the replacement.
Outputs and original-build failures remain intact. No live game was launched.

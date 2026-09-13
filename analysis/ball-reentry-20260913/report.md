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

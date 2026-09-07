# Branch return, invalid motion and repeated stage loading

## Scope and evidence

Follow-up to ba5dfc7. User reports disappearing/dead player after returning
from a branch and colliding, followed by a crash when reentering. Some
straight-moving entities disappear after travelling. Neither game may be
launched; use original assembly and offline console contracts.

Reuse the read reverse-engineering/ida-reverse workflow, precedent/tool
references and E-imports in ../gameplay-contracts-20260907/report.md. The skill
scripts opened the original copy as 7a40a8d6. IDA MCP survey confirms PE32,
3963 functions and unchanged original SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Imports remain readable Windows file I/O, graphics/input/audio/timing and COM;
dynamic LoadLibrary/GetProcAddress is present. No new external scope.

Pre-build diagnostic EXE SHA256:
AB37E455B3CABB907A23B761BE2F4B10BB0191F8FF8185F02B0647DB121A1514.
Matching map: build-runs/p3-save-worldmap-diag/kinoko.map.
Latest trace is 2946527754 bytes, last modified 2026-09-07 14:06:19.

## E-latest

- w1-c01a -> w1-c01b -> w1-c01a, then player 08780D30 has y/freeHeight NaN
  at frame 3500, x=3158.75. Previous samples show exit at (3120,1024), landing
  at y=895, then a new jump. SetDead(false) is called by original Update.
- Later w0-s01a is reloaded on every global update. The final RVA BD8F4 is
  source-built SQTable::AllocNodes (preferred 004BD8D0), with allocation
  pointer EAX=0. This is consistent with exhaustion after repeated loads.
- Earlier current-run script errors include EnemyUpdate_Ball with missing
  take. Failure logging reaches its cap, so absence of later messages is
  not evidence of successful callbacks.

## Work items

- [x] Check skills, unchanged target/imports, current EXE and crash map.
- [ ] Reproduce and restore the invalid-motion source.
- [x] Restore VM error recovery and retire failed global updates; test reentry once.
- [x] Check fairy and white-kedama walking, collision and wait/reset paths offline.
- [x] Build/test both variants and stage DATs.
- [ ] Commit source, regression tests and final evidence.
- [ ] User gameplay verification.

## E-exception-unwind

A nested uncaught script throw left stackbase=6 and three CallInfo frames
instead of returning to zero. Repeated failures in the supplied trace reached
stack bases around 0x13B1. Restore SQVM::Execute exception_trap from original
4971xx..4975xx and Squirrel 2.2.2 sqvm.cpp:971-1020: preserve the error object,
resume the nearest trap or unwind to the native/root boundary, kill suspended
generators, pop varargs/call frames, and release abandoned stack slots.
PUSHTRAP now stores base/top/IP/target in the original order and maintains
the local trap count. The .call/.pcall native functions propagate SQ_ERROR
and retain their distinct raiseerror flags.

Original 45E180..45E1B6 clears a failed Actor update. Original
4699BF..4699FD clears the global SquirrelFunction at 513C98 and resumes at
46995C, separate from the outer PostQuitMessage catch. Restore both using
the same empty-function assignment as the existing actor reset code.
The callback adapter restores its temporary stack on failure.

The tests cover uncaught nested errors, 64 script catches, 32 .call catches,
varargs throws, failed Actor update retirement while a healthy actor keeps
updating, one-shot UpdateStageChange over 120 ticks, and a failed global
callback called only once over 64 actual function_469900 updates. Stage I/O
in the reentry contract is a counter sink; this does not claim full in-game
stage reentry has been verified.

## E-call-stack-relocation

Original 495959 reloads the stack after CallNative. The reconstructed CALL
retained a destination pointer into the old allocation. A deterministic
native callback that relocates the stack and returns 12345 reproduced the
lost return before the fix. Reacquire the target by its index afterward.
The metacall helper now uses absolute argument indices and copies callee
and arguments before pushes can relocate their storage. A nested _call
metamethod checks the nonzero-base case.

## E-argument-and-closure-storage

494176..494180 tests ndefaultparams != 0 AND nargs < nparameters. RetDec's
boolean equality instead skipped varargs insertion yet recorded nonzero
varargs, causing an invalid unwind read. Restore the original condition,
old-value reference release and varargs stack transfer. Do not clamp invalid
varargs counts or invent new default-argument rules.

4950C0 is SQVM::CLOSURE_OP. Both captured-value and default-argument vectors
called reconstructed realloc with counts as pointers. Creating a closure
with object defaults deterministically terminated the console contract.
Restore vector allocation, symbol/local/outer captures, default stack-value
copies and target assignment. The tests cover all three capture kinds,
default object identity and stable reference counts across 64 calls/errors.

GETVARGV used int32_t* arithmetic with byte offsets 44/46 and an 8-byte
SQObject stride, multiplying both by four again. Restore the byte-addressed
lookup matching original 491500 and source GETVARGV_OP.

Independent source-build assembly anchors:
../evidence/raw/phase3-20260906/squirrel-sqvm.asm contains StartCall at line
13803, CLOSURE_OP at 2940, GETVARGV_OP at 11074. The main agent also checked
the corresponding supplied Squirrel source and original IDA assembly.

## E-error-handler-lifetime

492D00 now passes complete SQObject pairs to CallErrorHandler. Exercising it
then exposed a separate use-after-free in sq_seterrorhandler, 48A300:
assignment incremented and immediately decremented the NEW handler rather
than releasing the OLD one. Original 48A32D/48A331 saves the old value first.
Restore normal SQObject assignment. An anonymous handler now survives
registration, receives the exact thrown table, and is not invoked for
.pcall. Diagnostic 48ACE0 logging also no longer reads a function prototype
from native closures.

## E-enemy-motion-and-boundary

The console fixture loads the effective original enemy scripts, enemy PAT
and GetCallbackFuncTable from global.cv4. Two Init0106 fairies and one
Init0101 white kedama run 16560 actor frames through native manager updates,
terrain landing and real mutual collision callbacks, plus 48 offscreen
wait/reset/reactivation cycles. Coordinates and callbacks remain valid.
No movement, damage, release, reset or DAT script was modified to make this
test pass. The camera/terrain test fixture is deliberately wider for the
long walking segment and then sets explicit offscreen inputs for reset.

An abrupt synthetic camera teleport that skips EnemyUpdate_WaitForReset
also exposed a direct Reset continuation: EnemyUpdateProc instruction 665
writes old this.user after Reset has cleared it. Original 45FB90/45EB00 and
the packaged bytecode agree on that path. This is not promoted to a
reconstruction-specific bug without original dynamic comparison; no
skip-after-reset or preserved-old-user workaround was added. The passing
reset contract follows the ordinary wait-then-reset path. This boundary
remains relevant for future camera/branch reproduction.

The real w1-c01a -> w1-c01b -> w1-c01a terrain contract runs 1440 native
contact frames with player PAT takes 335/615 and remains finite. It has not
reproduced the first NaN seen in the user's game trace. Neither these tests
nor the VM fixes establish that all disappearing-entity cases are resolved.

## E-observation

Added bounded, read-only diagnostics: actor:invalid-state records the first
invalid coordinates/velocity/free-space/bounds at script, collision and
motion boundaries. actor:invalid-float-* records a script-side float store
with field offset and recent script instruction indices. actor:script-hide-*
records the original -16777215 offscreen sentinel with frame and camera.
These observations never clamp, replace, respawn or suppress actor state.
They pass the existing actor: filter, and no-log mode still silences only
the output. Existing trace/capture policy was not changed.

## Final offline verification

Both Release variants build and pass CTest 2/2 (archive_smoke and
stage_native_contract). Both also pass the extended contract for w1-c01a,
w3-s01a and w7-s01a. The existing Ten stone/block, eight-head expiry,
hidden-layer and point/save/1up tests continue to pass.

Reproduce the extended console test from the repository root:

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603 analysis/player_ground.cv4 analysis/gameplay-contracts-20260907/player.cv4 analysis/constant.cv4 C:/WorkSpace/6kinoko data/map/w1-c01a.act
```

stage_dat.ps1 copied all three DATs beside each EXE and verified size/SHA256.
No index.dat, save or configuration file was replaced. Neither game was
launched, including startup smoke; user no-launch instruction takes
precedence. All visual and live gameplay outcomes are pending user tests.

Final EXE SHA256:
- Diagnostic: 0E5C846E592C749EA3403A27467193A5A93AE77B0909E52CA55F8E9409154500
- No-log: BABE42D5F90ADD5BBD9CA076014C7774E3FCB2C80510A1D5CE01402F1106F8A3

IDA worker expiry was handled only through the skill start/open scripts.
Final session 945ec1b2 saved original-function annotations to
C:/rs-ida/e03eacb3-6kinoko.exe.i64. Scope/imports are carried forward above.
ADF P0: confirmed VM findings have original static anchors and reproducing
console tests; the NaN producer and complete gameplay fidelity remain
unconfirmed. This is ordinary game reconstruction, not an IOC analysis.

## User retest order

1. Diagnostic EXE: repeat branch exit, landing, jumping and side collisions,
   then die/reenter the same stage several times.
2. Follow an ordinary fairy and a white kedama while they remain on screen;
   then move away/return and let enemies meet. Note whether disappearance
   happens visibly on screen, only after leaving it, or on collision.
3. Retest Ten stone/block, eight-head expiry, hidden passage and item flight.
4. After diagnostic cases pass, repeat the first two with the no-log EXE.
5. On a failure, preserve retdec_trace.log/dump before another diagnostic
   run and report stage/branch, character/form, action and approximate time.

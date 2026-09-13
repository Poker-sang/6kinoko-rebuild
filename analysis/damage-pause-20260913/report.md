# Damage/death update groups

Scope: restore original injury/death pause, not invent a whole-game pause. Original identity/imports carry forward from ../map-visibility-20260913/original-survey.json and original-imports.json. IDA MCP session 28e5a592 has the same original SHA256. Original source-script bytecode player.cv4 SetDamage sets changingCount=30 and ::updateMask=GP_ACT|GP_PLAYER. SetDead uses the same mask. Player Update restores the mask through its original transition logic.

Original 469982 loads ActorManager at 5143E0 into ECX; 469987 writes the frame mask to 514420 (the manager field +64), then calls 4641D0. Original 4641D0 checks Actor+232 & manager+64 for script and motion updates. The reconstructed ActorManager is a separate storage block, but 469900 writes only detached global g622. Its actual +64 retains the constructor mask -1, so enemies continue updating during the scripted freeze.

Regression --damage-pause runs the real stage dispatcher with two real Actor instances and script callbacks. It checks 30 frames of player-only stepping/motion, enemy preservation, both-group resume, and mask-zero exclusion. Commit before running baseline.

## Repair and result

Baseline 7a12062's initial probe was intercepted by the old optional-file argument path; 11613f8 moves dispatch ahead of that branch. Baseline before2 then fails at the actual stage mask check: manager+64 is not g459 after 30 dispatches. These products/logs are retained.

Source checkpoint 4123bf9 replaces the detached g622 integer with an alias of the reconstructed global ActorManager+64. This restores the original shared storage and frame-snapshot timing; it adds no pause flag, hard-coded duration, or player/death-specific branch. Script writes during an actor callback affect the next stage-dispatch snapshot, as in the original. Camera/map/ACT dispatch conditions remain the original ones.

Both damage-pause-20260913-r1-diag and r1-quiet compile and pass CTest 6/6. The new regression runs 469900 with actual Actor objects and callbacks: mask 0x40000004 allows player callbacks/motion for 30 frames while enemy callbacks/motion remain unchanged; restoring the Actor bits resumes both groups; zero mask excludes Actor updates. Original constant.cv4 confirms GP_PLAYER=4 and GP_ENEMY=8. Existing texture ownership/5000-cycle, stage, ABI and diagnostics tests pass. Both candidates also pass --enemy-reentry using the original staged DAT scripts (four ball generations plus death motion).

Both EXEs have all three DATs copied and SHA256-verified beside them. The latest prior quiet-build marisaA.dat is copied to both new run directories without modifying the source save. Runtime manifests record executable hashes and source revision. Old runtime/build outputs are untouched.

Live gameplay is left to the user under the prior session preference; no new game was launched. Automated results establish mask propagation and Actor group freeze/resume, not an observed full player injury/death animation replay. The original packaged scripts remain responsible for the exact transition duration and restoration. Squirrel source/compiled auxiliary evidence carries forward from the preceding map-visibility report; no VM change is needed here.

Checklist: original script and IDA disassembly agree on update-mask semantics; exact storage mismatch identified; baseline failure retained; minimal storage repair; both variants and six regressions pass; original enemy scripts still pass; DATs staged; old products retained; source committed before every test batch.

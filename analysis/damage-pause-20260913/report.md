# Damage/death update groups

Scope: restore original injury/death pause, not invent a whole-game pause. Original identity/imports carry forward from ../map-visibility-20260913/original-survey.json and original-imports.json. IDA MCP session 28e5a592 has the same original SHA256. Original source-script bytecode player.cv4 SetDamage sets changingCount=30 and ::updateMask=GP_ACT|GP_PLAYER. SetDead uses the same mask. Player Update restores the mask through its original transition logic.

Original 469982 loads ActorManager at 5143E0 into ECX; 469987 writes the frame mask to 514420 (the manager field +64), then calls 4641D0. Original 4641D0 checks Actor+232 & manager+64 for script and motion updates. The reconstructed ActorManager is a separate storage block, but 469900 writes only detached global g622. Its actual +64 retains the constructor mask -1, so enemies continue updating during the scripted freeze.

Regression --damage-pause runs the real stage dispatcher with two real Actor instances and script callbacks. It checks 30 frames of player-only stepping/motion, enemy preservation, both-group resume, and mask-zero exclusion. Commit before running baseline.

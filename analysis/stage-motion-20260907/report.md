# Stage motion and start transition

Scope: follow-up to 76d6d14. Shop player animates falling but cannot move;
ordinary stages remain on the start display. The user reports other entities
now look normal. No game launch is permitted; tests use standalone console
programs. Carry forward E-imports/tool/skill evidence from the prior reports.

- [x] Read latest logs and compare original Actor::Update and terrain helpers.
- [x] Restore original collision-enabled movement and test it offline.
- [x] Locate and repair the start-display visibility failure.
- [x] Verify diagnostic/no-log builds, stage DATs; included in this backup commit.
- [ ] User runtime confirmation.

Original IDA session 73953b36. Native 45EC60 calls 469750 -> 4689D0 when
collisionMask is nonzero and the selected animation enables collision.
The reconstruction implemented only the mask-zero movement branch, explaining
why a player with nonzero velocity never changed position.

4689D0 enumerates nearby MCD/actor collision records, invokes 466E20, 4674B0,
4677C0 and 467CD0 in order, and writes movement/contact/slope state. Restore
these from original semantics, including one-way flags and slopes; no stage
coordinates or replacement gameplay rules.

Original stage.cv4::UpdateStageStart post-decrements stageChangeCount, fades
out at 30, and at zero hides StageStart.pl.visible, restores updateMaskPause,
and installs UpdateGlobal. Test the actual bytecode and native bridge before
changing this path.

## Movement correction

src/reconstructed/actor_collision.cpp ports the original 4689D0 calculation
and its 466E20/4674B0/4677C0/467CD0 helpers into C++. It preserves the ordered
vertical-interval, side, ceiling/corner and floor passes, signed MCD dimensions,
shape codes, contact bits, slope results, soft-side and one-way flags.
The C runtime gathers the same 12-byte collision records and invokes this
module from the original 45EC60 movement decision.

435220/436290/4362F0's query order is retained: start at the actor's cached
record index, scan forward, then backward where required. Query bounds use
the original 24-pixel expansion and map-layer offsets; selected records get
their original world-coordinate fields updated. Dynamic actor records follow
the map records, filtered by collision masks and horizontal proximity.
The selected support record reattaches the original map proxy through the
existing parent binding. Parent references are weak; detaching now releases
the weak count rather than the strong count.

45EC60 now takes its collision path only when both collisionMask and the
animation collision flag are enabled. Other animations use the original
unconstrained movement branch even when the actor has a collision mask.
The original slope adjustment, parent motion and bounds refresh are retained.

## Start display correction

The actual stage.cv4 countdown passed before any countdown/VM change. It
decrements from 120, performs its fade calls, hides StageStart.pl.visible,
restores updateMaskPause and replaces the global callback after 121 calls.
The separate failure was in 4525D0: the reconstruction drew the ACT even
when CAct+96 (the native visible field) was zero. Original 4528C9..4528CC
gates both the layer pass and queued BitBlt sprites on that byte.
Restore that gate without changing stage scripts, durations or state flags.

The test also runs the actual countdown with a native ActingPlayer instance,
not only a fixture table, and confirms its visible setter writes CAct+96.
A fake draw callback verifies hidden ACTs submit no layer draws and visible
ones still draw. No Direct3D device or game window is created.

## Verification and handoff

Both Release builds (p3-save-worldmap-diag and p3-save-worldmap-notrace) pass
archive_smoke and stage_native_contract, 2/2 each. Both contract executables
also pass with the effective original block.cv4 and stage.cv4:

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4
runtime-builds/p3-save-worldmap-notrace/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4
```

New coverage includes collision-enabled horizontal motion, falling/landing,
jumping off support, walls, ceiling, passing upward through one-way platforms,
both ground-slope directions, animation collision bypass, bidirectional cached
map queries, actual stage countdown/native visibility and draw suppression.
Existing registration, actor creation/recycling, pair callback and retired-map
lifetime tests remain passing.

IDA was restarted with the skill start.ps1/open.ps1 after interruption.
Session 248bcd08 surveyed the same original hash
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Squirrel 2.2.2 sqvm.cpp CallNative/Set and prior source-object disassembly
were used to check native callback arguments and root fallback. No VM opcode
or SetGlobalUpdateFunction behavior was changed for the start screen.

stage_dat.ps1 copied and hash-checked the three DATs beside each EXE.
No game executable was launched during this work, including across resumes.
Save/config files and unrelated user changes were preserved.

Final game EXE SHA256:
- Diagnostic: EA0D5C541D7A2329A0B7349940655D22FF66DD1ACA95858DF5CC26CBE7D73874
- No-log: F6CFF4866B325EE2734BD797019FE915B8E5801F07CAEF80B4FC831E6E4B55E8

User test:
1. Run runtime-builds/p3-save-worldmap-diag/kinoko_retdec_rebuild.exe.
2. Enter the shop, check landing/standing, walk both ways, jump and land.
3. Enter an ordinary stage, let the start display disappear, then test movement
   and terrain contacts; return to the map and reenter.
4. Repeat with p3-save-worldmap-notrace.

The native branches and isolated behavior are verified, while complete
gameplay, special terrain combinations and visual fidelity await user testing.
Existing logs also contain a separate ACT-inline GET failure and an exit-time
stack overflow; neither is claimed resolved by this motion/visibility change.

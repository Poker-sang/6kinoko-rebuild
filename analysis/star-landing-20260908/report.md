# Stars disappearing on landing

Scope: follow-up to 16857a5. User confirms enemy disappearance and occasional
map-transition crashes remain, but explicitly asks to focus only on stars
knocked out of blocks disappearing when they touch the ground. Do not launch
either game. Preserve original script behavior and loading decisions.

Carry forward read reverse-engineering/ida-reverse skills, required references
and E-imports from ../passage-ledge-20260907/report.md. Skill scripts reopened
the original as f903f0fb. Survey verifies the unchanged original PE32 SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.

- [x] Inspect actual item bytecode and existing native contracts.
- [ ] Reproduce the relevant star path and restore a confirmed discrepancy.
- [x] Build/test focused diagnostic and no-log variants; stage DATs and back up.
- [ ] User verifies gameplay.

Initial isolated InitStarC/InitStarD tests run the actual item UpdateWalk,
PAT animation 1060, native terrain collision and movement. On a synthetic
floor both stars land and bounce for 300 frames without Release or NaN.
This is negative evidence against an unconditional ground-contact release;
it does not yet cover original block initialization, actual stage geometry,
sprite placement or texture rendering.

## Expanded Contracts

Six 300-frame cases now cover InitStarC/InitStarD on the synthetic floor and
actual w1-c01a terrain, followed by two stars emitted by original Init0436
and its bound SetDamage/CreateItem/CreateItemCommon path. The bumper fixture
stands on either side of the block, then moves away to exclude accidental
pickup. SetDamage is invoked after eligibility would be established by the
block collision callback; this does not test the complete player's head bump.

The actual item PAT is parsed with texture slots that have dummy 1024x1024
dimensions and nonzero handles. This restores real PAT quad geometry while
leaving the D3D device null. Native actor-manager update, collision, rendering
transforms, and original item scripts run normally; device submission returns
before device access. Tests check finite positions, first ground contact,
negative rebound velocity, continued existence, and quads extending above
the surface. The star lands at actor y=900 on the actual y=896 floor, with
collision interval 871..896 and visible-quad interval 868..900. This follows
its packaged collision box/pivot; no coordinate adjustment was introduced.
The block-emitted stars remain unrewarded for 300 frames; native collision
with the player afterward calls AddStar once and marks each star for release.

All six cases pass before any gameplay change. The production source was
therefore changed only to observe the reported symptom, not to manufacture
a new star lifetime, bounce, pickup or visibility rule. No symptom fix is
claimed at this checkpoint.

## Focused Runtime Recording

actor:star lines record moving yellow stars (take 1060, UpdateWalk), including
position/velocity, contacts/bounds, visibility/activity/release flags,
priority/alpha, texture handle, transformed quad Y, map height and camera.
Motion and render-entry/draw phases distinguish simulation from drawing.
Sampling is bounded per handle and retains contact changes. Release logs
also inspect the active script call frames using checked prototype metadata.
No VM reference count, actor field, callback or physics value is modified by
the recorder. Existing trace call sites remain present in no-log mode.

RETDEC_TRACE_STAR_FILTER acts only inside retdec_entry.c's output function.
It keeps star lines, invalid-actor state, update/script failures, map paths and
native exceptions. Star records flush immediately. The dedicated build also
disables screenshots. The ordinary diagnostic build configuration is unchanged.

Build the focused diagnostic from the repository root:

```powershell
cmake -S . -B build-runs/star-landing-diag -G "Visual Studio 18 2026" -A Win32 -DKINOKO_REFERENCE_DIR=C:/WorkSpace/6kinoko -DKINOKO_RUNTIME_DIR=C:/WorkSpace/6kinoko-rebuild/runtime-builds/star-landing-diag -DKINOKO_RETDEC_TRACE_FILTER=ON -DKINOKO_RETDEC_MAP_FILE=C:/WorkSpace/6kinoko-rebuild/build-runs/star-landing-diag/kinoko.map "-DCMAKE_C_FLAGS=/DRETDEC_TRACE_STAR_FILTER /DRETDEC_DISABLE_RENDER_CAPTURE"
cmake --build build-runs/star-landing-diag --config Release --parallel 4
```

## Verification

Both star-landing-diag and p3-save-worldmap-notrace pass CTest 2/2 and the
extended original-asset contract. The extended command is unchanged from
../passage-ledge-20260907/report.md, replacing the run directory as appropriate.
New output is in build-runs/star-landing-diag/contract.log and
build-runs/p3-save-worldmap-notrace/star-contract.log. Both EXEs have all three
DATs copied beside them with size/SHA256 validation. Neither game was launched;
save/config files were not copied. No currently running rebuilt process or
current p3-save-worldmap-diag trace was present during read-only inspection.

Original IDA 45EC60 confirms parent motion, floor-slope correction, collision
dispatch and bound reconstruction. The original item CV4 instruction format
uses the supplied Squirrel 2.2.2 sqopcodes.h/sqfuncproto.h. Carry forward the
source-build VM assembly reference in ../evidence/raw/phase3-20260906/squirrel-sqvm.asm.
The first worker expired; skill open.ps1 reopened the same original as
090b9137. Original 45DBC0 confirms Release only marks the Actor's release byte
and manager's dirty byte. Notes were saved to C:/rs-ida/ffeb02fe-6kinoko.exe.i64.

Final EXE SHA256:
- Focused diagnostic: 845D8FEAAD0DF3AF6479645631AC79D8B4EDBD6C5979DB475E405E34BCEEFF79
- No-log: 7A37490559FDBBB858442D6A0AF869F8E77EBE225CBE63CED31DE90F27686C2D

Next evidence must come from the actual failing scene. Run
runtime-builds/star-landing-diag/kinoko_retdec_rebuild.exe, reproduce one
block-star landing disappearance, then close it normally. Preserve
retdec_trace.log beside that EXE. Note block location and whether the star
counter changes; do not retest enemy/map issues for this task.

P0 synthesis: no confirmed new gameplay defect yet. Negative offline results
do not override the user's observation. The no-game-launch constraint leaves
the runtime reproduction to the user; continue analysis from that focused log.

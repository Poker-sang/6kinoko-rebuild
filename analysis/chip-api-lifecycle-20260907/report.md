# Chip flag APIs and Actor callback initialization

Follow-up to 7bb2ab1. Stage entry improved, but some locations still make the
player disappear/die; immediate reentry can reproduce it, while waiting seems
to help. User further identifies two-head mode as affected, without claiming
it is the only affected form. Do not launch either game or add grace periods,
spawn changes, delays or altered death conditions.

## Scope and evidence

Read project instructions and reverse-engineering/ida-reverse skills, using the
same workflow/tool references and carrying forward E-imports and scope from
analysis/stage-entities-20260907/report.md. Skill start.ps1 reused the server;
the previous worker 078728c8 was unreachable. Skill open.ps1 created original
session 555a6bb6. Survey verifies PE32, 3963 functions and SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Readable imports and their limits are unchanged from the prior E-imports.

- [x] Inspect new player-state logs and original native methods.
- [x] Restore confirmed flag API and constructor divergences.
- [x] Add negative/positive regressions and two-head original-asset coverage.
- [x] Build/test both configurations and stage DATs.
- [x] Include source, tests and this report in a backup commit.
- [ ] User verifies the intermittent disappearance/death and immediate reentry.

## E-crashes: concrete native faults

The existing diagnostic log is approximately 1 GB. Targeted reads, not a full
log dump, found:

- Lines 12913582 and 13557707: access violation, rebuilt RVA 0x4EFCE,
  mapped by build-runs/p3-save-worldmap-diag/kinoko.map to function_45f760+14.
- Line 25160872: access violation, RVA 0x56A55, mapped to function_4681e0+37,
  called via function_45f810+12 (RVA 0x4F1BC).

These identify broken native methods, not proof of every script-driven death.
The old player log contained no two-head takes 100..199 or death take x55.
One long run reached the shared 6000-record limit. Several before-script x
strings were malformed while adjacent bounds and subsequent x values remained
consistent; their cause is not established, so do not treat them as proof of
an actual teleport. New xyBits fields allow checking the raw coordinate bits.

## E-native: restored original methods

45F760 SetChipFlag: restore ECX receiver and sign-extended int64 write at
Actor+392, with original thiscall cleanup and 64-bit return. A protected
console ABI test raised an access violation before the change and passes
afterward, including positive and negative flag values.

45F780 SetChipBoundType: restore ECX receiver and the 16-bit write at +410.
45F810 -> 4697C0 -> 4681E0 GetChipFlag: pass both the Actor and original global
CollisionManager explicitly instead of RetDec uninitialized receivers/FPU
temporaries.

Factored the existing 435220-style rectangle query and collision-vector append
to accept the state/rectangle explicitly. Motion still uses its original
24-pixel expansion. GetChipFlag uses the unexpanded rectangle, truncates floats
to integers, scans enabled registered map layers with their cached indices and
ORs Chip+16. It neither filters by Actor.collisionMask nor writes collisionFlag.
4361C0/436290/4362F0 confirm that this is the original max-dimension broad query;
no invented exact-shape filter was added to the flag query.

Regressions cover adjacent-but-outside records, multiple selected flags,
forward/backward cached scans, disabled layers, empty results, fractional
layer offset, unchanged collisionFlag, clear and immediate re-registration.

## E-constructor: corrected object offset

Original 45E390 initializes the update callback SquirrelObject at Actor+108.
The rebuild initialized +112, leaving the actual vtable word unset and writing
a vtable address into the actual type field. The constructor regression failed
before the correction and now verifies vtable/null-type/null-value placement.

Original 46AB10 calls 45E300 again when reusing a pool slot (46AC58), and the
rebuilt pool already does the same. Thus the offset correction applies to new
and reused Actors. The existing constructor already clears the chip record;
no additional timer-based reset or guessed reentry cleanup was introduced.

## Diagnostics and verification

Player take/actor transitions remain logged after the normal trace budget.
Rare original player.nut SetDamage/SetDead calls log their caller and first
argument. These observers only read VM/object state; no VM stack manipulation,
death suppression or gameplay flag changes. No-log builds still silence only
the output sink. This is intended to capture the remaining manual symptom if
the confirmed native fixes are insufficient.

Both Release builds pass archive_smoke and stage_native_contract (2/2 each).
Both variants also pass the extended original-script/PAT/ACT/MCD checks for
w1-c01a, w3-s01a and w7-s01a in TYPE_2HEAD. The walking fixture now calls the
published GetChipFlag method every frame and obtains its entry test position
from the selected ACT's marker, instead of a fixed w1-c01a fixture coordinate.

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603 analysis/player_ground.cv4 analysis/player.cv4 analysis/constant.cv4 C:/WorkSpace/6kinoko data/map/w3-s01a.act
```

Use w1-c01a.act/w7-s01a.act for the other maps and p3-save-worldmap-notrace
for the other build. Squirrel source/opcode definitions and the original
SetDamage/SetType/SetDead bytecode were inspected; no script or VM opcode was
edited. Fixtures retain the limitations documented in the preceding report:
not a complete scene, no graphics, and no other actors' full behavior.

stage_dat.ps1 copied and size/SHA256-verified all three DATs beside each EXE.
No save/config or original resource file was edited, and neither game was run.
IDA annotations were saved in the temporary original-copy database.

Final game EXE SHA256:
- Diagnostic: 9277D8D9C0257CB554690B0481A5CC33A30D927C088C1B7C681FDDF47B0420EB
- No-log: 82BE89D9022C6C53DAE28080657C6EEAE9AA93FF902FD36E14A37E9BC23CAC3C

Native defects are confirmed by assembly plus failed/passed offline checks
and the captured access violations. Whether they explain every intermittent
death, or the apparent benefit from waiting before reentry, remains unverified.
The known exit-time stack overflow and other unvisited legacy methods are not
claimed fixed.

Manual test: restart the diagnostic EXE, remain in two-head mode, walk and jump
over the previously failing positions. After a failure, reenter immediately;
compare with delayed reentry and report the stage, position and form. Preserve
retdec_trace.log, particularly actor:player-event and actor:player-state lines.

# Walking death follow-up and camera callback restoration

Scope: user confirms stage entry works after 1e79301, but horizontal movement
causes death. Neither game executable may be launched. Preserve original
loading, spawn, movement and death rules. No gameplay workaround was added.

## Evidence and work status

- [x] Read project instructions, reverse-engineering/ida-reverse skills and
  required workflow, tool, precedent and decision-quality references.
- [x] Compare native movement and original player bytecode; extend offline tests.
- [x] Restore the confirmed Camera::Update callback omission.
- [x] Build/test both variants and stage the required DATs beside each EXE.
- [x] Include source, tests and this report in a backup commit.
- [ ] User verifies whether the reported movement death is resolved.

Carry forward E-imports and scope from analysis/stage-entities-20260907/report.md.
Skill start.ps1 reused IDA; database 078728c8 remains reachable. Survey verifies
PE32, 3963 functions and original SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Imports remain the previously recorded file IO, D3D/D3DX, WinMM, USER32, GDI,
IMM32 and COM view; no new binary or analysis target was introduced.

Latest existing diagnostic log ends with w1-c01a and a later game:exit followed
by the known exit-time stack overflow. It does not record the death predicate.
The latest stage path was used for the offline terrain fixture, not assumed to
prove the user's exact cause. Asked whether shops are affected and whether a
tap or several steps trigger death; no reply was received before this report.

## E-walking: negative evidence and test limits

Read original 45EC60, 4689D0, 466E20, 4674B0, 4677C0 and 467CD0 in IDA. This
comparison did not establish a new horizontal collision defect. Do not claim
that all terrain combinations or original floating-point edge cases are proved.

The console test executes the original player.cv4::Update and
player_ground.cv4::Stand/Walk/Brake bytecode against a real reconstructed Actor,
with constants from the effective constant.cv4 and animations from marisa.pat.
It verifies native and script-visible bounds, movement, turning, stopping and
VM callback failures. On a flat floor the three 30-frame phases produce:

- Right: x 100 -> 161.25, vx 2.5, take 110.
- Reverse: x 161.25 -> 148.75, vx -2.5, take 110.
- Stop: x 148.75 -> 137.5, vx 0, take 100.

Test-only scene fixtures declare an Actor.funcUpdate slot, supply input/camera
values and player state, skip ladder/vector-map behavior, and throw if the
original Update invokes SetDead. These fixtures are not production changes.
An initial incomplete fixture could silently pass after its initializer failed;
the test now checks callback installation, actual movement and every VM failure.
This isolated fixture does not certify complete original actor/script setup.

The extended check mounts all three archives read-only through the existing
410500/407370 reader, then loads the actual w1-c01a ACT and marisala2 MCD with
g678==0 (no Direct3D device). The ACT loader parses scripts but does not run
them. Its terrain contains 906 records. The test registers the same te/wa
collision layer categories as stage.cv4::LoadStage, starts a fixture player
near the entry marker, and runs 60 frames in each direction without a death
callback or invalid width. Other actors, stage-script initialization, drawing
and the real following camera are outside this fixture.

Squirrel 2.2.2 source/opcode definitions underpin tools/inspect_cv4.py and the
native callback ABI comparison. No VM opcode semantics were changed.

## E-camera: confirmed defect and repair

Original 466470 takes ECX=Camera, checks the SquirrelObject at Camera+28, and
calls 45DFB0 with ECX=Camera+12 when it is a closure. Original 46996B explicitly
loads the global Camera into ECX before invoking it, under the 0x20000000 mask.

The rebuilt 466470 instead called function_4a9a30(), whose intentionally unusable
legacy wrapper always returns 0. Therefore it could never invoke the installed
camera callback. Restore explicit receivers, preserve the native thiscall bridge
and the existing update-mask/order, and reuse the recovered one-argument
SquirrelFunction callback helper.

Regression: a registered camera callback incremented zero times before the fix
(camera update callback skipped assertion), and once afterward. The test also
checks balanced VM stack state and that clearing the callback does not invoke it.
Confidence is high for this native omission. Its causal connection to the
reported movement death is still a candidate, not a confirmed symptom fix.

## Observation-only diagnostics

actor:player-state lines identify the real player.nut::Update callback, recording
before-script, after-script and after-motion phases. They report take, position,
velocity, free dimensions, contacts, collision flags, player and camera bounds.
Idle frames are sampled once per second; output is bounded to 6000 records.
The observer reads memory without calling Squirrel APIs or changing its stack.
It does not write player state or change any death/movement decision. As with
existing traces, the no-log build silences only the output sink.

## Verification and manual test

Both Release builds pass archive_smoke and stage_native_contract (2/2 each).
Both also pass the extended original-asset console command:

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603 analysis/player_ground.cv4 analysis/player.cv4 analysis/constant.cv4 C:/WorkSpace/6kinoko
```

Repeat with p3-save-worldmap-notrace for the second variant. Asset fixtures were
extracted with the existing kinoko_asset_probe console tool. Both game EXEs have
the three size/SHA256-checked DATs beside them; saves/configs were not changed.
Neither original nor rebuilt game was started; startup/gameplay smoke tests are
explicitly deferred to the user.

Game EXE SHA256:
- Diagnostic: 6228B390CAAC0A4D18374A79FB76D06E8A5DC28B2F94EADA2D29357FF7B6ABDE
- No-log: BC66ED746664B6F9F1CCE08CEBE184DB40F5506F885682EA3D9291924398CE6B

User test: run the new diagnostic EXE, enter the same stage, wait briefly, tap
right and left separately, then hold a direction and test a jump. Check both
survival and camera following. If death still occurs, preserve retdec_trace.log
and report stage/shop, player form, direction and approximate delay. The new
actor:player-state observations should discriminate incorrect bounds/contacts
from a camera-boundary failure; do not add a workaround based on the symptom.

Decision delta: horizontal walking passes isolated actual-asset checks; a
separate confirmed camera callback omission is restored. Return to manual
testing before claiming the user's movement-death symptom is resolved.

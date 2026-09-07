# Actor Reset and Ten keystone placement crashes

Follow-up to 183cc08. User reports that most gameplay issues improved, then
asks to fix a new in-stage crash and the Ten form's keystone placement crash.
Neither game may be launched. Restore original behavior, not ability-specific
workarounds, changed death checks or modified DAT scripts.

## Scope and status

Read project instructions, reverse-engineering/ida-reverse skills and tool/
precedent references. Carry forward the unchanged workflow and E-imports from
analysis/stage-entities-20260907/report.md. Skill start.ps1/open.ps1 opened an
original copy as 2b712b65. That worker later became unreachable; the same skill
scripts reopened 7ca49e69 for final annotations/save. Surveys confirm PE32,
3963 functions and original SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
No new binary target, network scope, or game execution was introduced.

- [x] Map both captured crash addresses to native methods.
- [x] Check original assembly/decompilation and the keystone bytecode.
- [x] Restore Move, Reset and shared instance cleanup.
- [x] Verify native contracts and real InitStone/PAT behavior offline.
- [x] Build/test both configurations and stage hash-checked DATs.
- [x] Include source, tests and this report in a backup commit.
- [ ] User gameplay confirmation of both crash scenarios.

## E-crash-map

The diagnostic log has approximately 1.34 GB. No retdec_crash.dmp exists at
the checked diagnostic runtime path. Targeted log reads and the matching map
file, before rebuilding, identify two distinct faults:

- Line 27613539, after w3-s01a: access violation at RVA 0x4CAB0, inside
  function_45dbd0 (Move), whose image address was 0x0044C8F0. Immediately before
  the fault the native call receives floats 0xC2200000 (-40) and 0. The bogus
  address 0xC22000F0 derives from displacement bits used as an object address.
- Line 32225153, after w7-c06a: access violation at RVA 0x4E8A1, inside
  function_45eb00+49 (Reset), image address 0x0044E870. Its caller is the native
  zero-argument Actor method adapter. The faulting read is at address 9.

Both methods retained RetDec uninitialized receivers. Move also retained
invalid simulated-FPU indices and pointer-derived conditionals. A protected
Move(-40,0) console regression failed before the repair and passes afterward.
The old Reset test terminated unsuccessfully; the restored test completes.

## E-move / E-stone

Original 45DBD0 loops while either displacement remains. Each iteration copies
previous position and bounds. With collisionMask, animation and its collision
flag present, each axis consumes at most +/-8 pixels before calling 469750.
Without collision it applies the remaining displacement in one iteration and
clears contacts. It then rebuilds scaled/mirrored world bounds, chip position
and int16 dimensions. It does not add velocity or parent displacement, and
Move(0,0) leaves state unchanged.

Restored thiscall ECX/ret-8 and that flow using the existing recovered collision
engine. Bounds refresh is shared with Actor::Update, retaining that existing
calculation rather than adding a separate position rule. No gameplay constants
were introduced beyond the original Move step bound.

The effective bullet.cv4 (archive 2, offset 3256657, size 15670) defines InitStone.
It selects take 1130, sets the original groups/masks, creates user state, calls
SetChipFlag(32), sets flag=0x800000 and invokes Move(40*player.direction,0).
That matches the captured -40 call. The original script then plays sound 33,
creates effect 1960 and assigns player.user.stone through a weak reference.

The test reads bullet.cv4 and item.pat through the original archive reader and
shared PAT parser, without loading textures or opening a window. It executes
the actual InitStone eight times across both directions, checking position,
unchanged velocity, take, flags, groups, initialized state, media calls and
weak-reference expiry after release. Media functions and the player holder are
test doubles only; production DATs and ability logic are untouched.

## E-reset

Original 45EB00 releases the parent weak reference and owner strong reference,
calls 45FB90 to clear the old script instance/callbacks, copies the initializer
and argument saved at Actor+56/+68, then calls Init with the saved coordinates
at +80/+84/+88. ResetPriority uses the priority resulting from that initializer.
The method is not a full placement-new Actor construction.

Restored explicit receivers and reference ownership. Saved SquirrelObjects are
retained across Init because it overwrites those same Actor fields. ClearInstance
is shared with existing in-place Actor destruction, preserving each caller's
cleanup order and the original separate update/collision callback state clears.
The Actor handle and chip metadata remain intact as in the original.

The 32-reset regression verifies initializer replay and argument identity,
reference-count stability, saved position/direction, priority, update/collision
callback replacement, old user/step cleanup, parent weak-count restoration,
expiry of old owner weak references, stable Actor count/handle and VM stack.

Squirrel 2.2.2 source/closure format and existing native callback helpers remain
the reference for the object/callback ABI. No VM opcode semantics were changed.

## Verification

Both Release configurations pass archive_smoke and stage_native_contract (2/2
each). Extended tests pass for w1-c01a, w3-s01a and w7-s01a in both variants,
including the real player, stage and block bytecode, PAT loading, movement,
new Reset coverage and new InitStone coverage.

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603 analysis/player_ground.cv4 analysis/player.cv4 analysis/constant.cv4 C:/WorkSpace/6kinoko data/map/w3-s01a.act
```

Repeat with the other map names and p3-save-worldmap-notrace. Default contracts
also cover blocked Move, diagonal 8-pixel steps and previous-position updates,
zero-displacement no-op, collision-disabled animation bypass, and preservation
of velocity/parent-delta fields.

stage_dat.ps1 copied and verified the three DATs beside each game EXE. Saves,
configs and unrelated worktree changes were preserved. Neither game was run;
startup/gameplay smoke tests remain assigned to the user. IDA annotations for
45DBD0, 45EB00 and 45FB90 were saved in the temporary original-copy database.

Game EXE SHA256:
- Diagnostic: F002197EF57D9B0BFB1D161BB164C155A432DCD8199050B9342CC41229BC4739
- No-log: BD519732994077B7B04929548974555D19C079056A5626A9A0048F67B386D84B

Confidence is high for repairing the two captured native faults. Full scene
behavior, special moving terrain, keystone riding/falling interactions and
other unvisited legacy methods still require gameplay testing. The separate
known exit-time stack overflow is not claimed fixed here.

User retest: return to the stage that just crashed and continue playing; in Ten
form place keystones facing both ways, near a wall and in open space, then
release/re-place and test riding them. Preserve retdec_trace.log if another
crash occurs and note which action preceded it.

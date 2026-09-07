# Hidden passage and walking off ledges

Scope: restore original behavior for passage crashes and enemies disappearing
at platform edges. Do not launch either game. Use IDA MCP, supplied C and
Squirrel 2.2.2 source/source-build assembly; execute only console contracts.

Read reverse-engineering and ida-reverse skills and required workflow/tool
references. Skill start.ps1/open.ps1 opened the original copy as b31919ba.
The registered connector cannot see this session; tools/ida_query.ps1 uses
the working managed IDA MCP HTTP endpoint.

## E-triage / E-imports

IDA survey: PE32, base 0x400000, 3963 functions, ordinary four sections.
SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Readable imports: CreateFileA/W, ReadFile, WriteFile, GetFileSize and
SetFilePointer; USER32/GDI32, D3D9/D3DX, WinMM, IMM32 and COM. Dynamic
LoadLibraryW/GetProcAddress is present. No packing blocker to static analysis.
The current diagnostic runtime contains only EXE and the three DAT files;
no current trace/dump is available. Previous evidence is in
../branch-return-20260907/report.md. Existing unrelated deletions and AGENTS.md
changes are preserved.

- [x] Read skills, project instructions and prior evidence; survey original.
- [x] Exercise ledges and moving-camera scenes; compare original collision paths.
- [x] Restore confirmed passage resource cleanup and diagnostic errors.
- [x] Build/test diagnostic and no-log variants; stage verified DATs.
- [x] Record evidence and include this work in a backup commit.
- [ ] User verifies passage entry/return and disappearing enemies in gameplay.

Game startup and visual/gameplay verification are reserved for the user.

## E-release

User clarified that the first-stage passage crashes when changing maps at
its far end, not while the hidden cover fades. Original 46F68B calls 450020
with the resource in ECX before freeing it. 450020 calls EndStage (450D80),
unregisters the environment (4513F0), closes resource-owned directory searches,
and destroys buffers, the critical section, the runtime link and its ACT.
451022..45104A retains the ACT name at resource+164 for that unregister call.
The reconstruction omitted both name storage and resource teardown, calling
free directly. After loading w1-c01a, w1-c01b and w1-c01a through the actual
LoadStage bytecode, ReleaseMap left the old named environment in the VM root.
The new assertion `!retdec_sqrat_get(root, retired_name, ...)` failed before
the fix and passes afterward.

Restore the registration name and resource teardown in the MapManager release
path. The existing reconstructed BeginStage shares the source ACT instead of
owning a clone, so its owner still destroys that ACT once; distinct owned ACTs
are destroyed by the runtime cleanup. Release the additional root reference
owned by the reconstructed registration helper. No script transition order,
map choice, actor spawn rule or original DAT content changes.

Also restore 452D31's sprite-vector clear: the finish pointer must be the saved
erase result (begin), not the destructor iterator after it reaches end. A
populated two-sprite fixture now checks begin==finish with capacity retained.

These are confirmed native mismatches. Without the user's current crash log,
their sufficiency to resolve the reported passage crash remains unconfirmed.

## E-anonymous-diagnostics

The VM error recorder and invalid-float recorder dereferenced prototype name
payloads as SQString pointers without checking the SQObject type. Anonymous
camera functions have OT_NULL names. A preliminary incomplete camera fixture
reached such an error and the error recorder itself raised an access violation.
Restore typed metadata reads and distinguish script/native call frames before
reading the prototype. The deterministic contract sets an anonymous closure's
OT_NULL name payload to zero and verifies that throwing unwinds successfully.
Source names receive the same check. This also applies in no-log builds because
they intentionally silence only the final output and retain trace call sites.

Squirrel 2.2.2 sqdebug.cpp::sq_stackinfos checks both name/source types before
reading string payloads. sqfuncproto.h confirms those fields are SQObjectPtr,
not unconditional strings. The source-built squirrel-sqvm.asm under
../evidence/raw/phase3-20260906 was also consulted while reviewing VM stack
ownership (SQVM::Remove, line 13024); that routine was not changed here.

Added bounded actor:update-failed records with actor ID, take, position,
velocity and camera bounds, plus script failure frame/instruction index.
These records do not change actor state or error recovery.

## E-scene-tests / Negative Evidence

User additionally reports enemies disappearing during ordinary walking and
suggests possible interaction with player movement. Tests now cover original
Init0106 fairies and Init0101 white kedama walking off a short platform and
falling through the visible area. Existing long walking, collision and
offscreen-wait/reset contracts still pass. No fall-specific workaround was
introduced because these tests did not reproduce premature disappearance.

The passage test executes original LoadStage and camera.cv4, actual native
LoadMap/ClearActor/ClearCollision/CreateCollision/CreateActorFromMap and render
layer registration, ACT layer callbacks, terrain/motion and actor render
transforms. It loads w1-c01a -> w1-c01b -> w1-c01a with 111/18/111 initial
actors and follows a moving player fixture for 180 frames per map. Coordinates
and free height stay finite; no unexpected script failures occur. A dummy
nonzero texture handle permits CPU render transforms with no D3D device;
the submit path returns before any device access. This is not visual QA.

The player, status, music, fade and event actions are fixture inputs/sinks,
not a full gameplay simulation. The first camera fixture lacked class_def.cv4;
loading its original Camera declarations before instantiation resolved that
fixture failure. Actor is a table sink in that class-definition fixture because
the actual Actor class is already locked by preceding test instances.

Exploratory 1800-frame camera traversals also encountered the previously noted
EnemyUpdateProc direct-Reset continuation and EnemyUpdate_Ball missing-take
errors. They are not promoted to reconstruction bugs without further evidence;
the supplied player fixture is incomplete and some actors move beyond the
ordinary wait/reset route. No production error suppression, preserved old user,
respawn, clamp, or modified enemy reset rule was added. Disappearance remains
an open gameplay finding. The old exit-time 429C70 defect is also unchanged.

## Verification and Handoff

Both Release builds pass CTest 2/2 (archive_smoke, stage_native_contract) and
the extended first-stage contract. Existing stone/block, eight-head expiry,
hidden-layer fade, item flight, VM unwind and terrain tests still pass.
stage_dat.ps1 copied and size/SHA256-checked all three DATs beside both EXEs.
No game was launched; no index.dat, save or configuration file was copied.

Extended console check from the repository root:

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603 analysis/player_ground.cv4 analysis/gameplay-contracts-20260907/player.cv4 analysis/constant.cv4 C:/WorkSpace/6kinoko data/map/w1-c01a.act
```

Repeat with p3-save-worldmap-notrace for the second variant. Outputs are in
each build tree's passage-final-contract.log. IDA comments were saved to
C:/rs-ida/7f350fb2-6kinoko.exe.i64 (temporary copy, original EXE unchanged).

Final EXE SHA256:
- Diagnostic: 71903DEDC6107D3697465531BED3C8F6A4B6E20C9A2A8599377893A26F5F1353
- No-log: 158BC9E97559823A04CBEC9F999D48B3AF9AFA7588FE3313F0B3079BAB58CE34

Manual test order:
1. Diagnostic EXE: enter the first-stage hidden passage, continue through the
   map transition, return and repeat three times. Include landing and jumping.
2. Observe the same walking enemy while standing still, following it and moving
   back and forth. Then observe a platform edge. Note whether it disappears
   inside the screen, after leaving it, or on actor contact.
3. Repeat passing cases with the no-log EXE.
4. On failure preserve retdec_trace.log and any crash dump before the next run;
   report the stage, movement/action, enemy appearance and approximate time.

ADF P0: native cleanup and diagnostic findings have static and offline execution
evidence. Complete passage resolution and enemy disappearance require user
runtime evidence. This checkpoint is a partial repair, not a fidelity claim.

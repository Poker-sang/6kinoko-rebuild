# Stage entity loading restoration

Scope: original C:/WorkSpace/6kinoko/6kinoko.exe, packaged DAT scripts,
and the rebuilt runtime. The user reports absent entities in simple stages
and crashes in complex stages. Do not launch either game executable; runtime
verification belongs to the user for this task.

## E-triage / E-imports

Read project AGENTS.md, reverse-engineering and ida-reverse skills and their
required workflow/tool references. Skill start.ps1 reused the HTTP server;
open.ps1 opened a temporary original copy as session d0caed36. Queries use
tools/ida_query.ps1 because the registered transport lists an unadopted worker.
IDA survey confirms PE32, base 0x400000, entry 0x4aca23, 3963 functions,
normal text/idata/rdata/data sections. Original SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Readable imports include CreateFileA/W, ReadFile, WriteFile, GetFileSize,
SetFilePointer, Direct3DCreate9, D3DX, WinMM, USER32, GDI32, IMM32 and COM.
No direct network/crypto imports in this table; LoadLibraryW/GetProcAddress
permit dynamic resolution, so the table does not prove absence of capabilities.

## Work items

- [x] Read skills, establish scope and survey original/imports.
- [x] Inspect existing logs and block.cv4 without launching the game.
- [x] Compare entity registration and map creation against original assembly.
- [x] Restore confirmed divergences and verify offline.
- [x] Build diagnostic/no-log executables and stage hash-checked DATs.
- [x] Record test instructions; this report is included in the backup commit.
- [ ] User runtime verification: simple and complex stages.

## Existing observations

p3-save-worldmap-diag/retdec_trace.log records block.nut main failing with
SET opcode 0x0d and `the index doesn't exist`. block.cv4 registers Init names
through SetInitFunctionByID, then assigns aliases with SET. The rebuilt
471D30 adapter forwards only the integer, dropping both Squirrel objects.
471E50 similarly forwards only a string to CreateActorFromMap.
These observations motivated the original comparison below. Complete game
behavior remains pending post-fix user testing.

## E-registration

Original 47325F registers 470FA0 through 471D30/471240. The adapter passes an
integer and two 12-byte SquirrelObject values, closure followed by environment.
470FCB..47101B requires OT_CLOSURE/OT_TABLE and rawsets Init%04x into that table.
Restore both parameters, ownership, and the explicit receiver of 4A9840.

Squirrel 2.2.2 sqapi.cpp::sq_rawset calls SQTable::NewSlot for tables, while
sq_set/SQVM::Set require an existing slot. The source build disassembly in
../evidence/raw/phase3-20260906/squirrel-sqvm.asm, SQVM::Set, first calls
SQTable::Set then follows delegation on a miss. This agrees with the original
block.cv4 failing on SET after the broken registration never inserted its key.
No VM opcode semantics or DAT script were changed.

## E-map-spawn

46F140 enumerates active layouts through 455890/452020, checks C2DMapLayout,
and compares the parent layer name. The reconstruction omitted the native
type-query assignment, so its result stayed zero, and also wrote bogus stack
arguments on each iteration. Restore the typed lookup and full name comparison.

469D10/471E50/471510 pass a layer name and the script environment. Restore the
missing environment and 463E60's explicit ActorManager receiver. 463E60 walks
32-byte layout records, resolves Init%04x in the supplied table, skips records
without script closures, and invokes the existing 463B40/45E5E0 actor path.
MCD records are accessed through the already reconstructed chip-data helpers.
463FD4..464049 establishes signed width/height, x += width*0.5+1,
y += height*(flag 0x10000 ? 0.5 : 1), z=-1, and the 48-byte init-data copy.
The script receives the original integer chip ID.

## E-collision-and-events

46F140's other direct consumers were checked before enabling the repaired
lookup. CreateCollision's 4693A0 also lost its receiver and forwarded junk
to vector and actor operations. Restore its coordinate preparation, original
hidden collision proxy, prepend order of paired layout/weak-actor vectors,
and scratch-vector resize. 468950 now resets the weak-vector end to its
begin after releasing its entries, as the original erase/cleanup sequence does.

45E65F..45E715 separately allocates an Actor* slot, puts it under shared
ownership, and writes this through the slot. The reconstruction had stored
the control-block address in both Actor+24 and Actor+28. Restore the separate
slot, since collision and parent lookups dereference Actor+24 as Actor**.

CreateEvent's 471720/469DD0 similarly dropped the callback and its environment.
46FD70 and 46EE20 now pass the original id,left,top,right,bottom arguments
using MCD dimensions and the supplied callback receiver. Invalid missing MCD
records return a diagnostic failure instead of using undefined rectangle
locals. Original DAT event records with valid rectangles follow the original
path. This is not a replacement event rule or a hard-coded stage exception.

Original IDA comments were saved successfully to
C:/rs-ida/e76ea5f7-6kinoko.exe.i64.

## E-offline-validation

tests/stage_contract.c compiles the actual reconstructed C into a separate
console test. It has no WinMain, game window, rendering, audio service or
game-startup call. It creates a VM and controlled native fixtures.

Coverage:
- Real native registration adapters, argument ownership and caller table.
- Original block.cv4 execution through the reconstructed bytecode reader/VM.
- Full named-map adapter, exact name match and missing layer/table entries.
- Original spawn order, integer ID, signed coordinates, pivot flag and init data.
- Actor slot/control-block separation and collision proxy creation/clear.
- Event callback receiver, record order and integer rectangle arguments.
- 600 additional actors beyond the original 512-slot initial pool; 605 total
  actors including two collision proxies, and unchanged VM stack depth.

Both Release build trees p3-save-worldmap-diag and p3-save-worldmap-notrace
pass archive_smoke and stage_native_contract (2/2 each).
Both also pass the contract executable with the real block.cv4 extracted by
kinoko_asset_probe from the effective archive=2 entry (33541 bytes).
The original base copy analysis/block.cv4 also passed before this final check.
The initial test fixture used multiple statements on one line, which the
existing source compiler rejected; using separate lines resolved that test
syntax issue. It was not an additional game regression.

Reproduce the packaged-script check from the repository root:

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_asset_probe.exe ../6kinoko data/script/block.cv4 analysis/stage-entities-20260907/block.cv4
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4
ctest --test-dir build-runs/p3-save-worldmap-diag -C Release --output-on-failure
ctest --test-dir build-runs/p3-save-worldmap-notrace -C Release --output-on-failure
```

stage_dat.ps1 copied and hash-checked all three DATs beside each EXE.
Save/config files were preserved. No data-directory or working-directory
override was used. Trace calls remain present in the no-log build; only
output/capture is silenced.

Final EXE SHA256:
- Diagnostic: E28AC10D014F22AAC646175244636FB4BB7004C48599867BA4D1C71B632865FB
- No-log: 0800F7097BDB0F53D792D6DD351F2C882FA20B01EC540544FF399E2290BD0E9F

## User test

Start runtime-builds/p3-save-worldmap-diag/kinoko_retdec_rebuild.exe.
1. Enter the same simple stage: check the player, enemies, blocks and items.
2. Test movement, jumping, attacking, landing and hitting a block.
3. Return to the world map and reenter to check creation/clear lifecycle.
4. Enter the previously crashing complex stage; report its name/number and
   whether any crash is immediate or follows a particular action.
5. Repeat with p3-save-worldmap-notrace to compare the no-log build.

For remaining failures retain the diagnostic runtime's retdec_trace.log and
any generated crash dump. Useful markers include actor:map-layer,
actor:map-records, actor:map-created, actor:init-registration-result,
actor:collision-layout-count, map:event-created and stagevm:failure-*.

No original/rebuilt game was launched during this task. Full visual, input,
collision physics, stage-specific scripts and crash resolution are not
claimed as runtime-validated. P0 synthesis: the repaired native contracts
have original static anchors and passing offline execution tests; the user's
reported gameplay outcome remains a candidate pending their test.

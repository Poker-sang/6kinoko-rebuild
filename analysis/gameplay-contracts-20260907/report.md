# Gameplay native and script contract restoration

## Scope and workflow

User requests fixes for Ten falling-stone/block crashes, leaving eight-head
form, intermittent landing death, hidden passages, enemy behavior, and floating
point/save/1up items. Neither original nor rebuilt game may be launched.
Use original assembly and packaged scripts as behavior specifications; console
contract tests may execute the reconstructed code without game startup.

Read AGENTS.md, ida-reverse and reverse-engineering skills, precedent-reverse,
tool-index and re-agent-workflow. start.ps1 reused the managed IDA server;
open.ps1 opened an original temporary copy as 1e3b2cb1. The registered MCP
client reports that session missing; tools/ida_query.ps1 reaches the same IDA
MCP tools through the managed HTTP server and works.

## E-triage / E-imports

IDA survey: PE32, base 0x400000, entry 0x4aca23, 3963 functions, ordinary
.text/.idata/.rdata/.data sections. Original SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Readable imports queried through IDA MCP include CreateFileA/W, ReadFile,
WriteFile, SetFilePointer, GetFileSize; USER32/GDI32 windows/text; WinMM time;
D3D9/D3DX rendering; IMM32 and COM. No direct network/crypto imports in this
table; LoadLibraryW/GetProcAddress allow dynamic resolution, so this is not
a capability-absence claim. Previous target/import evidence remains in
../stage-entities-20260907/report.md.

## Work items

- [x] Read instructions and survey original/imports.
- [x] Locate current crash RVAs in the matching pre-build linker map.
- [x] Compare affected native and Squirrel paths against original evidence.
- [x] Restore confirmed divergences and reproduce them in console tests.
- [x] Build/test diagnostic and no-log variants; stage checked DATs.
- [x] Record final evidence and commit backup.
- [ ] User gameplay and startup verification.

## E-current-crashes

Targeted search of the existing 2.56 GB diagnostic trace found recent native
faults at RVA 0x5404D and 0x5442A, both in function_4682a0 (preferred image
0x453FD0). Caller 0x44DEC5 is in function_45f820, Actor::IsExistChip.
Another fault has RVA 0x1208, resolved below to SQUserData destruction. Exit-time stack
overflows recurse through function_429c70 at preferred image 0x430670.
The pre-build map is build-runs/p3-save-worldmap-diag/kinoko.map.

## Verification boundary

No game launches. All gameplay outcomes remain pending user testing even
when static comparisons and offline contract tests pass.

## E-math / E-floating

4C6820 and 4C6AA0 in original IDA agree with Squirrel 2.2.2 sqstdmath.cpp:
read SQFloat arguments, call the CRT math operation, push SQFloat. RetDec
sqrt/asin/acos/log/log10/tan/atan/atan2/pow/exp used uninitialized x87 locals;
floor/ceil converted the result to an integer instead of passing float bits.
Restored all twelve damaged wrappers using the existing float-bit helper.
The pre-fix console test fails with `sqrt distance`; all math checks now pass.
Squirrel source-build disassembly reference remains
../evidence/raw/phase3-20260906/squirrel-sqvm.asm (VM/native ABI).

Original effective item.cv4 (archive 2, offset 3097384, 54344 bytes) runs in
the actual rebuilt VM with actual Actor initialization/Step/Update. Twelve
point/save/1up flights accelerate away, decelerate, home at 12 pixels/frame,
and call their original reward/release functions only within 16 pixels of
the player. Test doubles count rewards/media; they never write saves.
The fixture initially lacked PR_FRONT and checked the visibility byte instead
of the pending-release byte (+22); those test mistakes were corrected.

## E-region / E-event-query / E-immediate-callback

45F820 and 4682A0 original assembly confirms ECX receivers, float32 rectangle
arguments, ret-16 / ret-20, enabled map-layer queries and inclusive rectangle
overlap against native actors excluding the receiver. Restore the script
entry and region query using the existing 435220-compatible map scanner.
Repeated native script calls cover populated/empty/disabled layers, actor
exclusion, touching and disjoint rectangles, and stable VM stack depth.

45F7A0 forwards truncated Actor x/y, event-layer index and Actor+512+index*4.
46EF40/4355F0 use map query candidates followed by half-open point containment,
publishing last_id/left/top/right/bottom. Restore this path. Invalid indices
return a missing ID instead of dereferencing past the vector; valid original
indices follow the original path. 46FDB1..46FDB7 appends an event layer even
with a null callback or missing layout. That append was missing entirely;
restored before callback processing. Tests cover no-callback registrations,
multiple indices, boundaries and missing layers.

45F800/4697A0/462F30 also lost receivers. Restore immediate callback traversal
over active actors in manager order through existing 462CE0. Script test
verifies both callback directions and the original order.

Checkpoint: default native contracts and extended w1-c01a contracts pass.
Game binaries are not rebuilt/staged yet at this checkpoint. Further ACT
script loading, enemy, transformation and collision review remains active.

## E-hidden-include

The actual w1-c01a ACT hidden layer has inline source
`CompileFile("Data/Script/stage/HiddenLayer.nut", u);`. The environment lacked
`u`, producing the repeatedly logged ACT-inline GET failure. Original IDA
416056..41605F passes the same Sqrat object as both arguments to 415870,
installing a self-reference when u is null/missing. Restore this during ACT
environment preparation, preserving an existing non-null u.

The regression runs the actual ACT layer publisher and original include
through CompileFile, then its retained Init/Update callbacks. Before the fix
it reproduces the ACT-inline GET failure. Afterward the native C2DMapLayout
blend is alpha, entering the real hidden rectangle fades alpha to zero, and
leaving restores it to one. The test enables the original packaged-script
mode (g874=1); it does not replace the include or the fade code.

## E-delegate / E-enemies

Full enemy.cv4 loading includes the original enemy class, base, update, hit,
event and pattern scripts from the effective archives. Two real Init0106
actors reproduced shared event tables despite distinct EnemyInfo instances.
Both constructors ran, but DELEGATE used the broken 48E520 entry with no ECX
receiver. 490C80 also used the newly assigned type to release an old value.

Original IDA 490C80/48E520, sqvm.cpp::DELEGATE_OP,
sqobject.cpp::SQDelegable::SetDelegate, and source-build disassembly
squirrel-sqvm.asm::DELEGATE_OP+64..8B agree on the receiver, operand order,
cycle check, reference changes and assignment. Restore those operations
through the existing explicit-receiver helper, including actual release of
an expired delegate. No enemy-specific branching or DAT edits were added.

The same two-enemy regression now passes distinct user/event/data/child
tables, bound owner identity, full update callbacks, walking, landing and
wall reversal. Additional generic checks cover delegated inheritance,
independent writes, cycle rejection and reference counts.

## E-userdata / E-transform

RVA 0x1208 maps to 48BF50, the SQUserData scalar-deleting destructor. Its
uninitialized receiver was still present even though the ordinary Release
helper had already been reconstructed. Restore the thiscall/ret-4 entry and
the matching Finalize vtable entry. Share the destructor with Release while
keeping its release hook before destruction. IDA 48BF50 and Squirrel 2.2.2
squserdata.h confirm removal from the GC chain, delegate release, weak-ref
invalidation and optional free. Thirty-two offline cycles exercise the real
vtable entries and verify delegate counts and weak-ref expiry.

The actual effective player.cv4 Update/SetType/SetTake paths run 32 eight-head
expiry transitions, checking restored type/take, finite bounds, VM stack and
PlayBgmMargin's five arguments through the native binding. Portrait/status
and audio output are test sinks. This does not establish that every reported
eight-head crash had this destructor as its cause; user retesting is required.

## E-stone-block

Original InitStone/CallbackStone establishes a real rider. Walking off changes
the original update/collision callbacks to UpdateStone2/CallbackStone2. The
test lets the stone fall onto an original Init0435 point block, dispatches the
real collision pair, and executes CreateItemCommon's IsExistChip branch.
It checks stone release, retired block callback/damage function and downward
item direction, then clears the real native actors. The point block is
chosen from the original ID registration: 0433 has no item and 0443 uses the
separate break-block initializer. No production behavior was changed to fit
those initially incorrect fixture assumptions.

## E-remaining

Old logs show player y/freeHeight already NaN before SetDead(false), including
TYPE_2HEAD and TYPE_TEN. The first invalid coordinate was suppressed by the
old trace cadence, so the producing instruction is not identified. The
shared math/delegate/lifetime fixes are relevant candidates but are not
proof of this symptom's resolution. Existing terrain/player tests and the
new transformation/enemy cases remain finite. Do not replace death checks
or clamp/respawn invalid coordinates to conceal the upstream defect.

Other limitations: full visual/audio behavior, all enemy patterns/bosses,
startup and gameplay await user execution. The previously known exit-time
429C70 stack overflow is not fixed by this gameplay patch.

## Final verification and handoff

Both p3-save-worldmap-diag and p3-save-worldmap-notrace Release builds pass
archive_smoke and stage_native_contract (2/2 per build). Both also pass the
extended asset/script contracts for w1-c01a, w3-s01a and w7-s01a, including
all the new floating/hidden/enemy/transform/stone-block cases. Only console
test executables were run; neither game was launched. Trace-output behavior
and the no-log build switches were not changed.

stage_dat.ps1 copied and SHA256-verified the three original DATs beside each
game EXE. Save/config files and unrelated user edits were preserved. No
reference-directory working-directory or data-directory override was used.

Final EXE SHA256:
- Diagnostic: AB37E455B3CABB907A23B761BE2F4B10BB0191F8FF8185F02B0647DB121A1514
- No-log: 2594A42003C9A084A6AE5505CE843F7B7F83198D3C43711DCB362026AA2F4820

Reproduce extended console checks from the repository root:

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603 analysis/player_ground.cv4 analysis/gameplay-contracts-20260907/player.cv4 analysis/constant.cv4 C:/WorkSpace/6kinoko data/map/w1-c01a.act
```

Repeat with p3-save-worldmap-notrace and the other two map names as needed.
The extracted CV4 files in this directory are unchanged original evidence.
The first backup checkpoint is 1c1a8c8; the second includes the remaining
source, tests, extracted evidence and this final report.
The initial IDA worker expired before annotation. The skill scripts reopened
the same original as a42e5051; final comments were saved successfully to
C:/rs-ida/c7979d62-6kinoko.exe.i64.

User tests, diagnostic build first:
1. Ten: place/ride/walk off a stone and let it fall onto both ordinary and
   item-containing blocks; repeat facing both ways and near walls.
2. Let eight-head expire while standing, moving and near terrain; check
   restored appearance/collision and BGM transition.
3. Enter/leave the first-stage hidden passage; check its cover fades both ways.
4. Collect point/save/1up items and check return flight and one reward each.
5. Observe multiple enemies; check independent movement, wall reversal,
   collision, damage, hold/throw and reentry/reset.
6. Retest the formerly disappearing player's landing with the same stage/form.
   This symptom remains unconfirmed, as does the exact eight-head crash cause.

After the diagnostic cases pass, repeat with the no-log build. Preserve the
diagnostic trace and any dump on failure, and record stage, form and exact
action. P0 synthesis: restored contracts have original static and offline
execution anchors (R41/R4*); gameplay claims are bounded by no-launch scope
(R7). NaN and broader enemy behavior remain candidates for user validation,
not promoted to fully resolved. Ordinary game reconstruction; no IOC claims.

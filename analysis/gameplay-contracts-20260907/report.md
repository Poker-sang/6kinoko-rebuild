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
- [ ] Compare affected native and Squirrel paths against original evidence.
- [ ] Restore confirmed divergences and reproduce them in console tests.
- [ ] Build/test diagnostic and no-log variants; stage checked DATs.
- [ ] Record final evidence and commit backup.
- [ ] User gameplay and startup verification.

## E-current-crashes

Targeted search of the existing 2.56 GB diagnostic trace found recent native
faults at RVA 0x5404D and 0x5442A, both in function_4682a0 (preferred image
0x453FD0). Caller 0x44DEC5 is in function_45f820, Actor::IsExistChip.
Another fault has RVA 0x1208; it remains to be resolved. Exit-time stack
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
Restored all eleven damaged wrappers using the existing float-bit helper.
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

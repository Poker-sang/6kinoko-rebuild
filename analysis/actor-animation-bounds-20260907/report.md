# Actor animation bounds and aliases

Follow-up to 35a36e3: user reports death shortly after entering shops/stages,
sometimes with no visible player. No game launches. Preserve the original
spawn and death rules; compare native animation setup and packaged assets.

Carry forward skill/scope and E-imports from
analysis/stage-entities-20260907/report.md. Read the reverse-engineering and
ida-reverse skills and required references again on resume. The prior IDA
worker 248bcd08 was unreachable; skill start.ps1/open.ps1 reopened the original
as 078728c8. Survey verified PE32, 3963 functions and the same original SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.

- [x] Inspect current logs and original player death conditions.
- [x] Compare SetTake and PAT parsing against original disassembly.
- [x] Restore confirmed divergences and add offline coverage.
- [x] Build both variants and stage hash-checked DATs.
- [x] Include source, tests and this report in a backup commit.
- [ ] User verifies shops and ordinary stages.

Original player.cv4::Update checks time==0, hitTop && hitBottom && freeHeight<20,
freeWidth<12, bottom>=camera.bottom+96, and terrain hazard flags before death.
No changes are planned to these conditions.

462280's reconstructed explicit-receiver wrapper still executes RetDec x87
temporaries with uninitialized register indices and pointer-derived branches.
Original 4622E7..462457 derives local/world bounds from four signed PAT ints,
actor scale and direction, then stores the first animation frame.

464F80 stores -1 alias pairs and resolves them after all full records, in
reverse input order. The reconstruction discarded those pairs. It also
ignored -2 continuation links and only accepted the first frame's collision
rectangle, while original 465BDE accepts the first available rectangle.

## E-bounds: confirmed native divergence

Replaced the damaged 462280 body with the original signed-int bounds conversion,
local padding, scale/direction transform, int16 dimensions, frame selection and
total duration assignment. Take ID and frame counters are still written before
lookup; missing takes still preserve the selected animation and its bounds.
No-collision animations retain the original zero local bounds behavior.

The regression test perturbs all eight simulated FPU registers and checks
asymmetric bounds, mirrored/scaled bounds, dimensions and first-frame pointers.
Before the correction, the world-left assertion (93.5) failed. It now passes
independently of those simulated registers. Original assembly 4622E7..462457
is the static anchor, with offline execution of the actual rebuilt function
as the behavioral check. Gameplay confirmation remains pending.

## E-pat: restored loader branches

Factored animation-record parsing so the packaged loader and console tests run
the same code. Restore -1 aliases in reverse input order after loading nodes;
missing source IDs do not create or replace a take. Restore -2 forward/back
links, and capture the first available collision rectangle on any frame.
Free the pending node/frames/auxiliary data on a read failure.

Synthetic coverage verifies a forward-reference alias chain, missing/cyclic
sources, duplicate destination precedence, two continuation nodes with an
alias between them, independent normal nodes, late-frame collision bounds,
duration accumulation and full stream consumption. Ordinary Actor ticking
still loops within its current node: no invented continuation stepping.
Original 45E120 and 46537A/465BF6/465DA9 were checked with IDA and annotated;
the temporary-copy database was saved.

## E-player-pat: actual asset coverage

Archive 0, data/actor/marisa/marisa.pat, offset 150465603, size 123769.
Encrypted extracted bytes SHA256:
7A5E5260B46F7E5E483C24321B709F8B0695C66F5A1DD4E64259CAC6E91216AE.
The package XOR key is uint8((offset >> 1) | 0x23). Header version is 5;
an initial test-only assumption of version 4 was corrected after inspecting
the bytes. Production header acceptance was not changed.

The actual runtime parser consumes all 123769 bytes, with 307 texture names
and 227 selectable animation nodes. All 227 take IDs below 10000 select a
non-null frame and ordered bounds in both directions. These player takes have
no aliases; therefore alias restoration is NOT evidence of the player-death
cause. The native SetTake bounds corruption is the direct confirmed defect.

Take 0 at (100,200), unit scale: world bounds (89.5,170,110.5,201), dimensions
21 x 31. Passing this actual loaded animation through the terrain narrow phase
lands at y=239 on a floor at y=240, with hitBottom=1, hitTop=0 and freeWidth=21.
This fixture does not trigger either of the original crush predicates.
No floor, spawn, camera or death rule was added to production.

Original player.cv4::SetTake computes user.type*100 + requested_take, plus one
when holding an item. Inspection uses tools/inspect_cv4.py and the Squirrel
2.2.2 SQFunctionProto::Load/SQInstruction format and opcode definitions.
No Squirrel execution semantics or DAT scripts changed in this correction.

## Verification

Both Release builds, p3-save-worldmap-diag and p3-save-worldmap-notrace, pass
archive_smoke and stage_native_contract (2/2 each). Both standalone contract
executables also pass with the effective original block.cv4, stage.cv4 and
marisa.pat. Existing movement, lifecycle and start-display tests remain passing.

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_asset_probe.exe ../6kinoko data/actor/marisa/marisa.pat analysis/actor-animation-bounds-20260907/marisa.pat
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603
runtime-builds/p3-save-worldmap-notrace/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603
```

Game EXE SHA256:
- Diagnostic: 5BA52589659ED5977691D9BCC02BBF4CC159A3E912D8E3288B501BDBCACF7118
- No-log: 426BAE6104DC3CF71548DD95F1D69359E9914A2E9C98117A14C5C52752785EE9

stage_dat.ps1 copied and verified all three DATs beside each EXE. No game was
launched, so the requested manual test replaces the usual startup smoke test.
No save/config files were changed. Full gameplay/visual fidelity and the
previously noted ACT-inline GET failure and exit-time stack overflow are not
claimed fixed here.

## User retest

1. Launch runtime-builds/p3-save-worldmap-diag/kinoko_retdec_rebuild.exe.
2. Enter the same shop, initially release all controls and watch whether the
   player appears, lands and remains alive. Test walking, jumping and landing.
3. Return to the map and reenter the shop three times; repeat in the ordinary
   stage, allowing its start overlay to finish before testing controls.
4. Repeat with p3-save-worldmap-notrace after the diagnostic run succeeds.
5. On failure, report stage/shop, player form, holding-item state, whether the
   player was visible, approximate delay until death and which build was used.
   Preserve retdec_trace.log in the diagnostic run directory.

Decision delta: confirmed bounds/loading mismatches restored; offline checks
pass. Return to user runtime testing before promoting the symptom to resolved.

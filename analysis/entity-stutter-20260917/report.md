# Composite enemy stutter repair — 2026-09-18

## Scope and evidence

Original: C:/WorkSpace/6kinoko/6kinoko.exe and its unchanged three DAT files.
No enemy parameters, scripts, loading order or game rules were changed.
IDA MCP survey identified native x86 PE, SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
E-imports: imports.json, readable file I/O, synchronization/thread, timer,
window/input and graphics/audio imports. Initial session 8a905bce became
unreachable; reopened with bundled start/open scripts as 866c7b77.

E-static: original-492ed0.json and original-492ed0-asm.json show
SQVM::CallNative calling [closure+60] directly at 493119. There is no
VirtualQuery diagnostic. The supplied ../squirrel-2.2.2/SQUIRREL2/squirrel/
sqvm.cpp agrees, as does its vendored byte-identical source, compiled
sqvm.obj disassembly in source-sqvm-disasm.txt (CallNative symbol).
original-callnative.json is the earlier exploratory 497A00 table lookup,
not CallNative; it is not the evidence for the native-call conclusion.

E-assets: extracted unmodified anchor, kedama-combination, kedama-cannon,
kedama-black CV4 and decoded listings remain in this directory.
E-user-log: selectively sampled the 4,175,955,455-byte
runtime-builds/final-destructor-20260917-r1-diag/retdec_trace.log.
Samples are dominated by property lookup/reference/actor trace records;
no script exception was found by targeted error search. User confirmed
both quiet and diag stutter, so file writes alone could not explain it.

## Cause and repair

The quiet sink discarded output only AFTER retdec_trace_i32/name had
formatted strings. Composite enemies amplify these per-property and
per-reference calls. Every native closure call additionally queried Windows
memory protection only to print a reconstruction diagnostic. Neither work
is part of original game behavior.

The existing C++ diagnostics module now answers whether a label will be
consumed before the C formatting helpers perform formatting. VM trace
call sites and out-of-line helpers remain; the VM has not been compiled
with trace calls removed. Quiet script-failure capture still receives its
formatted metadata. The per-call VirtualQuery block was removed, restoring
the direct invocation confirmed in original assembly and Squirrel source.

Normal diagnostics retain failures and scene transitions. Detailed tracing
requires KINOKO_TRACE=1 plus KINOKO_TRACE_VERBOSE=1. It remains deliberately
opt-in and can still be expensive. Ordinary builds remain quiet with no
automatic screenshots. Crash/debugger access remains available.

## Measured comparison and tests

Offline fixture: original enemy scripts, actual native Actor/animation data,
three roots, 240 updates; no rendering or sound service. PlaySE/CreateEffect
are test-only observation stubs. This is an update-cost comparison, not a
claim of full-game FPS or original executable frame-for-frame equivalence.

| Case | Before, r3 | After, r4 | Actors |
| --- | --- | --- | --- |
| Tall stacks Init0989 | about 20–35 ms/update | about 2.4 ms/update steady state | 36 |
| Anchors Init0cfb | about 19–38 ms/update | about 1.9–3.7 ms/update | 393 |

Original anchor scripts preallocate chain objects; the large actor count is
preserved, not reduced. r3 sampling artifacts remain for inspection, but
incomplete startup module enumeration means unknown instruction samples
are not assigned to a particular Windows function.

Final fixture additionally uses original upward rounding, checks stable VM
stack depth, exact composite counts, and successful cannon spawning/effects.
All 21 CTests passed in BOTH r5 quiet and r5 diag, including prior platform,
water, collision, VM ownership, ABI, animation, texture and diagnostic checks.
Diagnostic capture test verifies quiet numeric filtering while retaining
script-error metadata. All build trees, executables, logs and samples remain.

Batch commits:
- r1: f38961e (fixture missing original statement separators; retained failure)
- r2: 494505f (fixture missing math registration; retained failure)
- r3: 14402dc (valid baseline composite tests)
- r4: f34f735 (performance fix; cannon fixture later needed effect observer)
- r5: ebaa46e (final tests and documentation; 21/21 per build)

Final EXEs:
- runtime-builds/entity-stutter-20260918-r5-quiet/kinoko_retdec_rebuild.exe
- runtime-builds/entity-stutter-20260918-r5-diag/kinoko_retdec_rebuild.exe

stage_dat.ps1 copied and SHA256-checked the three DAT files beside both EXEs.
run_staged.ps1 launched them with WorkingDirectory unset. Quiet gameplay and
diag first-stage gameplay were observed; user actively supplied inputs and
confirmed validation. A tool key attempt was rejected due to intervening
user input, so no claim is made that the agent independently completed the
jump sequence. Both processes subsequently exited. The user explicitly
confirmed both reported stutters fixed and faster scene changes.

No additional gameplay bug has been established. Full C++ VM replacement,
remaining legacy asm/CRT code and broad migration are future incremental work,
not claimed complete in this repair.

Checklist: skills/project instructions read; scope/imports recorded; original
IDA and Squirrel source/disassembly compared; changes committed before tests;
quiet+diag tested; DAT staging verified; all historical artifacts retained.

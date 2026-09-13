# Road-unlock crash

Scope: fix the user's post-clear route-unlock crash in the existing rebuild,
preserving original DAT logic and unrelated worktree deletions. Reference
files and previous test artifacts are read-only. Use isolated new test outputs.

## E-imports

IDA MCP session f9ff9241, survey_binary standard, original SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
PE32 base 0x400000, 3963 functions, readable conventional sections and imports:
file IO, USER32/GDI32, Direct3D9/D3DX, WinMM, IMM32 and COM; LoadLibrary and
GetProcAddress permit dynamic imports. SendMessageA/RegisterClassExA are
misclassified by the survey and do not establish networking/registry behavior.

## E-crash

The user identifies both gc-fix-20260913-r5-diag first-chance dumps as the
reported road-unlock failure. Both fault at EXE RVA 0xa2499 in
retdec_squirrel_release, reading 0xffbe8200. First dump stack:
retdec_squirrel_assign -> retdec_clean_vm_assign_integer ->
retdec_execute_clean_vm -> function_497680_this -> function_48ace0 ->
function_415810_this -> function_451640 -> function_466050 -> function_469900.
This establishes a bad old value during a VM slot assignment, not its cause.

## Work items

- [x] Read skills, project constraints, previous reports, and crash stacks.
- [x] Recover faulting script/slot and identify original behavior.
- [x] Implement a source-grounded C++ correction and regression coverage.
- [x] Commit before testing; build separate diagnostic and quiet candidates.
- [x] Verify contracts and bounded gameplay smoke; retain all artifacts.

## E-script and original implementation

Dump inspection identifies data/System/effect/EffectLayer.nut::Update,
instruction 22 (LOADINT r2,1 following effectList.len()). The old r2 value is
the effectList SQArray with corrupt vtable 0xffbe81fc. Original packaged
EffectLayer.cv4 was extracted unchanged into this analysis directory.

IDA 490683..490689 saves caller _top-_stackbase before Resume changes frames;
490883..490889 restores this saved value into CallInfo::_prevtop. Recovered C
instead saved the resumed generator's size. IDA 49076b..4907aa clears saved
slot zero, matching the supplied upstream Resume implementation. Use the
actual Squirrel 2.2.2 Yield/Resume through typed C++ bridges with layout checks.
The existing source-build squirrel-sqvm.asm at offsets 1ee7..1f58 independently
shows Yield, trap adjustment, restoration of temp_reg to the source slot, then
Return. The rebuilt clean VM omitted that restoration and trap adjustment.
RETURN also now kills a completed generator as the original Execute does.

IDA 4A36D0 confirms full SQObjectPtr result retention across array Remove and
Push. The C reconstruction had an incomplete stack temporary, lost native
receivers, and fabricated stack writes. The C++ adapter uses upstream SQArray
Get/Remove and SQVM::Push, preserving numeric-index/error/result behavior.
About 500 lines of generated generator/array code were replaced with source
calls and short typed adapters. No DAT, route condition, or save rule changed.

## E-regression and user validation

Source/test checkpoints and per-EXE SHA256 values are in each new
runtime-builds/road-20260913-*/validation.json. All corresponding build trees
and runtime files are retained, including unsuccessful test candidates.
All nine generated EXEs have the three DATs copied and SHA256-verified beside
them. Baseline probes failed (first yield became null, effect stopped at step 1,
list not removed). The fixed short generator completes at step 3 and clears.

Final source a18716d0eea29053047d959d486575945ae6b9e7, r4-diag and r4-quiet:
CTest 4/4 each, including generator return values, caller stack, array virtual
pointer, removal ownership and error behavior. Both also execute original
EffectLayer Update and GenSmokeEffect, recording five draws of each of five
smoke frames (25 total), then normal generator completion and empty list.
Only the draw boundary is a fixture; the original bytecode is unchanged.

Both game EXEs launched with run_staged.ps1 (WorkingDirectory unset), showed
first-stage gameplay, airborne player and enemies, and responsive windows.
The user took over input; agent stopped input when the UI tool reported that.
The user explicitly confirmed the road problem fixed after playing through.
Quiet run produced no trace or automatic capture files. A first quiet launch
while diagnostic was still active exited 1 (single-instance); after diagnostic
exit the quiet launch succeeded. No archive/config/save bypass was used.

## Separate cleanup exception

Diagnostic PID 29720 subsequently failed during shutdown. Retained WER dumps
are runtime-builds/road-20260913-r4-diag/shutdown-29720*.dmp. First dump records
c0000409, parameter 0x15, with function_450020+0x31 -> function_469680+0x79 ->
function_40d940 -> WinMain. run_staged finally reported -1073741819.
This is separate from the confirmed road fix and is not claimed fixed here.
The user requested a checkpoint followed by fixing this cleanup exception.

ADF P0: static/source evidence establishes the generator and remove defects;
baseline/fixed probes, original smoke bytecode and user gameplay confirmation
support the road repair. The exact first write corrupting the old dump's
vtable was not captured. No claim of universal game stability is made.

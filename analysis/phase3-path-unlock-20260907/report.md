# New-save world-map route visibility

Scope: restore original new-save path locking and progression-driven unlocking.
Original DAT scripts and save/config files remain unchanged. Test with separate
runtime copies, including absent saves and controlled progress fixtures.

## Carried evidence

Original PE32 SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
E-imports/survey from ../phase3-save-worldmap-20260906/report.md remain valid:
normal readable file/Direct3D/WinMM/USER32/GDI32/COM imports. IDA skill
start.ps1/open.ps1 reopened the original as cbd6ff39; survey confirms same hash.

## Work items

- [x] Review original save initialization and map mask scripts.
- [x] Identify original/rebuilt state divergence.
- [x] Restore the missing native/VM behavior.
- [x] Build diagnostic/no-log executables, run archive tests and stage DATs.
- [x] User confirmed the reported new-save route-locking issue is fixed.
- [x] Commit checkpoint and record evidence.

## E-script

WorldMap.Init enumerates hidden_mask* layers into maskLayer. InitMaskLayer
turns off matching rail ChipLayout.visible/alpha. ProcMaskLayer restores routes
only if the original stage's clear bit matches the encoded mask route index,
with additional original extra-stage conditions. InitSaveDataTable gives a new
save zero clear/open counts and false world clear flags. InitStageSaveData
initializes each stage's clear bitfield to zero.

## E-divergence

The pre-fix fresh runtime loaded ten hidden_mask* native layers, but
WorldMap.global.maskLayer stayed empty. PreArrangement saw 441 rail records
and zero hidden records. The prefix-specific slice trace was never reached.
Artifacts are under ../evidence/raw/phase3-path-unlock-20260907/.

WorldMap.Init calls key.tostring().len() and key.tostring().slice(0,11) before
comparing to hidden_mask. Native 4A2360 -> 48A720 -> 492440 implements tostring.
The reconstructed 492440 handled bool/int/float but omitted OT_STRING, routing
existing strings to the fallback formatter `(string : 0x...)`. Prefix matching
therefore failed for every mask layer and InitMaskLayer had nothing to hide.

Original IDA 492505..492532 copies both words of the existing string and adjusts
reference counts. Squirrel 2.2.2 sqvm.cpp::ToString has `case OT_STRING: res=o;
return;`. Its built sqvm.obj disassembly also branches directly for 0x08000010.
Restore the same branch using retdec_squirrel_assign. No stage-clear bit, map
rule, DAT content or script coordinate is changed. Temporary diagnostics were
removed; the final runtime diff is only this string branch.

## Validation and handoff

After identifying the cause, the user requested no further agent game launches
and took over runtime verification. No post-fix game process was launched by
the agent.
Both build-runs/p3-save-worldmap-diag and p3-save-worldmap-notrace Release builds
complete and pass archive_smoke (1/1 each). stage_dat.ps1 verified and copied
the three DATs alongside both EXEs. Existing marisa*.dat files were preserved.

Test the updated runtime-builds/p3-save-worldmap-notrace/kinoko_retdec_rebuild.exe:
1. Enter a slot without progress. Routes controlled by unfinished stages should
   remain hidden, while the original initial route remains available.
2. Load an existing progress slot. Its legitimately unlocked routes should remain.
3. Complete a stage and return to the map. Only its corresponding new routes
   should unlock. Reenter the same save after restarting to check persistence.

The cause and code correction are grounded in original disassembly, upstream
source and pre-fix runtime state. The user subsequently confirmed the reported
issue is fixed. Coverage of existing saves and post-clear restart persistence
was not separately enumerated; no automated post-fix runtime pass is claimed.
The implementation and user confirmation are recorded in the same commit.
The original IDA annotation at 492505 was also
saved successfully to C:/rs-ida/fba00215-6kinoko.exe.i64.

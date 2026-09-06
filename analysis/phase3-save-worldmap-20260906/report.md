# Save deletion and title-to-world-map restoration

## Scope

Original: C:/WorkSpace/6kinoko/6kinoko.exe and its three packaged DATs.
Restore the original DAT scripts' save deletion and confirmed game entry.
Use isolated runtime copies of marisaA/B/C.dat for destructive test actions.
Preserve the existing runtime and reference saves and unrelated worktree edits.

## Work items

- [x] Read project instructions and reverse-engineering / IDA skills.
- [x] Survey original and record readable imports.
- [x] Decode original title/save scripts and identify native bindings.
- [x] Restore and verify save deletion and empty-slot display (user tested).
- [ ] Restore and verify confirmed entry to the world map.
- [ ] Verify diagnostic and no-log builds, staged DATs and earlier stages.
- [ ] Commit completed implementation checkpoints.

## E-triage / E-imports

IDA skill start.ps1/open.ps1, HTTP session d21b11e5. Original SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
PE32, base 0x400000, entry 0x4aca23, 3963 functions, normal text/idata/rdata/data
sections. Readable IAT: CreateFileA/W, ReadFile, WriteFile, SetFilePointer,
GetFileSize, CloseHandle, Direct3DCreate9, D3DX, WinMM, USER32, GDI32, IMM32,
COM, LoadLibraryW/GetProcAddress. Survey's SendMessageA network and
RegisterClassExA registry categories are heuristic misclassifications.
No packing evidence in this survey. Dynamic imports remain possible.

## E-script-flow

tools/inspect_cv4.py on analysis/titlemenu.cv4 and analysis/savedata.cv4:
AcceptDelete calls InitSaveDataTable on the selected table, sets savecount=-1,
then WriteCurrentSaveData increments it and invokes SaveTable(filename, table).
The original resets/writes a slot; it does not delete the file.
UpdateMenu waits for stageStart > 30, invokes WorldMap/PlayerImage/PlayerStatus
BeginStage(0), Fader2.FadeIn, title EndStage and RestoreStatus.
Native bindings: 473010 registers 472C90 LoadTable and 472E50 SaveTable via
471C10/471160. Original 472C90 inflates a length-prefixed zlib stream then
4722E0 restores typed values. 472E50 calls 472820 then 404390 to compress.

## Initial artifacts

Latest diagnostic runtime contains a zero-byte marisaA.dat. Reference A/B/C
are 1012/1010/668 bytes. These files are observations and are not overwritten.
The original's internal filename must be handled by its scripts, not inferred
from the external slot filename.

## E-save-restoration

Original 4A9C60 copies both words of key and value SQObjects (4A9CBE/4A9CC0,
4A9CF8/4A9CFA). RetDec only copied type and used a scalar for a two-word output.
Restore contiguous pairs, preserving sq_addref/sq_release ownership.
Original 4728E9 skips values whose type has no 0x7e bits, including null.
Restore this omission rule, recursive error propagation and iterator cleanup.
Bound streams by the actual inflated length / 0x20000 output capacity.

The decompiled zlib allocator calls were missing entirely in 473F30/476F40,
and 404390/404430 treated Z_STREAM_END as failure. Vendor the matching zlib
1.2.3 source and keep the original wrapper names and stream call sequence.
LoadTable now reports decoding/parsing failure instead of returning success
after ignoring an empty/uninitialized buffer. This lets LoadCurrentSaveData
execute the original InitSaveDataTable fallback. UpdateTop consequently sees
result.clearCount=0 and draws zero without a menu-specific override.
SaveTable restores its by-value object copy/destruction around serialization.

User confirmed both deletion and missing-file slots now work. Diagnostic
Release passes archive_smoke. Current next blockers observed at game entry:
WorldMap.Init missing chipCount, title UpdateMenu missing PlayerImage.SetFaceType.
Computer Use initialization/retry/reset all returned native pipe OS error 2;
existing PID-specific input/FFmpeg driver is used as the fallback.

Both diagnostic/no-log Release builds pass archive_smoke. No-log save checkpoint
launch used run_staged.ps1 with SHA256-checked DATs beside the EXE; its second
stage screenshot is raw/phase3-save-worldmap-20260906/notrace-save-smoke.png.
No logging/capture files were produced by that no-log runtime. Save checkpoint
is committed before the world-map work.

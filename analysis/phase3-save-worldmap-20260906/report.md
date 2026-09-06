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
- [x] Restore and verify confirmed entry to the world map.
- [x] Verify diagnostic and no-log builds, staged DATs and earlier stages.
- [x] Commit save checkpoint 79b080a and final world-map checkpoint.

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

## E-map-bindings

Original 433C90 registers C2DMapLayout and ChipLayout. The publisher only
recognized g299 (C2DLayout), omitting g327 map layouts. Restore separate native
pointer views and the map's chipCount/GetChipLayout/GetChipByPosition,
SetChipLayout/SetChipID/GetChipID/SetChipRect/PreArrangement operations.
435FC0/436110 use the 32-byte vector at +264/+268. ChipLayout exposes the
record's chipID, left/top, floating coordinates, layoutID, visibility and alpha.
435AF0 sorts by left then top; 435860 rebuilds map extents. The reconstructed
MCD representation resolves chip data directly instead of duplicating the
original's lookup caches. Query ordering and MCD rectangle semantics follow
435720/435220; 435FD0 edits the shared MCD source rectangle.

42F350 registers CActResourceChip and ChipInfo; publishing map resources as
CActResource2D omitted GetChipInfo and the actual flag/rectangle fields.
Restore the correct resource class and typed pointer properties. IDA
432320/432400/4324E0 establish integer, short and bool widths/offsets.
4341F0:434351/434360 also publishes layer.alpha/blend pointers to map layout
+320/+328. Without those aliases UpdateAreaEffect attempted null arithmetic.

450350:4507FE..4508EE publishes CActPlayer staging, margin, offset, visibility,
resolution, size and name descriptors. Most are pointers at +108..+148, not
mirrored values. Restore these descriptors, including offsetX/Y at +124/+128.
The pointers were already initialized by BeginStage. This fixes InitEvent's
scroll origin writes without changing script coordinates.

4522F0 updates layouts through vtable +28. Restore that call for map layouts;
434B40 supplies the original unlimited view bounds. 434F40's lost ECX caused
the first map draw failure; restore its thiscall entry into the existing map
renderer. Keep C2DLayout updates routed through the established faithful
implementation. Map draw/update obey per-layer and per-chip visibility.

## E-inline-source / E-portrait

PlayerImage.ACT includes a complete CFaceInfo class, OnCreate/Init/Update and
SetFaceType definitions. Original 415FD0:416268 compiles inline source. The
rebuild previously scanned text for CompileFile and ignored other source.
Use ../squirrel-2.2.2/SQUIRREL2's compiler in an isolated C++ VM, transfer its
closure stream, and execute through the existing rebuilt VM. No C++ objects
cross the boundary. Existing packaged CV4 files and original scripts remain
unchanged; the experimental C++ Execute backend remains OFF.

PlayerImage.Update eventually deletes nextFace[idx]. Original 493CD0 and
sqvm.cpp::DeleteSlot retain a full SQObjectPtr while removing the table entry,
assign both words to the result, then release the temporary. The decompiled
body retained only the type and incremented memory at address 4. Restore the
explicit VM receiver, metamethod path and ownership sequence.

The remaining portrait mismatch was an empty AssociateResource binding.
Original 424210 -> 4252E0 -> 41EF20 assigns layer+100=resource and
layer+96=resource.resourceID. Restore these assignments and return the original
integer result. Existing layout update reads the newly selected resource.
The original inline script selects face_6 for the tested save; diagnostic
logs confirm that same resource selection. User confirmed the portrait.

## E-clear-render-layer

The map's first Update calls ClearRenderLayer. The failure stack in
map-entry-4.log resolves to 46A1D0 via 470EE0 and the script VM. The list clear
passed a global placeholder to delete instead of each allocated node. Its
operator-delete wrapper (4AB2A6) independently ignored its argument and freed
that same global. Restore both argument paths, matching original 46A1D0 and
4AB2A6. The world map then updates without this invalid free.

IDA session d21b11e5 expired during the task. Reopened through skill open.ps1
as cfd01ef2, rechecked the same binary hash, and verified the final fixes there.
Final comment/save requests found that worker expired too; no annotated IDB is
claimed. The addresses, source comparisons and runtime artifacts remain here.
tools/x64dbg_query.py exposes the debugger's dynamically available tools and
reads its local connector configuration without printing authentication data.
One debugger trial used a breakpoint inside a call displacement; that trial
was discarded and its process stopped. No conclusions rely on that trial.
ASan was also unsuitable for this build: its global redzones break RetDec's
pre-existing adjacent-global layout assumptions. The useful failure evidence
came from the ordinary build's vectored exception frames and link map. Frame
logs now retain the existing diagnostic prefix so the trace filter keeps them.

## E-cropping / E-window-size

PlayerStatus.UpdateMap obtains its item image, computes source coordinates
as src_width/2-16 and src_height/2-16, then invokes BitBlt for a 32x32 rectangle.
48A7D0 (sq_getinteger) must truncate the actual float in the VM slot. RetDec
called an x87 conversion stub without loading the slot value. Restore the
conversion and explicit stack receiver. Original IDA, sqapi.cpp::sq_getinteger
and freshly built sqapi.obj disassembly agree (cvttss2si [slot+4]). User
confirmed the carrot cropping after this fix. No icon-specific offsets exist.

Original PE subsystem version is 5.01; the modern linker default was 6.00.
Both WinMain implementations make identical GetSystemMetrics calls, but Windows
applies different non-client frame compatibility: original client=640x480,
rebuild=630x470. A copied EXE changed only with editbin /SUBSYSTEM:WINDOWS,5.01
returned client=640x480 (subsystem-window-2.png and its diagnostic trace).
Restore that linker setting in CMake. Do not resize layers or edit HUD scripts.
User confirmed the bottom status bar after this change.

## Final validation and boundary

Final Win32 Release builds: p3-save-worldmap-diag and p3-save-worldmap-notrace.
Both compile and pass archive_smoke (1/1 each). stage_dat.ps1 checks all three
DAT hashes beside each EXE, and run_staged.ps1 reports WORKING_DIRECTORY unset.
No --data-dir or KINOKO_DATA_DIR override was used. Source compilation is
linked statically; no new runtime DLL is required.

User tested deletion, absent-file zero counts, map entry and movement, portrait,
carrot, status bar and map BGM transitions. The final no-log run independently records second-stage
playback (notrace-final-opening.mkv/.png), then the world map with the same
save as original-map.png (notrace-final-title.png is named after the planned
capture but actually contains the map). notrace-final-map-move.png confirms
the map remains responsive after input. No .log/.bmp/.dmp files were emitted
in that no-log runtime. It was left open on the map for the user.

Reference saves changed during the user's own testing (C is now absent);
the initial independent original-test runtime retains its original copies.
No attempt was made to undo the user's save/config edits or unrelated git edits.

Existing issues outside this increment remain: block.nut startup initialization,
some in-stage inline script paths, and recursive shutdown cleanup. These were
observed before or after the requested title-to-map workflow; this report does
not claim complete playable stage logic or a clean shutdown. Runtime screenshots
are verification artifacts, not sources for fabricated gameplay or coordinates.

Final EXE SHA256:
- Diagnostic: F61B7AB7C631D1F837181200833966B3B603D7CD3CCE9BCEC53CF7BCF26576E1
- No-log: 53F6DDFE1DE660D6A937E0D8DE6EBF47736FB04FE72490EC146F73E3D6B79936

Synthesis: static addresses/source and runtime evidence independently support
the completed behavior. Scope ends at map entry/navigation; later stage and
shutdown issues are explicitly retained. No unrelated threat/IOC conclusions.

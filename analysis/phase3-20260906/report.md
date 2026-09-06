# Phase 3 title and save selection restoration

## Scope and work items

Original: C:/WorkSpace/6kinoko/6kinoko.exe and opening.mkv.
Restore original DAT-driven title animation and save selection; preserve the
completed opening stages. No replacement title scripts or animation constants.

- [x] Read project, reverse-engineering, IDA and computer-use skills.
- [x] Survey original executable and readable imports.
- [x] Identify the first title script failure and compare native bindings.
- [ ] Restore title animation and save selection behavior.
- [ ] Verify diagnostic and no-log Win32 Release builds with staged DATs.
- [ ] Record final evidence and commit a backup.

## E-imports / triage

IDA HTTP MCP session 45ad8981 (skill start.ps1/open.ps1). Registered transport
does not see the HTTP session; tools/ida_query.ps1 reaches it successfully.
SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
32-bit PE, base 0x400000, entry 0x4aca23, 3963 functions and normal text,
idata, rdata and data sections. No packing evidence in this survey.
Readable imports include CreateFileA/W, ReadFile, WriteFile, GetFileSize,
SetFilePointer, Direct3DCreate9, D3DX transforms, WinMM timing, GDI, USER32,
IMM32 and COM. No direct network or crypto imports in this view; LoadLibraryW
and GetProcAddress allow dynamic resolution, so this is not an absence claim.

## E-property-registration

Existing p3-title-diag trace: TitleLogo.Init fails at COMPARITH on
layer.dst_y -= 160 with null/integer operands. UpdateFall subsequently fails.
Original TitleLogo.cv4 decoded with tools/inspect_cv4.py establishes the
receiver, property and opcode without guessing from animation appearance.

The reconstructed retdec_sqrat_set_offset_closure wrote both getter and
setter to the same table/key, overwriting the getter. Both call sites did
this, leaving __getTable populated by setters returning no value.
Original IDA 422A60 (integer) and 422D00 (float) use 51BEA8/51BEAC for
getters, 51BEA0/51BEA4 for setters. Register one callback per table.
The same helper serves C2DLayout and CActLayer, so both must be corrected.

Squirrel 2.2.2 sqvm.cpp:423 DerefInc and opcode COMPARITH agree with the
recovered original 494BF0. The null originates in native property lookup.

Computer Use initialization/retry/reset all returned native pipe unavailable
(OS error 2). Use render captures/FFmpeg and x32dbg for runtime evidence.

## E-origin and rendering

Original 41F580:41F5DA copies dst_x/y/z (+144) to ox/oy/oz (+156) before
publishing a layer. Restored the missing copy. This fixes the cursor that
was visible at the top of the title instead of beside a save slot.

450950:450BB0 enumerates all resources before registering layers. The
rebuild published an empty resource table, so DrawNumber could not find
font_number. Publish native resource instances using the existing binding
helpers, including atlases not attached to a visible layer.

Recovered BitBlt: 4555A0 argument adapter -> 4514A0 36-byte command queue
-> 4522F0 preparation of 184-byte entries containing a CSprite at +36
-> 4525D0 draws after the ACT layers. Restored vector size updates, sprite
UV/color initialization and the SetFVF/DrawPrimitiveUP calls. Original
45265B saves/clamps texture addressing; restore it to stop wrapping the
delete-menu texture. Original 415FD0:4160DC installs the six BLEND_* constants
in every script environment; the missing constants stopped UpdateConfig.

DrawNumber also executes COMPARITHL. Restored its actual VM-stack operands
and LOCAL_INC's SQObjectPtr assignment, matching 494710 and sqvm.cpp:407.
Squirrel source-object disassembly is saved as raw/phase3-20260906/squirrel-sqvm.asm.
DerefInc retains contiguous receiver/key pairs and passes their addresses to
Get/ARITH_OP, agreeing with the original and the existing repaired VM.

## Runtime evidence at rendering checkpoint

Artifacts: ../evidence/raw/phase3-20260906/ (ignored binary recordings).
diag-full-opening.mkv shows both earlier opening stages, automatic title
handoff, logo fall/squash/recovery, then the menu. diag-title-motion.png samples
that animation at 10 Hz. diag-slot-b.png and diag-slot-c.png record successful
cursor movement. User confirmed the cursor and subsequent submenu appearance.
Diagnostic title idle no longer grows the VM stack or raises title/font errors.
The pre-existing block.nut startup error remains for later investigation.

x32dbg attached to PID 38244, image base 0x5A0000. Break at rebuilt 4514A0
(RVA 0x47060) confirmed ECX=0x080D5210 and arguments x=319, y=217,
width=16, height=20, resource=0x0CCC40D8, source=(0,0), blend=1, alpha=1.0.
After detach this test process was unresponsive and was terminated; a fresh
ordinary run completed full playback. No claim that debugger detach is fixed.

Both Release builds compile and pass archive_smoke (1/1). All three DATs
were staged and SHA256 checked beside each EXE. Final no-log playback and
key-mapping persistence remain pending at this checkpoint. An automated
submenu recording stopped when the window lost foreground focus; the test
driver refused to inject keys into another application.

## Follow-up scope from user

Implement keyboard mapping next. Existing reference keyconfig.dat is 68 bytes
(17 little-endian words); inspect and verify against original 46B880/46B7C0.
The supplied marisaA/B/C.dat are 1012/1010/668 bytes, with a size prefix and
zlib-looking payload. User explicitly deferred difficult save-progress work.
Keep original files intact; use independent runtime copies for persistence tests.

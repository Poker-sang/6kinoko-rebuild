# Input devices and frame timer

Original reference: `C:/WorkSpace/6kinoko/6kinoko.exe`, SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
IDA MCP database `92f44055`; decompilations and input assembly are saved here.
The existing `../application-lifecycle/40dbe0.json` supplies keyboard/mouse
polling and the timer registry's try-lock-before-wait evidence.

## Restored behavior

- 408930 creates DirectInput with CLSCTX_ALL and calls Initialize(instance, 0x800).
  The reconstruction had omitted Initialize. Device setup now ignores the initial
  Acquire result as the original does, retaining devices for reacquisition.
- 408BF0/408EB0 enumerate attached game controllers in enumeration order, create
  DIJOYSTATE devices with foreground/exclusive cooperation, retain capability
  button counts (DIDEVCAPS + 16), and enumerate axes. 409030 applies the original -1000..1000 range
  by object ID; a failed range stops axis enumeration. Device creation failure
  stops controller enumeration. Format/cooperation/capability results are ignored
  by the original controller callback and remain so. No new mappings or POV logic.
- 408C80 polls/reacquires controllers then fetches 80-byte states without clearing
  on failure. 40DCDC then reads keyboard (clear on failure) and mouse (no clear).
  The old reconstruction discarded its mouse state; it now publishes a named
  20-byte cache. Controller count retains the original low-byte conversion.
- 408E1B sets mouse DIPROP_AXISMODE (property ID 2) to DIPROPAXISMODE_REL.
  The old reconstruction used DIPROP_BUFFERSIZE (ID 1) with the same value;
  R2 corrects this property and the capability field after SDK/assembly review.
- Existing mapping preserves +/-500 thresholds, six normalized axes, twelve
  configured buttons, and the saved controller record's broadcast semantics.
  Consumers use a typed snapshot and pointer instead of integer pointer math.
- 412890/412AD0 keep the 16ms timer, optional one-shot extra delay, auto-reset
  events and insertion-order signals. 412B80/412C10 preserve insert-before-create
  and signal/close-before-erase. The application now uses real HANDLE ports and
  restores 40DECD's try-lock/registration check before waiting.

## Ownership and explicit compatibility differences

COM devices and containers now have native owners. Failure paths release partial
input initialization rather than leaving a half-initialized COM object. Shutdown
unacquires/releases every controller before releasing DirectInput. The existing
foreground keyboard fallback remains only when no keyboard device exists; it is
reconstruction compatibility behavior, not newly attributed to the original.

Timer shutdown uses an atomic stop flag, joins the worker before closing registered
events, and balances successful timeBeginPeriod with timeEndPeriod (the original
lacks the balancing call). A missing delay event uses Sleep rather than a busy
loop; thread creation failure returns no registration so the existing application
16ms fallback can run. Registration remains a borrow, never independently closed
by the application. Initialization and CRT shutdown order are preserved.

## Validation

The existing physical-input/configuration contracts now use the typed cache and
cover invalid controller indices as well as thresholds, normalization, releases,
configuration broadcast and excluded keyboard keys. These are compile/link checks
only for this batch. No game, CTest or contract executable is run, per user request.

## R1 compilation and R2 review correction

R1 source `6856e5c` compiled and linked all targets successfully; no executable
was run. Its game EXE and verified three DATs remain in
`runtime-builds/input-timer-r1-quiet` (EXE SHA256
`8A45D69AFE98D048BE42BFCF6833BE131009A342DA7992ADF698F6325A7D51A2`).
Final SDK/assembly comparison found two constants/fields to correct in R2:
mouse property ID 2 means relative axis mode, and DIDEVCAPS offset 16 means
button count. R2 also removes the two unused split globals now covered by
native capability storage and the published mouse snapshot. Raw GUID/data-format
evidence in `device-formats.json` confirms DirectInput8A and the 80/256/20-byte
controller/keyboard/mouse formats.

R2 source `7218c62` also compiled/linked all targets and its DATs were staged.
Compiler review flagged the C ABI timer initializer's explicit bad_alloc throw
under /EHc. R3 declares `noexcept(false)` on that boundary (as already done for
the game math boundary), preserving the failure path without the false nothrow
assumption. There is no gameplay change from R2. All R1/R2 artifacts remain.

## R3 handoff

- Source commit: e23feca91cbde5827a69f1ea277631bc4d53f474.
- Win32 Release, logging disabled, build tree: build-runs/input-timer-r3-quiet.
- All targets compiled and linked successfully (exit 0), including the updated
  input/configuration and application contract sources. No game or local test
  executable was run. Existing unrelated compiler warnings remain; the new
  timer initializer warning is resolved.
- EXE: runtime-builds/input-timer-r3-quiet/kinoko_retdec_rebuild.exe.
- EXE SHA256: ED25ED8693EB23CFA19BDCD2B5B37A8FB4D2B778A8974927BDAC243DF4D66AE2.
- stage_dat.ps1 copied the three required DAT files beside the EXE and verified
  size and SHA256 successfully. All prior build/runtime artifacts are retained.
- Hardware/controller behavior and gameplay are not runtime-verified in this
  batch; controller support follows the original attached DirectInput devices,
  with no added XInput mapping or automatic hot-plug behavior.

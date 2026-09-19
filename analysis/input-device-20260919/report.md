# Physical input migration

Tested implementation: 181b23cc76cf5693c19a41cbf3a23669846262e9.

Ten address entries moved to physical_input.cpp and direct_input.cpp:
407500, 408320, 4083E0, 408930, 4089C0, 408B30, 408C80,
408D00, 408E60, 408E80.

The 168-byte physical input layout now has named fields and static layout
assertions. Direction/button updates share explicit helpers. Original x86
counter wrapping is retained without signed-overflow undefined behavior.
Original IDA evidence confirms signed unassigned button checks and unsigned
keyboard scan bytes; the old RetDec C did not preserve both correctly.
Negative-direction priority, strict +/-500 thresholds, positive-count button
release, unchanged unassigned slots, low-byte modifiers and double-to-float
axis scaling are retained. No speculative bounds checks were added.

DirectInput uses native C++ COM methods. Existing trace sites, publication
order, initialization/failure cleanup, fallback keyboard polling and mouse
polling behavior are preserved. This is not a claim to restore controller
enumeration: 408BF0 remains the existing stub. Hardware COM failure paths have
not received a new mocked-device test in this batch.

Metrics relative to 8ef32c7:
- Main C: 89,841 -> 89,267 lines (-574).
- Address definitions moved: 10; remaining main-C definitions: 1,298.
- New C++ runtime lines: 295; net runtime reduction: 279.

Validation:
- Win32 Release quiet and diagnostic builds succeeded.
- Both complete CTest suites passed 46/46.
- Added physical input contracts: high scans, unassigned fields, simultaneous
  opposite keys, release pulse, signed overflow, modifier requirements,
  joystick deadzone endpoints, axis scaling, disabled-device clearing.
- Migration boundary check passed; original decompiler reference unchanged.
- Three DAT files staged beside each executable; sizes and SHA256 verified.
- Unique build/runtime names: input-181b23c-{quiet,diag}-20260919.
- source-commit.txt records the tested implementation in both runtime dirs.
- No graphical smoke test: user's modules-c4270ea game was running (PID 37660)
  and was left untouched as requested. Startup/first-level behavior of these
  two new executables is therefore not claimed verified.

Evidence was recovered using IDA MCP database kinoko-device-auto-20260919.
Only original-valid-*.json files below are successful decompilation evidence;
earlier failed session artifacts remain local and are not evidence.
The large 4175F0-like group was investigated but not migrated: the body
contains actual tree lookup, allocation and reference-count operations with
lost receivers, not merely dispensable jump stubs. Resolving that ABI chain
is still required before consolidating it safely.

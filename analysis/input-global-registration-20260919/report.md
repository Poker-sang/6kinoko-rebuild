# Input and global script registration

Tested commit: 1a9a54f42d239b989c532b571348640b994af2fa.

Moved original 46D950 (Input registration), its 46CFB0 class builder, and
473010 (global script initialization) into readable C++ registration modules.
Input retains five methods and 31 fields, including shared b/k and br/kr
storage and the original s1..s9,s0 registration order. Its ClassType descriptor
now has explicit contiguous storage, matching the original 24-byte record.
Global initialization retains 32 native functions, update/render mask bindings,
six constants, class registration order, class_def.nut loading, reference
ownership and diagnostic calls. Existing native ABI adapters are unchanged.
No new runtime boundary handling was added.

Original evidence: IDA MCP decompilations of all three entries; raw original
Input names; assembly push operands for all 32 global native wrappers.
All 31 Input names/types/offsets/flags and all 30 direct global registration
names/order were compared programmatically. All 32 native wrapper addresses
match the original assembly (419E60 retains the existing CompileFile adapter).

Main C: 93,850 -> 92,765 lines (-1,085).
Address-named definitions: 1,330 -> 1,327.
New runtime C++/headers: 304 lines. Net runtime reduction: 781 lines.

Validation:

- Both Win32 Release builds passed all 46 CTests.
- Extended the existing stage contract with repeated global registration,
  native closure lookup, Sleep/timeGetTime dispatch, stack balance, Input
  int/bool alias reads/writes, native GetAssign receiver, and adjacent storage.
- Migration boundary check passed.
- Three DAT files staged beside each main EXE, with size/SHA256 verification.
- Build/run directories: scriptbind-1a9a54f-{quiet,diagnostic}-20260919.
- Maps, logs, source-commit.txt and all prior artifacts retained.
- Initial d72e686 build succeeded; its new test script needed a newline after
  an unbraced Squirrel if statement. The test-only fix passed both full suites.
- No graphical smoke test: the user was running actorbind-e2673d6 (PID 8776
  when checked); that game was left untouched.

This reduces game binding boilerplate; it does not change Squirrel VM coverage
or establish that any remaining unmapped entry is unnecessary.

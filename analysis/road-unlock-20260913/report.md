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
- [ ] Recover faulting script/slot and identify original behavior.
- [ ] Implement a source-grounded C++ correction and regression coverage.
- [ ] Commit before testing; build separate diagnostic and quiet candidates.
- [ ] Verify contracts and bounded gameplay smoke; retain all artifacts.

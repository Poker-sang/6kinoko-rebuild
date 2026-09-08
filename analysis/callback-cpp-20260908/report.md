# Script callback C++ migration

Scope: Actor/Camera callback binding only. Preserve original DAT loading,
script decisions and existing VM ownership helpers. Reference inputs are
read-only; carry forward ../cpp-cleanup-20260907/report.md and
../act-cpp-20260908/report.md. All validation artifacts must be retained,
with a source commit before testing and one bounded smoke per variant.

## E-triage / E-imports

Read ida-reverse and reverse-engineering skills, precedent-reverse, tool-index,
re-agent-workflow, analysis-decision-framework and repository AGENTS.md.
Skill start.ps1/open.ps1 opened a temporary original copy as aae389b7.
IDA MCP survey confirms PE32, base 0x400000, 3963 functions, four normal
sections and SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
The imports page confirms CreateMutexA, GetModuleFileNameA,
SetCurrentDirectoryA, Sleep and CreateFileW. Full import classification is
carried forward from cpp-cleanup E-imports: file operations, Windows UI,
graphics, audio, input, COM and dynamic library loading. No packing obstacle
was observed. This is a game reconstruction, not a malware investigation.

## Work Items

- [x] Read scope, previous evidence and current callback implementations.
- [x] Confirm original layouts, calling conventions and call sites in IDA.
- [x] Migrate callback binding into C++ and add focused ownership/ABI checks.
- [ ] Commit source before fresh quiet/diagnostic builds and validation.
- [ ] Record tests, bounded smoke outcomes and artifact provenance.

## E-callback / Static Evidence

IDA analyze_batch disassembly and xrefs confirm:

- 45FCD0: Actor.SetUpdateFunction, registered at 460EF4. The environment is
  Actor+44, and the callback (VM, environment, function) occupies Actor+92.
- 45FD80: Actor.SetCollisionCallbackFunction, registered at 460F45. Only
  0x08000100 (OT_CLOSURE) binds the supplied closure to Actor+44. Other types
  use 45DF10(NULL) to construct the empty callback at Actor+120.
- 4663C0: Camera.SetUpdateFunction, registered at 466A5B. Its environment is
  Camera+0 and callback is Camera+12. All three setters consume a 12-byte
  by-value SquirrelObject, release locals before the input and end in ret 12.
- 466470: Camera.Update called from 469900 at 469970. Dispatches 45DFB0 only
  for OT_CLOSURE; other types return unchanged. Native closures are excluded.

Comments were saved through IDA MCP to C:/rs-ida/d8410a1c-6kinoko.exe.i64.
The initial analyze_batch/decompile requests used incompatible argument
shapes and were rejected; the corrected analyze_batch request succeeded.

The 12-byte SqPlus SquirrelObject is vtable/type/value, not SQObjectPtr.
Supplied ../squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp sq_addref/sq_release
confirm external RefTable ownership with GC enabled. Also consulted supplied
source-build squirrel-sqvm.asm CallNative +0x325..+0x385, which restores the VM
frame before raising the last error. No VM implementation change is made.

The C++ module names the object/callback fields, asserts their sizes/offsets,
and uses noncopyable local owners to preserve normal-path release order.
Existing copy/assign/release, empty-function creation and invocation helpers
remain C code, now externally linked. Existing VM trace call sites remain.
The previous null-camera guard and collision fallback's two zero-temporary
cleanup calls are preserved; these are compatibility details, not newly
claimed original behavior. Four handwritten assembly entry bridges are removed.

## Pre-Test Checkpoint

Commit before configuring/building/running tests. Use new directories
build-runs/callback-cpp-20260908-{quiet,diag} and
runtime-builds/callback-cpp-20260908-{quiet,diag}. The runtime manifests will
record the source commit, EXE hashes and DAT checks. Preserve all artifacts.

New focused contracts call all three setters through the actual thiscall
adapters, alternate/self-assign closures 64 times, inspect the external
RefTable counts, check stack balance, invoke the Camera closure, and verify
non-closure/empty handling. Existing script registration tests remain.

## First Validation Attempt

Source 98ff6ddf61380f2704685161baaae221fa3dbfe5 built successfully in both
new directories. DAT staging passed. CTest passed archive/ABI but the new
callback contract stopped at the empty collision callback assertion. All
64 binding/self-assignment iterations and Camera dispatch checks had passed.
The test assumed canonical OT_NULL, while the unchanged 45DF10 compatibility
helper copies a zero-initialized temporary into its empty callable. The C++
migration preserves this existing result. Correct the test to compare with
that constructor's result; do not change runtime logic to satisfy the test.
No game was launched for this attempt. All files are retained. Revised tests
use fresh callback-cpp-20260908-r2-{quiet,diag} build/runtime directories.

# Actor state C++ migration

Scope: continue the readability migration after f696509. Preserve original
DAT loading and scripted behavior; reference files are read-only inputs.
Read ida-reverse, reverse-engineering, precedent-reverse, tool-index,
re-agent-workflow, analysis-decision-framework and repository AGENTS.md.

## E-triage / E-imports

Skill start.ps1/open.ps1 opened original session a565c4f1. IDA MCP survey
confirms PE32, base 0x400000, 3963 functions and four normal sections.
SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Imports include CreateMutexA, GetModuleFileNameA, SetCurrentDirectoryA,
Sleep and CreateFileW. Carry forward full E-imports classification from
../cpp-cleanup-20260907/report.md: file I/O, UI, graphics, audio, input,
COM and dynamic loading. No packing obstacle was observed.

## Work Items

- [x] Read current source, prior evidence and repository constraints.
- [x] Verify Actor state methods against original disassembly and call sites.
- [x] Move verified bodies into C++ and cover behavior/ABI with contracts.
- [x] Commit source, build both variants in new directories and run contracts.
- [x] Stage all DATs and record bounded smoke/user acceptance with provenance.

Each gameplay check ends after entering stage one, attempting a jump and
seeing an enemy. Retain every artifact and failed attempt. Existing gameplay
access violations are not automatically migration regressions.

## E-state / Original Evidence

IDA analyze_batch confirms 45DBC0 (registered at 460E55) writes one byte to
Actor+22 and manager+120. It marks deferred release, not immediate destruction.
45DE70, called from 463B40 at 463B9F, copies 48 bytes to Actor+376 with rep
movsd, ECX=this and ret 4. The migrated C interface preserves the rebuild's
explicit receiver, null guards and return value.

45FE80 is SyncAnimation (registered at 46126C). It consumes a 12-byte SqPlus
SquirrelObject with ret 12, checks type 0x0a008000 and Actor+200 before calling
4A9B40. It copies source Actor+212/+216, upper-clamps the frame against the
destination vector's signed byte span /248, and calls 462250 to write the
selected frame to Actor+204 and Actor+152. Negative source frames are not
unconditionally clamped. C++ preserves these branches and uses unsigned
32-bit arithmetic for pointer span/offset wrap. The legacy 462250 body is
outside this batch; the migrated path retains its already-inlined behavior.

Original comments were saved in C:/rs-ida/3441ceee-6kinoko.exe.i64. Supplied
Squirrel sqapi.cpp sq_addref/sq_release and source-build squirrel-sqvm.asm
CallNative +0x36A..+0x385 were consulted. SqPlus external ownership still uses
the existing C VM helpers; this batch does not switch the execution backend.

Three implementations move to actor_methods.cpp; two naked bridges are
removed. The existing trace call and helper remain. Contracts verify exact
copy/write spans, repeated deferred release, valid/high/negative frame indices,
empty/missing animation, non-instance input, self-sync, script registration,
VM stack balance and external reference ownership using real Actor instances.

## Pre-Test Checkpoint

Commit these sources before configuring/building/testing. New directories:
build-runs/actor-state-cpp-20260912-{quiet,diag} and
runtime-builds/actor-state-cpp-20260912-{quiet,diag}. Preserve all artifacts and
record the source commit in each runtime validation manifest. README files are
unchanged per the user's updated preference. Future batches may cover complete
modules/call chains, with evidence and validation determining the scope.

## Validation Result

Source checkpoint b284f1be8a792fa1093ec7d6341221db4f9c02a9 was committed before
both Win32 Release builds. Both pass CTest 3/3, including the new real-instance
SyncAnimation/script/RefTable tests. DAT staging passes size and SHA256 checks.
Build logs, CTest logs, runtime manifests, EXEs and screenshots are retained.

Quiet gameplay entered stage one, received a Z jump input and a bounded
three-second Right hold, and showed the first enemy in smoke-tool-04.png.
The next game action closed the process; smoke-close.log confirms closed=true
and alive/responding before closure. Computer Use initially produced brief
keys and IME composition without progressing the menu, so the existing
focus-guarded game tool provided DirectInput-compatible key durations. Its
captures and the earlier Computer Use captures are all retained.

Diagnostic gameplay reached the Stage 1 world-map node in smoke-03.png and
was alive/responding. The user then confirmed gameplay had no problem and
asked to continue. Do not claim the diagnostic automated jump/enemy sequence
completed: this variant is accepted based on user feedback. At resumption no
game process remained. No repeated gameplay or old-crash investigation followed.

EXE SHA256:
- quiet: 93CB4CF249AA05234B65A74CD9073DFF857B6170AFDCE9EF31072A888C026618
- diagnostic: 59B76404D5C45AA6BCEA4B6F7F7CC7EBD147E852791998DDF134138A255B24EE

ADF P0: static original evidence and executable contracts support the migrated
behavior; gameplay evidence is bounded as above. Remaining generated code and
existing compatibility guards limit any broader equivalence claim. No README
changes or original binary resources are included in the source commits.

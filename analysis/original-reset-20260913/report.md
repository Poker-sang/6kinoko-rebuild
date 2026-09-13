# Restore original Actor cleanup receiver

## Scope and authoritative evidence

The user explicitly rejected the defensive callback-identity guard and requested
original behavior, with no invented gameplay/exception rules. That guard was
removed in 609efb5. Normal failure retirement is unconditional again.

Skills: ida-reverse, reverse-engineering, their already-read precedent/tool-index
and workflow. Original opened with skill scripts, IDA MCP session 1155f095 via
tools/ida_query.ps1. Survey confirms original SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155, PE32,
3963 functions. Carry forward readable Windows file/graphics/input/COM imports
from analysis/evidence/E-triage.md. Supplied Squirrel 2.2.2 sqclass.h Get/Set,
sqvm.cpp and source-built analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm
remain the library reference; no VM backend substitution is used.

## Root cause

Original 45FB90 clears native callbacks, then calls SquirrelObject rawset twice.
45FC7A: mov ecx, offset 513C58; call 4A97B0 at 45FC86.
45FC94: mov ecx, offset 513C58; call 4A97B0 at 45FC99.
513C58 is the global Actor CLASS SquirrelObject (g602). The keys are step and
user (g601/g600). Both calls reset CLASS DEFAULTS. They do not clear the old
instance's fields. Finally 45FCAD loads actor+44 and releases only the native
owner's instance reference. The running VM still owns its old instance.

The reconstruction incorrectly supplied actor+44 as the rawset receiver. This
cleared old instance.user while EnemyUpdateProc was still executing. Its final
hitBottomPrev assignment then failed and the original unconditional error path
retired the newly installed update. Old balls also failed reading parent.user.take.
Later death could keep vy=-5 forever because update no longer applied gravity.

The previous report's suggestion that this was an original script boundary
failure is withdrawn. It relied on pseudocode without recovering the receiver
at these two call sites. The old test that required oldReset.user/step to be null
encoded that same mistaken assumption and has been corrected.

## Direct original execution oracle

An optional debugger host starts the original executable, pauses before WinMain,
restores its one-byte breakpoint, suspends the main thread and loads the test DLL.
The DLL calls original Actor and Squirrel functions at their actual loaded image
base. No game window, rendering loop or gameplay input runs. The original
instructions in Reset, cleanup and VM are unchanged. Source probe bytecode is
compiled using supplied Squirrel 2.2.2 and executed by the ORIGINAL VM.

Oracle experiments (all build/run artifacts retained):
- 609efb5/r1, 2db3aed/r2: fixed-address mapping approach blocked by existing mappings.
- 7c0b75d/r3: debugger host works; probe needed actual ASLR base resolution.
- 9ba0c71/r4: probe reached Actor creation; missing tenth CreateActor argument
  corrupted the probe ABI. These two probe crashes are not original gameplay bugs.
- 10dd086/r5: corrected original CreateActor ret 28h ABI. Reset returned success;
  old instance.user remained accessible, new native instance was distinct.
- 72614f7/r6-diag: tests/original_actor_reset.nut runs 32 nested Reset calls.
  Old instances retain their user fields, new instances are distinct, initializer
  executes 33 times, original VM returns success and update remains a closure.

The oracle is opt-in through KINOKO_BUILD_ORIGINAL_ORACLE=ON. Its DLL and host
are development artifacts, not a replacement game runtime.

## Implementation and validation

Source 72614f7 ports 45FB90 into typed C++ kinoko_actor_clear_script. The rawset
receiver is g602 at both calls, matching original assembly. The now-unused naked
45FB90 bridge is removed. Callback failure behavior is original/unconditional;
there is no identity guard, expected-error masking, special enemy rule, velocity
clamp, DAT patch or automatic runtime error suppression.

Diagnostic r6 passed CTest 4/4 and the original DAT enemy contract with ZERO
script failures: four ball generations across offscreen resets, visible moving
balls, ordinary collision death and post-reset OnHitStep death under original
gravity. Reset contracts retain old user/step identity, verify native weak-owner
release and initial argument replay, and execute a Reset inside an active
callback followed by a successful write through its old user. A succeeding
frame executes the new callback. The separate original oracle passes 32 resets.

Every game EXE has size/SHA256-verified original DATs beside it. No new live
gameplay test is claimed; the user retains that role. The prior world-one
playthrough validated 33bff37, not this newly restored implementation.

Final r6-quiet also passed CTest 4/4 and the DAT enemy contract with zero
script failures. Both game executables have staged, hash-verified DATs and
validation manifests tied to 72614f7. Ordinary quiet play does not enable
script snapshots or exception dumps. Live gameplay remains user-owned.

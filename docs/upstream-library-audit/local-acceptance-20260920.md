# PR #7 local Windows acceptance

Tested source: `8f9756da2e9d5cbc1600672606439b356eea3956`, already committed
before this test batch. No production source was changed during acceptance.
The subsequent documentation commit does not change either tested executable.

## Build and contracts

Both builds used Visual Studio 18 2026, Win32, Release, trace filtering enabled,
normal BGM service and writes, and the original `C:/WorkSpace/6kinoko` reference
assets. Only `KINOKO_RETDEC_DISABLE_TRACE` differs (OFF diagnostic, ON quiet).

| Variant | Build tree | Runtime directory | Full CTest |
| --- | --- | --- | --- |
| diagnostic | `build-runs/pr7-local-20260920-r1-diagnostic` | `runtime-builds/pr7-local-20260920-r1-diagnostic` | 52/52 passed |
| quiet | `build-runs/pr7-local-20260920-r1-quiet` | `runtime-builds/pr7-local-20260920-r1-quiet` | 52/52 passed |

All paths are relative to `C:/WorkSpace/6kinoko-rebuild`. Each build retains
source-commit.txt, configure.log, build.log, ctest.log, the linker map, staging
and launch logs, and CTest's original results. Runtime directories retain the
EXE, source-commit.txt and all three DAT. Nothing from older batches was removed.
The source boundary checker and all five provenance checker tests passed;
199 retained upstream members and documented adaptations were verified.

`stage_dat.ps1` copied and SHA256-verified the three DAT beside each EXE.
`run_staged.ps1` launched that EXE directly with working directory unset.
No data-directory override, alternate staging EXE, index.dat or reference saves
were used. `local-artifacts.json` records hashes and sizes of the tested EXEs,
DAT, codec snapshots and supporting logs.

## User-assisted short gameplay

The user interacted with both windows during verification and reported
“没有问题继续” for diagnostic and “没问题继续” for quiet. Treat these as
user-assisted acceptance, not as an unattended input replay.

Diagnostic: the assistant observed the rendered title and first-stage game
screen, player and monster sprites. The window's input changed during the
assistant's observation, so its attempted key action was rejected by the UI
tool. The process subsequently ended and its preserved trace ends in
`game:exit`. Quiet: the assistant observed the Stage1 transition and rendered
first-stage screen. The user completed the interaction and closed the window;
the subsequent assistant key attempt had no live target. No trace file was
created in the quiet runtime directory.

The limited first-stage/jump/monster acceptance is therefore supported by the
user's confirmations and the observations above. The assistant did not
independently record a successful jump key injection or an exit code for either
process. No additional gameplay session was started to duplicate the user's
check. No exhaustive visual, audio-sample or function-sequence parity claim is
made, and the original EXE was not played in this batch.

## Original binary evidence and remaining scope

IDA MCP was started/opened with the ida-reverse skill's supplied scripts. The
HTTP tool bridge retained `ida-survey.json`, `ida-imports.json` and
`ida-4aa750.json` in the diagnostic build tree. The original SHA256 is
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
It is a 32-bit PE at 0x400000. The import evidence includes d3d9/d3dx9_33,
GDI32, IMM32, USER32, WINMM, ole32 and KERNEL32 (137 imports total): graphics,
input/windowing, timing, COM and file/process/runtime support. This is not an
exhaustive dynamic API inventory. The original declaring-base resolver
4AA750 was re-exported as assembly to preserve a local evidence anchor for
the already-implemented property receiver correction.

The supplied `../squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp:645` was also
checked: `sq_getinstanceup` returns the instance user pointer and checks the
class/base typetag chain. It does not itself choose SqPlus's separate `__ot`
base pointer. No VM or receiver code was changed in this acceptance batch;
no new Squirrel source-disassembly equivalence claim is made.

The five historical upstream integrations and their scoped acceptance are
ready for PR review. Referenced legacy CRT exception/RTTI placeholders, custom
game registration/native descriptor policies and the frame-copy operand
heuristic remain explicitly outside this PR's completed replacement claims.
They require their own recovery evidence rather than speculative cleanup.

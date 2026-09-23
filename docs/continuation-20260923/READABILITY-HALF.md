# Address-named body inventory, 2026-09-23

The unchanged `tools/audit_readability.py` lexical inventory counts address-named
function body lines in first-party C/C++ files literally listed by top-level
CMake. Baseline `30a2c57`: 156 address-named functions, 883 body lines.
After batch 59 (`0444f3b`): 123 address-named functions, 439 body lines.
The requested strict half threshold is 441 lines; the result is 49.7% of
baseline, a decrease of 444 lines (50.3%). This measure is lexical and is
not a proof of runtime equivalence or total reconstruction progress.

- Batch 56: 883 -> 765. IDA 457A10 child traversal has a named receiver-side
  implementation, and IDA 470D00/4721A0/472240 integer metadata binding now
  shares a named five-word record writer and delegate installer. Source
  commits `9ad4020`, `e6df44e`; successful quiet build source `e6df44e`.
- Batch 57: 765 -> 559. Removed 45 unreachable RetDec functions from the
  rebuilt C file after checking identifier uses in non-archival `src`,
  `include`, and `tests`. The exact list is in
  `removed-unreachable-r57.json`; the original archival decompilation is
  retained. This 206-line metric reduction is dead-code removal, **not**
  newly reconstructed runtime behavior. Source commit `e255e0a`.
- Batch 58: 559 -> 476. Eleven native Squirrel callbacks expose typed VM
  implementation helpers behind original address entrypoints, and six
  class-registration entries share a named Sqrat root lifetime routine.
  Source commit `155ae8b`.
- Batch 59: 476 -> 439. Input script initialization takes an explicit
  `KinokoInputManager*` in its implementation; formatted Squirrel VM errors
  use a typed helper with one `va_list` owner at the ABI boundary. Source
  commit `32b8dac`.

Each successful batch had its own independent Win32 Release build and
staged the three original DATs next to its EXE with size/SHA256 verification.
Batch 56's first independent build failed the isolated method-entry contract
link; the correction was committed and rebuilt in a different directory.
Build artifacts and logs, including this failed build, remain available.
The final executable is
`runtime-builds/readability-input-error-quiet/kinoko_retdec_rebuild.exe`,
SHA256 `EE217F7BE5E1776368F0A2B54B937BA8FF60820F1CF797186E0578285FBC7CC7`.
No game, CTest, or contract executable was run by the agent. Compilation of
contract targets is not a claim that their assertions passed. The original
unrelated modification to
`docs/decompiler-cleanup-r126/original-stage-evidence.json` remains untouched.

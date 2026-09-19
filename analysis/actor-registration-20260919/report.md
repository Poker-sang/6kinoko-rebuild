# Actor registration migration

Implementation tested: e2673d6 (parent implementation: 1ba04e3).

Original executable entry 460E00 registers 15 Actor methods and 50 properties.
The new `src/squirrel/actor_registration.cpp` expresses those registrations as
ordered tables. All 50 property names, types, offsets and flags were compared
against the original IDA MCP decompilation in original-460e00.json. Method
targets and wrappers are unchanged from the existing ABI-correct bindings.
Original isActive/isStatic byte alias, InterrputCollisionCallback spelling,
read-only take, global object references, OT_NULL defaults, and release order
are preserved. No new runtime boundary handling was added.

Main C: 94,541 -> 93,850 lines (-691); address-named definitions 1,331 -> 1,330.
New C++ implementation: 116 lines; net runtime source reduction: 575 lines.
This migrates game-side registration, not another portion of Squirrel's VM.
Input registration (46D950) and resource suspension/reload remain unchanged.

Validation:

- Migration boundary check passed.
- Release Win32 quiet and diagnostic builds passed, each with 46/46 CTests.
- Existing stage contracts exercise Actor default slots, callbacks, movement,
  collisions, animation and lifetime through the real scripting runtime.
- Three original DAT files staged beside both main EXEs with size/SHA256 checks.
- Build trees and run directories: actorbind-e2673d6-{quiet,diagnostic}-20260919.
- Each runtime directory includes source-commit.txt; logs and link maps retained.
- First build (1ba04e3) exposed a missing actor_animation.h include; corrected
  before the successful builds. Failed build artifacts are also retained.
- No graphical smoke test: user's old slim-f2da106 game was still running
  (observed PID 11540); it was not closed or otherwise operated.

The smaller code size is a maintenance improvement, not evidence that
unmapped original functions are unnecessary or that full migration is complete.

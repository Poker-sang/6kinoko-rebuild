# Opening and title BGM playback

## Scope and evidence

Continue the phase-two restoration without modifying DAT files or adding scene
timers. Binary identity/imports and Squirrel bytecode format are carried forward
from `../phase2-20260906/report.md`. Original IDA MCP database: `41353d21`.

## Restored behavior

- Original `470257` reads the fourth PlayBgm argument. The rebuilt `470220`
  incorrectly forwarded argument three (`100` in op.nut) as the loop flag.
  The original op.cv4 call passes `false` as argument four. Restored that
  argument, so op.ogg is no longer restarted at EOF.
- Original `412203` reads the SFL sidecar regardless of the fallback loop flag.
  Restored that independent loading step. op.ogg has no sidecar; title.sfl has
  loop start `0x3DA5E` and end `0x229D6C` (sample frames). Title therefore loops
  even though its PlayBgm call also passes false.
- Original `4124D6` reads cue position at cue+16; `412514` requires a nonzero
  start and length. The reconstructed reader now follows those fields/checks.
- Original `412240` decodes at most 4096 PCM bytes per read and preserves the
  amount past the loop end when seeking back. The replacement Vorbis path now
  tracks delivered samples and uses exact sample seeking, rather than using
  decoder read-ahead or seeking to an earlier compressed frame boundary.
- Original `409A11` stops playback on the update after a short decoder read.
  Restored that EOF transition instead of waiting for a software pending-byte
  count while a looping DirectSound ring can expose old samples.
- Original `40A4A8` releases active/pending/retired audio buffers after joining
  worker threads. Restored release at manager shutdown, rather than deferring
  it to CRT destruction.

No script, door event, animation frame, BGM filename special case, arbitrary
delay, or forced fade was added.

## Verification

- Diagnostic and no-log Release builds succeeded and the three DAT files were
  staged beside each EXE with SHA256 verification.
- The diagnostic archive smoke test passed (1/1).
- Final diagnostic trace shows op.ogg looping=0, source-ended -> stop-at-eof ->
  release, followed later by title.ogg looping=1 and the original SFL markers.
- The user confirmed that opening BGM no longer repeats, the in-game transient
  sound is fixed, and title BGM loops again.
- WASAPI loopback captures and extracted original music/sidecar are under
  `analysis/evidence/raw/phase2-bgm-20260906/`. `tools/capture_loopback.py` uses
  PyAudioWPatch; its optional executable is launched through run_staged.ps1.
  Captured sample duration is not a reliable wall-clock reference when Windows
  supplies no output packets. The long title capture was interrupted by game
  closure, so it is not claimed as an independent completed-loop test.
- Latest no-log build was not independently replayed after the final EOF/SFL
  changes. That runtime output was subsequently absent during handoff; no
  unrelated workspace cleanup was reversed.

## Deferred exit defect

The close-window trace reaches `game:exit`, then raises `0xC00000FD` (stack
overflow) at rebuilt RVA `0x308A3`. The link map places this in
`function_429c70`, the resource tree destruction helper. Its generated recursive
calls pass `&g1224` instead of the actual right child. Original `429C89` recurses
with node[2], then deletes the current node and walks its left child.

This interrupts normal resource teardown before it can reach the audio shutdown
path, consistent with the reported pause and transient sound before process
termination. It is distinct from the now-fixed in-game EOF behavior.

The user explicitly requested committing the current fixes and deferring this
exit exception. No resource-tree cleanup changes were made.

## Checklist

- [x] Original executable/bytecode anchors reused and checked with IDA MCP.
- [x] BGM argument, sidecar, EOF and shutdown behavior restored from assembly.
- [x] Both configurations built and DAT staging verified.
- [x] User playback feedback and diagnostic results recorded without claiming
      uncompleted long-run verification.
- [x] Exit exception identified and deferred at the user's request.

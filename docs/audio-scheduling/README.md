# Audio scheduling and lifetime — R1

## Evidence and scope

Original `6kinoko.exe`, SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`,
IDA database `972a06cb`. JSON files retain original decompilation and the
40A950 assembly needed to disambiguate its register arguments and final flag.
Interior addresses in a few exploratory queries resolve to their enclosing
function; use the function name/start, not the requested address, as evidence.

This batch covers the live BGM worker/request/retirement chain, public playback
bindings, sound-pool cleanup and device shutdown. It is not a replacement of the
Vorbis codec or a claim that every standalone original audio API is exposed.
No Squirrel VM implementation or trace call is changed.

## Changes

- `AudioWorkers` owns both threads, wake events and their shared critical section.
  Partial startup and normal shutdown use one stop/join path. Thread handles are
  joined before closing events or releasing buffers; the final runtime guard
  performs resource shutdown before member/global destructors run.
- Named public playback/device/pool functions replace address-named exports and
  their live callers, including script registration and stage cleanup. Filename
  parameters are real `const char*`; worker startup returns a borrowed `HANDLE`.
  Master volume uses a real float argument. Unreferenced receiver-losing fake
  constructor/handle-allocation exports are removed; the live typed allocator
  remains. Script wrapper argument layout is preserved (Win32 pointer width).
- `40A950` sets a retire-after-fade flag; `40A9A0` clears it. Ordinary `FadeBgm`
  to zero now keeps the buffer, while a replacement fade retires the old track.
  `4098A0` immediate gain updates do not replace an existing fade envelope.
  Delayed starts clear their deadline on starting; fades wait for that start.
  Original unsigned strict deadline comparisons and DWORD delay addition are
  retained instead of inferred signed/clamped deadline behavior.
- `40AAE0` active-handle traversal moves completed streams to a retirement list.
  `40ABF0` processes loading before retirement; failed loads also retire their
  request. The loader releases both track owners and their `BufferRecord`,
  removing live handles. Stop drains queues even when workers were not started.
  Queue production/handle lookup is synchronized with retirement.
- BGM overlap storage is a stable owning list. The former 31-fading-track cap
  and forced eviction had borrowed the SE pool's unrelated 32-slot assumption.
  `40A0F0`/`40AAE0`/`40ABF0` use BGM handle lists; SE's 32-buffer pool remains.
- Audited `4099C0`: keep the 1 MiB ring, 0x8000-byte blocks, previous/next write
  boundaries, DirectSound write-cursor gate and EOF handling. No new decoder,
  loop-point format, asset search path or guessed audio content is introduced.

## Reconstruction boundaries and validation

The existing manual-reset stop event, COM failure handling, joined waits instead
of original busy polling, and lock around decode are retained reconstruction
choices. This is not a claim of identical thread timing. The native handle store
still uses its existing generation/index scheme and monotonic allocation slots;
retirement now destroys the owned record, but does not invent a reuse policy.

The existing contract source adds zero-volume retention, explicit retirement,
deferred record cleanup and more-than-32 overlapping owner checks. Per user
instruction, contracts and game must NOT be executed. Only compilation/linking
and DAT staging are performed; any original-equivalence statements above are
based on static evidence, not a gameplay or audio listening test.

## R1 build handoff

- Source commit: `c14c944f1d6581476961bf817ccbfe2bacf1a965`.
- Build: `build-runs/audio-scheduling-r1-quiet`, Win32 Release, trace disabled.
- All targets compiled and linked successfully (exit 0). Regression contracts
  were compiled only; no game, CTest or contract executable was run.
- EXE: `runtime-builds/audio-scheduling-r1-quiet/kinoko_retdec_rebuild.exe`.
- SHA256: `0B0AB1E9EBD8E76D4C30FC3F71DAF0BFC4711477E6B26DC19F0D06B99E71CD40`.
- All three DAT files were copied beside the EXE and size/SHA256 verified by
  `stage_dat.ps1` (exit 0). All earlier runtime/build artifacts are retained.

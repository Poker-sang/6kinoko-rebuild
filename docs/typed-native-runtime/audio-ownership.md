# Native audio ownership

Historical baseline below. The 2026-09-22 [audio scheduling batch](../audio-scheduling/README.md)
supersedes the fixed 31 fading-track description with a stable owning list,
consolidates worker/event/lock ownership, and restores explicit retirement of
playback records. Its local validation was compilation and DAT staging only;
no game or tests were executed. Original binary evidence is now available in
that batch's directory; the earlier validation limitations below describe the
earlier environment.

The active audio path is compiled as C++ in `src/reconstructed/audio_runtime.cpp`.
It replaces 82 functions formerly embedded in the generated C host. It calls the
Windows SDK DirectSound interfaces, not manually indexed COM virtual tables.

## Owners and borrows

- `AudioDevice` owns the module, device, primary buffer and optional listener.
  Release order is listener, primary, device, module. The remaining C globals
  g876/g877/g878 are borrowed aliases; they never acquire another reference.
- `BgmTrack` is move-only. It owns encoded input, decoded samples, scratch,
  decoder and secondary buffer. Decoder destruction precedes freeing the input
  it borrows. Moving current playback to one of the 31 fading tracks transfers
  ownership; it does not duplicate pointers or COM references.
- `SoundEntry` and `SoundSlot` own secondary-buffer references. `ComOwner` adopts
  exactly one reference; `get()` borrows, `put()` releases the old output slot,
  and `detach()` explicitly transfers ownership. Moving two distinct references
  to the same COM object still releases the destination's former reference.
- The manager owns native buffer records and their long path allocations.
  Pending/active/retired queues hold handles only. They do not own decoders or
  buffers. Generation/index bits are unchanged; overflowing the 16-bit slot
  field is now rejected instead of manufacturing an aliased handle.
- Worker handles are joined before buffers, decoder state, queues, critical
  sections or the device are destroyed. Closing a HANDLE alone is not joining.

## Preserved behavior

The existing asset reader and EXE-relative DAT search remain the loading path.
The g874 archive-mode flag still controls `.wav` to `.cv3` resolution. CV3 keeps
its original packed 18-byte WAVEFORMATEX and 4-byte payload length; neither the
asset format nor save format is rewritten. BGM ring/chunk sizes, refill logic,
loop points, fade order and diagnostic service/write switches are retained.

Native records have explicit `sizeof` and `offsetof` assertions. Unidentified
regions remain opaque; this change does not infer a new engine object layout.

## Verified defects corrected

The old unsigned cast around `CoInitialize` made its nonnegative test always
true. Workers now test the HRESULT with `SUCCEEDED`. Device cleanup no longer
writes through an uninitialized local pointer. SDK GUID objects replace taking
a GUID address from the first DWORD of split generated globals. Thread entry
points use the actual `DWORD WINAPI(void*)` signature. Failed partial manager
construction releases allocated queue sentinels before a retry.

## Validation limits

`com_owner_contract` exercises ownership transitions including self-move and two
references to one COM object. `audio_runtime_contract` compiles the production
implementation against a mock SDK buffer and the real Windows event/thread APIs;
it checks layout, FIFO order, handle generations, wrap writes, lock failure,
move/release, CV3 truncation, worker joins and repeated initialization/teardown.

Windows quiet and diagnostic build/test results must be checked at the final PR
revision. These asset-free contracts do not establish visual/audio or gameplay
parity with the original EXE and DAT files, which are unavailable here.

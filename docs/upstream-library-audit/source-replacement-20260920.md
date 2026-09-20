# Production source replacement, 2026-09-20

This continuation implements source replacement after the earlier acceptance-only
checkpoint. It does not claim that all remaining legacy ABI compatibility code
has been removed.

## Binding operations

Production now executes Sqrat BindFunc, Object getters/destruction and Function
execution, and SqPlus CreateArray, integer-key string SetValue, NewUserData,
GetUserData/RawGetUserData, GetTypeTag, SetDelegate and CreateInstance through
normally constructed upstream objects. The old operation bodies are deleted;
remaining wrappers marshal unaligned host records and preserve the recovered
return values, reference transfers and error policy. No upstream vtable is
installed in a byte record. Three generated Sqrat virtuals (4029F0, 402A10,
453580) are deleted and nine legacy vtables point at typed x86 entry adapters.

IDA evidence: original 415810 pushes closure then environment, calls 48ACE0
with one argument/no result/the byte at 5109DB, then pops one slot. The source
Function body performs that same sequence; the host passes its existing scoped
call entry and error flag without changing global Sqrat settings. Original
4029F0 and 453580 use ECX receivers and callee stack cleanup, tested by invoking
the replacement entries through thiscall function pointers. The instance factory
uses the source exception path but catches it before the recovered C boundary.

## Retired audio implementation

The actual BgmTrack owns VorbisDecoder; its State destructor executes upstream
ov_clear. BufferRecord is a request record, not an OggVorbis_File or a C++
decoder. The private manager never dispatches through the old handle/decoder
vtable words. Removing their assignments and host exports disconnects the
superseded implementation. The unused exported handle-table constructor is also
removed; native record sizes and used field offsets stay statically checked.

[Closed-component evidence](retired-audio-island.json) records source commit
f19c0aca93172fe3c172a334ae8378b50aa00c9b and both matching link maps. The audit
includes named functions, all retained source roots, tables, address-taking,
exact numeric addresses and interior literal references. It finds no external
inbound references to the selected component: 33 functions, seven data records,
1,595 definition lines. 27 nodes were still retained in those maps. All selected
bodies, prototypes and tables are now deleted, including the old ov_clear path
47AED0, codec teardown helpers, retired BgmBuffer/handle-manager virtuals and
their private support. The immutable original decompilation remains available.
This is a source-graph boundary proof, not proof for arbitrary computed pointers.

## Validation checkpoints

- 8c54ad6: r2 diagnostic/quiet builds, 52/52 CTests each.
- 56417b8: r3 diagnostic/quiet builds, 52/52 CTests each; Sqrat virtual ABI cases.
- f19c0ac: r4 diagnostic/quiet builds, 52/52 CTests each; retired audio identities.
- The subsequent removal/source-factory batch receives independent r5 build and
  runtime directories, with source-commit.txt before testing. Results are recorded
  separately after execution. Earlier gameplay approval does not validate r5.

Remaining game-specific Sqrat/SqPlus registration, native descriptors, legacy
Boost blocks and CRT exception/RTTI compatibility are separate migration work;
this change does not represent those adapters as upstream implementations.

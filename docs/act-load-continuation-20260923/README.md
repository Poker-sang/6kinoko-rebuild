# ACT load continuation

Previous batch-23 EXE was confirmed normal by the user on 2026-09-23. This is
user gameplay validation, not an agent-run test. Original 6kinoko.exe SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session 23485cc6 (temporary copy); the earlier Squirrel 2.2.2 source
disassembly in analysis/remaining-mapping-20260920 remains the VM auxiliary.
This sequence addresses native ACT loading, not VM opcode behavior.
Batch 24: original 43C860 accepts a borrowed 3D layout plus a reader-holder
pointer and version 1, then delegates the property stream. The implementation
now exposes those actual pointer types and the legacy vtable adapters convert
only at their integer ABI boundary. Property order and schema stay unchanged.
Evidence: 43c860.json.
Batch 25: the native ACT key parser already owns its local reader value and
borrows the newly created 3D layout. It now passes a real layout pointer and
address of that reader slot directly to the recovered 43C860 interface.
The legacy virtual adapter stays available for original ABI callers; key
failure cleanup and subsequent publication remain unchanged.
Batch 26: original 43FB00 takes a borrowed CStringLayout and reader-holder
pointer, accepts version 1, reads properties, then joins displayed/pending
strings before clearing displayed text. The native reader exposes actual
pointer types and named string fields; the original virtual ABI retains a
small conversion adapter. The key loader passes its local reader slot.
Evidence: 43fb00.json.
Batch 27: IDA 42C030 reads version-1 C2DLayout properties through a
borrowed reader holder and writes byte 312 after successful values.
The native API now takes real pointers and sets Layout2DRecord's asserted
pivot-invalid flag; its virtual ABI adapter and parsed property order remain.
The document factory passes its local reader slot directly. Evidence: 42c030.json.
Batch 28: texture, render-target and chip property loaders all borrow a
resource and the same active reader slot. The native factory now passes
actual pointers and each reader exposes a typed boundary while the virtual
ABI adapters retain their signatures. The distinct schemas and texture
crop invalidation remain in their original order; no eager load is added.
Batch 29: mesh resource creation already returns a typed Resource owner.
Its distinct ACT schema reader now borrows that Resource and the active
reader slot directly, with the virtual C ABI only converting at its edge.
The factory still clears and frees the mesh on read failure, and resource
loading remains deferred to the original resource pass.
Batch 30: the key and layer property streams borrow newly allocated ACT
records and the active archive reader. The document parser now calls typed
record/reader APIs; existing integer ABI functions remain adapters for
legacy callers. Key presence/type checks, layer key/timeline loops and
schema ordering are unchanged.
Batch 31: the map-layout and root ACT document property readers borrow
the existing record plus active archive reader. Both native call sites now
pass actual pointers, while old integer ABI entries delegate to them.
Map record parsing, parent association before resource count, and the
separate post-parse resource pass retain their prior order.

## Batches 32每51 (2026-09-23)

The user confirmed the batch-31 game build behaved normally before this
sequence. That confirmation is user validation, not an agent-run smoke test.
IDA MCP session `a86383bd` was used to check the original ACT entry points,
including 41F800 (layer loader), 428150 (document loader) and 42B4A0
(C2DLayout constructor). These batches keep the recovered Squirrel 2.2.2
implementation as auxiliary context; no VM opcode implementation was changed.

- 32每36: describe the CActScript payload at +92/+96/+100/+101, migrate load,
  destroy, read, write and initialization to named fields. Read-failure and
  write-state restoration still follow their prior behavior.
- 37每38: initialize the C2DLayout's embedded sprite, scale and color through
  its verified layout record, then retain typed factory ownership until return.
- 39每41: describe the 32-byte map cell's runtime index/enabled/opacity, map
  layout factory fields and the native-buffer-backed record span.
- 42每43: describe the CActKey owned layout and script name, then carry the
  decoded layout as a pointer until it is installed into the key.
- 44每47: use typed key objects, named key/timeline list counts and layer
  identity fields; keep new layer ownership typed until parent publication.
- 48每51: prepare and publish document layer/resource arrays through their
  named spans, preserving the original parent-resolution and resource-index
  ordering before the later resource loading pass.

Each numbered source commit preceded a separate quiet Win32 Release build,
DAT staging and SHA256 verification, followed by a numbered handoff commit
pushed to the PR branch. The initial batch-35 and batch-38 builds failed on
C++ type checks; their artifacts were retained and corrected versions built
in fresh directories. No game, CTest or contract executable was run by the
agent. The last EXE is `runtime-builds/act-document-resource-publish-r51-quiet/kinoko_retdec_rebuild.exe`
(source commit `f8142ac`, SHA256
`9FD2D3F9CD2B158CC24A29E2B98EEEECF8810D1EC13A89B12202601AAA19B87A`).

The post-batch lexical audit of 154 first-party files finds 1,815 functions:
1,308 named structured candidates, 175 named mixed legacy, 176 thin bridges,
and 156 address-named implementations. This is only a readability inventory;
it does not prove runtime equivalence. The remaining clusters are largely
in the decompiled host, Squirrel host/bindings, audio and stage/input paths.

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

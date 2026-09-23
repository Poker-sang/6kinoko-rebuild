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

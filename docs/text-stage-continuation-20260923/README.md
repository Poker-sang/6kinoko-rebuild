# Text and stage continuation

Original: C:/WorkSpace/6kinoko/6kinoko.exe, SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session 7251442a. Squirrel 2.2.2 source disassembly was consulted
for VM provenance; selected text/stage functions are native game code.

Batch 1: 441250 prunes atlas records only when their last glyph precedes the
live glyph minimum and their reference count is zero. Glyph ID and atlas
texture/reference fields use original byte offsets with schema assertions.
The layout receiver is a borrowed pointer; an empty glyph queue retains the
original INT_MAX minimum.
Batch 2: 4410C0 rebuilds the pending string from text plus pending, resets
cursor and line metrics, decrements each borrowed glyph atlas reference,
clears deque storage, then invokes 441250. IDA MCP database 7251442a
confirmed the offsets and call order. Its C ABI now carries a borrowed
KinokoStringLayout pointer and the mapped layout/glyph/atlas fields are
accessed by named records.
Batch 3: the glyph deque's copy, pop, clear and pending-text paths now
use StringLayoutRecord, StringGlyphRecord and AtlasLifecycle rather than
numeric member offsets. Borrowed atlas references still change before deque
mutation, while copy_queue_storage retains its original no-adjust contract.
Batch 4: IDA MCP 412CA0 confirms ImmGetContext/ImmGetDefaultIMEWnd
for the original HWND and CFS_EXCLUDE (128) with a zero-origin client
area derived from GetWindowRect. The active application call now uses
kinoko_ime_initialize in the IME module; original context globals retain
their binary layout and release path.
Batch 5: IDA MCP 4D3CE0/4D3E50/4D3F50 shows three global
container initializers that each register a corresponding atexit cleanup.
The existing native list, render queue and sound tree ownership remains
unchanged; startup and contract callers now use named registration APIs.
Batch 6: native 440910 adds a glyph to the current atlas. Its renderer
now accesses the already verified glyph ID/borrowed atlas, atlas texture,
last ID/reference count and layout next-ID fields via typed records. The
original texture upload, geometry and deque insertion order is preserved.
Batch 7: in original 440910, cursor X/Y, line height and wrap threshold
are text-layout members, while glyph X/Y/width/height are sprite members.
The native character path now writes these named fields in the same order,
including tab and newline handling; atlas packing offsets remain pending.
Batch 8: original 440910 atlas packing maintains page X/Y, current row
height and 512x512 limits at offsets 0/4/8/12/16. These fields are now
part of the asserted AtlasLifecycle layout. Upload, row advance, and glyph
rectangle use named fields with unchanged comparisons and update order.

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
Batch 9: user confirmed the preceding batch-8 EXE worked normally; this is
user-run validation, not an agent gameplay test. Fresh IDA MCP session b9a84557
for original SHA256 2db975a4... confirms CStringLayout 43E890 initializes
font, face, colors, spacing, alignment, metrics and layer defaults. Named
StringLayoutRecord fields now carry those values without changing allocation
and container construction order. Evidence: 43e890.json.
Batch 10: IDA 43EA10 clears displayed and pending strings, rebuilds
then prunes glyph atlases, destroys glyph/atlas containers, and destroys
face/pending/displayed strings in order. The native destructor now uses
StringLayoutRecord/StringRecord members for each string storage and capacity
reset; its ownership order and public ABI remain intact. Evidence: 43ea10.json.
Batch 11: 43E890 creates the atlas vector owner at layout +160 and 43EA10
deletes it after glyph queue teardown. The layout schema now asserts atlas
and glyph owner slots at +160/+176; atlas access, creation and deletion use
the explicit owner member. Other historical vector/deque bookkeeping slots
remain untouched at their original offsets.
Batch 12: original 43E890/43EA10 constructs and destroys the glyph deque
at layout +176. Native deque access now reads the asserted glyph_owner
field and construction/destruction publishes or clears that owner pointer.
Glyph records continue to borrow atlas pages; no refcount order changed.
Batch 13: IDA 43EC30 copies three independent strings, style bytes
88..127, edge byte 128, style bytes 132..159 and tail bytes 200..259,
with padding 129..131 untouched. The clone uses named record member
boundaries for these spans and retains its existing atlas/deque assignment
and subsequent clearing. Evidence: 43ec30.json.
Batch 14: IDA 40ED40 destroys renderer payloads before the bitmap buffer
at renderer +344; 445230 copies the renderer string at +368 separately
from the first 348 bytes. The asserted 404-byte FontRendererRecord names
the bitmap, pixel owner and label slots. Renderer assignment/destruction
now uses those slots, retaining shallow buffer-copy and release order.
Evidence: 40ed40.json and 445230.json.
Batch 15: original 40EC70 allocates the renderer pixel-list owner at
+348 and initializes the label record at +368/+384/+388. Original 40ED40
releases pixel nodes, bitmap and label before deleting the list owner.
The font module now publishes/clears the explicit pixel_owner pointer and
initializes the named label storage. Evidence: 40ec70.json and 40ed40.json.
Batch 16: original 440CA0 transfers CStringLayout face, RGB low bytes,
font height/weight, edge flag, and character/line spacing into the font
renderer before texture setup. The font configuration now reads these
named layout members and preserves the low-byte color behavior and call
order. Evidence: 440ca0.json.
Batch 17: original 440CA0 writes a 256-byte face, paired RGB endpoints,
font height/weight, edge, spacing and packed color into renderer fields.
FontRendererRecord now asserts those source-backed offsets and the native
configuration uses them, leaving unrelated runtime and padding bytes intact.
Batch 18: original 40EC70 initializes the renderer weight, style flags,
spacing, bitmap pointer, label length/capacity and pixel owner. Native
construction now names these members and preserves untouched runtime and
padding bytes. The original +352 list-size and DC/runtime zeroes remain at
their explicit binary boundary.
Batch 19: IDA 40F1C0 creates/selects a font for one glyph, publishes DC
and previous font, computes ascent/X/Y from edge and margins, then 40F2E0
restores the DC/font. The renderer schema now asserts runtime slots 0..8
and 308..343; FontSession uses these named fields without changing Windows
call order or the per-glyph pixel clear. Evidence: 40f1c0.json, 40f2e0.json.
Batch 20: original 40F370's glyph raster path reads DC, X/Y cursor,
ascent, bounds/stride, output/gradient and color from renderer offsets.
The native path now uses the asserted renderer fields, keeping its exact
size, wrap, bitmap and alpha operations. Evidence: 40f3c8.json.
Batch 21: the font's optional edge pass uses renderer edge, temporary
output, destination, stride and bounds. The native outline/rasterize
functions now share the same typed runtime fields as 40F370's glyph path,
retaining the original bitmap loop and edge-width accounting.

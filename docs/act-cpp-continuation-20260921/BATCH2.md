# Shared document schema and typed layer/key queries

The layer query, update, source count and renderer now share DocumentRecord.
The duplicate 240-byte DocumentLayers schema is removed. The live layer range
uses KinokoActLayer pointer representations, and the native ActArray owner
remains explicit. Its current integer slot representation is read via memcpy,
not through an incompatible pointer-array reference.

455DC0 and 455F50 return separately allocated one-word wrappers. Their borrowed
layer/key pointers and the output slots are now typed end to end. The allocator
is unchanged; conversion from its legacy integer return is localized there.
452040 and 452020 become kinoko_act_first_key and kinoko_act_layer_layout, with
borrowed pointer returns. 41EFB0 becomes kinoko_act_layer_update; it still copies
position bits before calling the same script callback. The root callback uses a
named prefix within the document's script, not another document-wide overlay.
The frame loop continues to reload source/count and active holder after script
callbacks; the reverse draw order and virtual targets are unchanged.

The original 455DC0/455F50/452040/452020 decompilation supports the wrapper size,
layer range at 208/212, key list/count at 180/184, and key layout pointer at +4.
It is damaged around implicit receivers and some returns. Current null/negative/
broken-chain guards and callback result forwarding are retained reconstruction
behavior, not newly claimed original logic. No resource-load pass, callback,
retry, missing asset replacement, or failure cleanup policy is added here.

An isolated optimized test links the actual query source and controls only its
allocator and traces. It exercises guarded unaligned raw records, borrowed
object preservation, both wrapper allocation failures, key list/bounds/null
cases and the existing key-before-layer release trace order. The broad C tests
retain their independent raw fixture offsets and assertions. Existing frame
mutation and real-file lifetime tests remain selected.

All source modifications are committed before execution tests. Build results
will be recorded separately. Original evidence files and hash expectations are
unchanged. No original DAT or gameplay validation is implied.

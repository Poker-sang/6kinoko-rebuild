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

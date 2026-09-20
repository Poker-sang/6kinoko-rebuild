# Local defined-arithmetic adaptation to libogg 1.1.3

The source origin and unmodified archive/member hashes remain in `UPSTREAM.json`.
This is an altered source version. `PATCHES.json` pins each changed member.

`bitwise.c`: cast byte operands to unsigned long **before** left shifts in both
LSB-first and MSB-first look/read. Integer promotion previously made e.g.
`255 << 24` and `255 << 31` signed-int undefined behavior. The public long
return type, 32-bit mask, bit position, EOF sentinel and byte-consumption
rules are unchanged. This does not adopt a later Ogg page-flushing algorithm.

Evidence: the independent 0–32-bit oracle in `vorbis_decoder_contract` covers
all eight bit offsets, both bit orders, all-ones/all-zero/random input and EOF;
it exposed six shift sites with UBSan on revision `2cc8a985`. Encoded fixture
and decoded PCM snapshots are compared under the same toolchain, not across
unrelated float implementations. No original-game oracle is claimed.

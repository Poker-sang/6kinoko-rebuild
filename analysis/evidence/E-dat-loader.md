# E-dat-loader: archive format and runtime boundary

Recorded 2026-08-23 from IDA MCP session `bd667883`, then checked against the local DAT files.

## Static evidence

- `_WinMain@16` at `0x473b30` calls `sub_407360` for `6kinoko_a.dat`, `6kinoko_b.dat`, and `6kinoko_c.dat`.
- `sub_407360` at `0x407360` is a wrapper around `sub_410500`.
- `sub_410500` at `0x410500` opens the file with `CreateFileA`, reads a 2-byte count, a 4-byte index size, then reads and decodes the index payload.
- `sub_404230` at `0x404230` seeds a 624-word state with `seed = index_size + 6` and `0x6c078965`.
- `sub_404270` at `0x404270` twists the state and applies the original temper masks `0xff3a58ad` and `0xffffdf8c`.
- `sub_410750` at `0x410750` stores each path in a resource tree and associates it with the current archive index.
- `sub_4109d0` at `0x4109d0` resolves a normalized path, opens the associated DAT, seeks to the stored offset, and limits reads to the stored size.

## Decoded record layout

After the two byte-wise XOR layers, each index record is:

```text
uint32 offset
uint32 size
uint8  path_length
byte   path[path_length]
```

The first XOR layer consumes the custom MT-style generator output. The second layer starts with `0xc5` and `0x89`; after each byte it adds the current second key to the first and adds `0x49` to the second, modulo 256.

## Local validation

The reconstructed loader decoded the following headers and counts:

| Archive | File size | Index size | Entries |
| --- | ---: | ---: | ---: |
| `6kinoko_a.dat` | 163424746 | 67161 | 1638 |
| `6kinoko_b.dat` | 44681244 | 1331 | 52 |
| `6kinoko_c.dat` | 11796163 | 4790 | 138 |

`kinoko_archive_smoke.exe D:\\6kinoko` reads one real entry from each archive. The current mount order preserves the original override behavior: later archives replace an earlier entry with the same normalized path. The run indexed 1828 records and retained 1742 unique paths.


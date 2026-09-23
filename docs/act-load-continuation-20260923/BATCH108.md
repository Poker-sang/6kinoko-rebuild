# R108: inline ACT texture buffer helper

Source commit: `9491413`.

The private `replace_buffer` helper in `src/reconstructed/act_texture_io.cpp`
was removed. Its four call sites now invoke `kinoko_native_buffer_replace`
directly with explicit `uint32_t` size conversion. Public ABI readers and
writers were left unchanged.

The readability inventory changed from 108 to 107 thin bridges and from 491
to 488 estimated thin-bridge body lines. `named_mixed_legacy` remains zero.

## Build artifact

- Win32 Release build tree: `build-runs/bridge-cleanup-r108`
- Delivery executable: `runtime-builds/bridge-cleanup-r108/kinoko_retdec_rebuild.exe`
- EXE SHA256: `ACE1587EB8171616A4E614C1A52BF20EA81D31CADFCA54D97A16CBDA3377047F`
- `6kinoko_a.dat`: `DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`
- `6kinoko_b.dat`: `4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`
- `6kinoko_c.dat`: `80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`

The three DAT files were staged beside the executable and hash-verified by
`tools/stage_dat.ps1`. Per the test handoff, no game, CTest, or contract
executable was run by the agent.

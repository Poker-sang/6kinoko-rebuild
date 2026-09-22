# Script File Execution R1

- Source commit: `d0e134fb1f1bc05222cd987b0c216245ca1d9e6e` (committed before build).
- Build: `build-runs/script-file-r1-quiet`, Visual Studio 18 2026, Win32 Release,
  `KINOKO_RETDEC_DISABLE_TRACE=ON`.
- Full build completed successfully, including contract executables. Existing
  compiler warnings remain; this is not a claim of a warning-free build.
- EXE: `runtime-builds/script-file-r1-quiet/kinoko_retdec_rebuild.exe`.
- EXE SHA256: `96E121FBA98D24E7910654AE023E07DA7B23EC1F60D1140A327C09CD57EFE970`.
- Log: `build-runs/script-file-r1-quiet/build.log`.
- Game, contract executables and CTest were **not executed**. Runtime testing is
  handed to the user; no immediate test is required.

`tools/stage_dat.ps1` copied and verified all three resources beside the EXE:

| File | Bytes | SHA256 |
| --- | ---: | --- |
| 6kinoko_a.dat | 163424746 | DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64 |
| 6kinoko_b.dat | 44681244 | 4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2 |
| 6kinoko_c.dat | 11796163 | 80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E |

No original saves or index.dat were copied. Existing build/run directories and
artifacts were preserved. This title starts its own R1 numbering.

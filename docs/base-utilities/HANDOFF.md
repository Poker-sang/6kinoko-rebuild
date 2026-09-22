# Base Utilities R2

- Source commit: `13b613169ada6a38f93ee29b8b50597090614be6`, committed before build.
- Build tree: `build-runs/base-utilities-r2-quiet`.
- Visual Studio 18 2026, Win32 Release, `KINOKO_RETDEC_DISABLE_TRACE=ON`.
- Full build exit code 0, including base utility and updated script, application,
  device and stage contract executables. Existing warnings remain.
- EXE: `runtime-builds/base-utilities-r2-quiet/kinoko_retdec_rebuild.exe`.
- EXE SHA256: `DB344C7D587B5C64E59657513465AB1E8E281D26DABF87A35A716BEA0C8BB09D`.
- Build log: `build-runs/base-utilities-r2-quiet/build.log`.
- No game, CTest or contract executable was executed. Runtime validation remains
  assigned to the user; no immediate test is required.

This batch removes ten selected address-named bodies from the main C file:
eight are recovered as named interfaces and two obsolete iterator helpers are
deleted. It also corrects the preceding script batch's throwing C-linkage
declarations under MSVC /EHsc.

`tools/stage_dat.ps1` copied and verified exactly these three resources beside
the EXE, without copying index.dat or original saves:

| File | Bytes | SHA256 |
| --- | ---: | --- |
| 6kinoko_a.dat | 163424746 | DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64 |
| 6kinoko_b.dat | 44681244 | 4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2 |
| 6kinoko_c.dat | 11796163 | 80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E |

R1 source `d471c55` and its failed compilation log remain in
`build-runs/base-utilities-r1-quiet`; all earlier build/runtime artifacts remain.
R2 is the second revision within the Base Utilities batch.

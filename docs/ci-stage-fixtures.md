# Stage CI fixture repair (2026-09-22)

The user authorized local CI test execution before pushing this repair, replacing
the earlier compile-only instruction for this work. No game was launched.

The two failing CI tests share startup-free stage fixtures. Successive failures
were exposed after earlier fixture failures were fixed:

- EndStage received integer 1234 as a native vector owner. The fixture now creates
  command/sprite containers through the production APIs, checks that clearing
  preserves owners/capacity, and destroys both containers afterward.
- Adjacent C strings omitted line breaks between Squirrel statements. Map alias
  checks and remaining stage script boundaries now use explicit newlines.
- The stage fixture used the global map event list without constructing its
  native container. It now constructs/destroys that container.
- The damage-mask fixture calls the game update path (which always updates input)
  without game startup. It now constructs/destroys native input device, cluster
  and key containers without creating hardware devices.

Only tests and CI triggering were changed. No runtime invalid-pointer suppression,
skipped assertions, or disabled failing tests were introduced. Windows x86 now
automatically runs on push only; manual dispatch remains available.

Local successful source: `d832e51`.
Build/run directories: `build-runs/ci-stage-fixture-r4-quiet` and
`runtime-builds/ci-stage-fixture-r4-quiet`.
Win32 Release, quiet, trace filter enabled, reference directory set to the same
nonexistent asset directory convention as CI. Full build succeeded. The exact
CTest regex was extracted from `.github/workflows/windows-x86.yml`:
**36/36 passed**, including stage_native_contract and damage_pause_contract.
Log: `build-runs/ci-stage-fixture-r4-quiet/ctest.log`.

Migration boundary checks and the legacy-islands, upstream-provenance, and ACT
evidence Python suites also passed locally. The evidence JSON had stale CRLF
bytes in this workspace despite its existing eol=lf attribute; restoring its
exact committed bytes made the pinned hash check pass without changing the
reference or weakening its check.

All R1–R4 build logs/artifacts remain. Three DATs were copied and SHA256-verified
beside each generated game EXE (R2–R4); the tests did not use those game assets.
The R1 build produced only the stage contract target.

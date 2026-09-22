# Texture Binding R1 handoff

Source commit: 4b456631a0fb63482ccb82f4efa62d7506670e60 (master), before build.
Build tree: build-runs/texture-binding-r1-quiet.
Executable: runtime-builds/texture-binding-r1-quiet/kinoko_retdec_rebuild.exe.
SHA256: BFB0E5CC67CE0629F62EDDC4E0F3223415F2F702D86C48ADAA10675523108286.

Visual Studio 18 2026 / Win32 Release / KINOKO_RETDEC_DISABLE_TRACE=ON.
Configure and full default-target build exited 0. This includes the new binding
contract and updated texture store, stage, device, actor, quad and ACT contracts.
Existing warnings remain; no game, CTest or contract executable was executed.
stage_dat.ps1 exited 0: all three original DAT files copied beside EXE and
verified by size/SHA256. All build/log/runtime artifacts remain independently
available; no previous build was overwritten or removed.

Three-batch result: typed CV2 receiver/fields/pixels, typed D3D image output and
reference transfer, original square-cap/creation-lock behavior, named eight-stage
binding keys and final-release/reset invalidation. Four original address-named
entry definitions (414010, 40E630, 405D60, 405E30) and eight split stage globals
are retired from active sources. Compile success is not gameplay equivalence.
Palette/RLE/loose-BMP branches remain explicitly documented in README.md.

The user merged PR #9 during batch 2. Batches 2/3 continue on updated master;
source and handoff records are pushed there, not to the merged PR branch.
The unrelated R126 original-stage-evidence working-tree change remains untouched
and excluded from these commits.

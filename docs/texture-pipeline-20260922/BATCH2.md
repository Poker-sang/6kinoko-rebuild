# Texture Upload R1 handoff

Built source: 38da65058e5f83e707c925d1d9ad0db7de9e3178, committed before build.
While that build completed, the user merged PR #9 and requested continuation
on current master. Fetched origin, fast-forwarded master to merge 5e013c1,
and cherry-picked the one unmerged source commit as 3e593524 (full source,
headers, tests, CMake and third_party trees verified identical to built source).
No further push is made to the merged PR #9 branch.

Build tree: build-runs/texture-upload-r1-quiet.
Executable: runtime-builds/texture-upload-r1-quiet/kinoko_retdec_rebuild.exe.
SHA256: 53A2FDA87EA0B943F83880198364FFDCA25DB2D503D7CE55C2DB535DD2E80C06.
Visual Studio 18 2026 / Win32 Release / KINOKO_RETDEC_DISABLE_TRACE=ON.
Configure and full default-target build exited 0, including texture_image_contract
and the updated texture_store_contract. Existing warnings remain.
stage_dat.ps1 exited 0: exactly the three original DAT files copied beside EXE
and verified by size/SHA256. Configure/build/DAT logs and all old products remain.
No game, CTest or contract executable was executed; user runtime validation is
separate. An unrelated working-tree edit to the old R126 original-stage evidence
was preserved across the branch switch and is not part of this batch.

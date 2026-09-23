# Batch 95 - r90-regression-fix-quiet-r95

Source committed before build: f3655b3 (`Restore shared VM publication during root registration`).

Regression scope: r89d (`44710127`) to r90b (`c2a23aba`). The fix restores the removed
`g664 = address(current_vm())` publication before root registration and keeps the
saved `vm_address` sourced from `g664`. `script_file.cpp` still uses `g664` for the
bytecode VM, so this restores the shared VM slot without changing the camera/map
binding migration.

Build: `build-runs/r90-regression-fix-quiet-r95`.
EXE: `runtime-builds/r90-regression-fix-quiet-r95/kinoko_retdec_rebuild.exe`.
EXE SHA256: `F14AF0320ED21EB212B1B751B3F293A837B207B5AB26960952C3AAC8B968D303`.

Quiet Win32 Release configure and full build succeeded, including contract
compilation. Three original DATs were staged beside the EXE and size/SHA256
verified.

No game, CTest, contract executable, or local automated test was executed; runtime
verification is delegated to the user.
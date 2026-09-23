param([string]$Name,[int]$Batch)
$ErrorActionPreference='Stop'
$BuildTree="build-runs/$Name"
if (Test-Path $BuildTree) { throw "Existing build is retained: $BuildTree" }
New-Item -ItemType Directory -Path $BuildTree | Out-Null
$SourceCommit=git rev-parse HEAD
$SourceCommit | Set-Content "$BuildTree/source-commit.txt"
cmake -S . -B $BuildTree -G 'Visual Studio 18 2026' -A Win32 -DKINOKO_REFERENCE_DIR=C:/WorkSpace/6kinoko "-DKINOKO_RUNTIME_DIR=C:/WorkSpace/6kinoko-rebuild/runtime-builds/$Name" -DKINOKO_RETDEC_DISABLE_TRACE=ON *> "$BuildTree/configure.log"
if ($LASTEXITCODE -ne 0) { Get-Content "$BuildTree/configure.log" -Tail 12; exit 1 }
cmake --build $BuildTree --config Release --parallel 4 *> "$BuildTree/build.log"
if ($LASTEXITCODE -ne 0) { Select-String -Path "$BuildTree/build.log" -Pattern 'error ' | Select-Object -First 10; exit 1 }
powershell -NoProfile -ExecutionPolicy Bypass -File tools/stage_dat.ps1 -Executable "runtime-builds/$Name/kinoko_retdec_rebuild.exe" -SourceDir C:/WorkSpace/6kinoko *> "$BuildTree/dat.log"
if ($LASTEXITCODE -ne 0) { Get-Content "$BuildTree/dat.log" -Tail 10; exit 1 }
$Digest=(Get-FileHash "runtime-builds/$Name/kinoko_retdec_rebuild.exe" -Algorithm SHA256).Hash
$Record="docs/text-stage-continuation-20260923/BATCH$Batch.md"
@"
# Batch $Batch - $Name

Source committed before build: $SourceCommit.
Build: $BuildTree.
EXE: runtime-builds/$Name/kinoko_retdec_rebuild.exe.
SHA256: $Digest.

Quiet Win32 Release configure and full build succeeded, including contract compilation.
Three original DATs were staged beside EXE and size/SHA256 verified.
No game, CTest, contract executable or local automated test was executed.
All earlier artifacts retained. Existing compiler warnings remain.
"@ | Set-Content $Record -Encoding utf8
git add $Record
git commit -m "Record text/stage batch $Batch quiet build and DAT handoff"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
git push
exit $LASTEXITCODE

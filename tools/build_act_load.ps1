param(
    [Parameter(Mandatory=$true)][string]$Name,
    [Parameter(Mandatory=$true)][int]$Batch,
    [Parameter(Mandatory=$true)][string]$SourceDir,
    [string]$Generator
)
$ErrorActionPreference='Stop'
$SourceDir=(Resolve-Path -LiteralPath $SourceDir).Path
Set-Location -LiteralPath (Join-Path $PSScriptRoot '..')
& (Join-Path $PSScriptRoot 'build_staged.ps1') -Name $Name -SourceDir $SourceDir -Generator $Generator
$BuildTree="build-runs/$Name"
$SourceCommit=(Get-Content -LiteralPath "$BuildTree/source-commit.txt" -Raw).Trim()
$Digest=(Get-FileHash "runtime-builds/$Name/kinoko_retdec_rebuild.exe" -Algorithm SHA256).Hash
$Record="docs/act-load-continuation-20260923/BATCH$Batch.md"
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
git commit -m "Record ACT load batch $Batch quiet build and DAT handoff"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
git push
exit $LASTEXITCODE


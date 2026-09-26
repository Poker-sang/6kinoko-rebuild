param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9_-]*$')]
    [string]$Name,
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,
    [string]$Generator
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$assets = (Resolve-Path -LiteralPath $SourceDir).Path
$buildTree = Join-Path $repo "build-runs/$Name"
$runDirectory = Join-Path $repo "runtime-builds/$Name"
foreach ($path in @($buildTree, $runDirectory)) {
    if (Test-Path -LiteralPath $path) { throw "Existing artifacts are retained: $path" }
}
foreach ($letter in @('a', 'b', 'c')) {
    if (-not (Test-Path -LiteralPath (Join-Path $assets "6kinoko_$letter.dat") -PathType Leaf)) {
        throw "Required DAT missing from explicitly supplied SourceDir: 6kinoko_$letter.dat"
    }
}
if (Test-Path -LiteralPath (Join-Path $repo '.git')) {
    $revision = (& git -C $repo rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Cannot identify source revision' }
    $changes = & git -C $repo status --porcelain --untracked-files=no
    if ($LASTEXITCODE -ne 0 -or $changes) { throw 'Commit tracked changes before building a recorded batch' }
} else {
    # A git archive is independently buildable; export its exact revision beside it.
    $revision = (Get-Content -LiteralPath (Join-Path $repo 'SOURCE_REVISION.txt') -Raw).Trim()
}
if ($revision -notmatch '^[0-9a-f]{40}$') { throw 'Invalid source revision' }
New-Item -ItemType Directory -Path $buildTree | Out-Null
$revision | Set-Content -LiteralPath (Join-Path $buildTree 'source-commit.txt') -Encoding ascii
$configure = @('-S', $repo, '-B', $buildTree, '-A', 'Win32',
    "-DKINOKO_REFERENCE_DIR=$assets", "-DKINOKO_RUNTIME_DIR=$runDirectory",
    '-DKINOKO_RETDEC_DISABLE_TRACE=ON')
if ($Generator) { $configure += @('-G', $Generator) }
& cmake @configure *> (Join-Path $buildTree 'configure.log')
if ($LASTEXITCODE -ne 0) { throw "Configure failed; see $buildTree/configure.log" }
& cmake --build $buildTree --config Release --parallel 4 *> (Join-Path $buildTree 'build.log')
$buildExit = $LASTEXITCODE
$executable = Join-Path $runDirectory 'kinoko_retdec_rebuild.exe'
# A partial all-target failure may still produce the game; stage it for traceability.
if (Test-Path -LiteralPath $executable) {
    & (Join-Path $PSScriptRoot 'stage_dat.ps1') -Executable $executable -SourceDir $assets *> (Join-Path $buildTree 'dat.log')
}
if ($buildExit -ne 0) { throw "Build failed; retained artifacts/log: $buildTree/build.log" }
if (-not (Test-Path -LiteralPath $executable)) { throw 'Build did not produce the game executable' }
$files = foreach ($file in @($executable) + @('a','b','c' | ForEach-Object {
    Join-Path $runDirectory "6kinoko_$_.dat"
})) {
    $item = Get-Item -LiteralPath $file
    [ordered]@{ name=$item.Name; size=$item.Length; sha256=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash }
}
$record = [ordered]@{
    source_commit=$revision; configuration='Win32 Release'; trace_disabled=$true
    source_directory=$repo; build_directory=$buildTree; runtime_directory=$runDirectory
    generator=$Generator; full_build_succeeded=$true; dat_staging_verified=$true
    contracts_compiled=@(Get-ChildItem -LiteralPath (Join-Path $runDirectory 'tools') -Filter '*_contract.exe').Count
    game_run=$false; tests_run=$false; files=$files
}
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $buildTree 'artifacts.json') -Encoding utf8
Write-Output "SOURCE $revision"
Write-Output "EXE $executable"
Write-Output "MANIFEST $buildTree/artifacts.json"

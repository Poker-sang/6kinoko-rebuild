param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeDir
)

$ErrorActionPreference = 'Stop'

$sourceRoot = (Resolve-Path -LiteralPath $SourceDir).Path
if (-not (Test-Path -LiteralPath $RuntimeDir)) {
    New-Item -ItemType Directory -Path $RuntimeDir | Out-Null
}
$runtimeRoot = (Resolve-Path -LiteralPath $RuntimeDir).Path

$datNames = @(
    '6kinoko_a.dat',
    '6kinoko_b.dat',
    '6kinoko_c.dat'
)

$datFiles = foreach ($datName in $datNames) {
    $sourcePath = Join-Path $sourceRoot $datName
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Required DAT file is missing: $sourcePath"
    }
    Get-Item -LiteralPath $sourcePath
}

foreach ($sourceFile in $datFiles) {
    $destination = Join-Path $runtimeRoot $sourceFile.Name
    Copy-Item -LiteralPath $sourceFile.FullName -Destination $destination -Force

    $sourceHash = (Get-FileHash -LiteralPath $sourceFile.FullName -Algorithm SHA256).Hash
    $destinationHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    if ($sourceHash -ne $destinationHash) {
        throw "Hash mismatch after copying $($sourceFile.Name)"
    }

    $destinationFile = Get-Item -LiteralPath $destination
    if ($sourceFile.Length -ne $destinationFile.Length) {
        throw "Size mismatch after copying $($sourceFile.Name)"
    }

    Write-Output ("{0} {1} bytes SHA256={2}" -f $sourceFile.Name, $destinationFile.Length, $destinationHash)
}

Write-Output ("Staged required DAT set ({0} files) into {1}" -f $datFiles.Count, $runtimeRoot)

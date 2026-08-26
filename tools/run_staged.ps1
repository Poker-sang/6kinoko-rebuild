param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [string]$SourceDir = 'D:\6kinoko',

    [string]$RunsRoot,

    [string]$RunName,

    [string[]]$ArgumentList = @(),

    [switch]$Wait
)

$ErrorActionPreference = 'Stop'

$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($RunsRoot)) {
    $RunsRoot = Join-Path $projectRoot 'runtime-runs'
}

$executablePath = (Resolve-Path -LiteralPath $Executable).Path
$sourceRoot = (Resolve-Path -LiteralPath $SourceDir).Path
$executableItem = Get-Item -LiteralPath $executablePath
if ($executableItem.Extension -ine '.exe') {
    throw "Executable must be a .exe file: $executablePath"
}

$requiredDatNames = @(
    '6kinoko_a.dat',
    '6kinoko_b.dat',
    '6kinoko_c.dat'
)
$sourceDatFiles = foreach ($datName in $requiredDatNames) {
    $sourcePath = Join-Path $sourceRoot $datName
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Required DAT file is missing: $sourcePath"
    }
    Get-Item -LiteralPath $sourcePath
}

New-Item -ItemType Directory -Path $RunsRoot -Force | Out-Null
$runsRootPath = (Resolve-Path -LiteralPath $RunsRoot).Path

if ([string]::IsNullOrWhiteSpace($RunName)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmssfff'
    $RunName = '{0}-staged-{1}' -f $executableItem.BaseName, $stamp
}
$runtimePath = Join-Path $runsRootPath $RunName
if (Test-Path -LiteralPath $runtimePath) {
    throw "Refusing to reuse an existing runtime directory: $runtimePath"
}
New-Item -ItemType Directory -Path $runtimePath | Out-Null
$runtimeRoot = (Resolve-Path -LiteralPath $runtimePath).Path

$stagedExecutable = Join-Path $runtimeRoot $executableItem.Name
if (Test-Path -LiteralPath $stagedExecutable) {
    throw "Refusing to overwrite an existing EXE: $stagedExecutable"
}
Copy-Item -LiteralPath $executablePath -Destination $stagedExecutable -ErrorAction Stop

$sourceExecutableHash = (Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash
$stagedExecutableHash = (Get-FileHash -LiteralPath $stagedExecutable -Algorithm SHA256).Hash
if ($sourceExecutableHash -ne $stagedExecutableHash) {
    throw "Hash mismatch after copying the executable"
}

foreach ($sourceFile in $sourceDatFiles) {
    $destination = Join-Path $runtimeRoot $sourceFile.Name
    if (Test-Path -LiteralPath $destination) {
        throw "Refusing to overwrite an existing staged file: $destination"
    }
    Copy-Item -LiteralPath $sourceFile.FullName -Destination $destination -ErrorAction Stop

    $sourceHash = (Get-FileHash -LiteralPath $sourceFile.FullName -Algorithm SHA256).Hash
    $destinationHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    if ($sourceHash -ne $destinationHash) {
        throw "Hash mismatch after copying $($sourceFile.Name)"
    }
    $destinationFile = Get-Item -LiteralPath $destination
    if ($sourceFile.Length -ne $destinationFile.Length) {
        throw "Size mismatch after copying $($sourceFile.Name)"
    }
    Write-Output ("STAGED {0} {1} bytes SHA256={2}" -f $sourceFile.Name, $destinationFile.Length, $destinationHash)
}

if (Test-Path -LiteralPath (Join-Path $runtimeRoot 'index.dat')) {
    throw 'Unexpected index.dat in the newly created runtime directory'
}

Write-Output ("STAGED {0} {1} bytes SHA256={2}" -f $executableItem.Name, $executableItem.Length, $stagedExecutableHash)
Write-Output ("RUNTIME_DIR {0}" -f $runtimeRoot)
Write-Output 'DATA_MODE copied-three-dat-files'
Write-Output 'WORKING_DIRECTORY isolated-runtime-directory'

$startParameters = @{
    FilePath = $stagedExecutable
    WorkingDirectory = $runtimeRoot
    PassThru = $true
}
if ($ArgumentList.Count -gt 0) {
    $startParameters.ArgumentList = $ArgumentList
}
if ($Wait) {
    $startParameters.Wait = $true
}

$process = Start-Process @startParameters
if ($Wait) {
    Write-Output ("EXIT_CODE {0}" -f $process.ExitCode)
} else {
    Write-Output ("PID {0}" -f $process.Id)
}

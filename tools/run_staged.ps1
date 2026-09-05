param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [string[]]$ArgumentList = @(),

    [switch]$Wait
)

$ErrorActionPreference = 'Stop'

$executablePath = (Resolve-Path -LiteralPath $Executable).Path
$executableItem = Get-Item -LiteralPath $executablePath
if ($executableItem.Extension -ine '.exe') {
    throw "Executable must be a .exe file: $executablePath"
}

$executableDirectory = $executableItem.Directory.FullName
$requiredDatNames = @(
    '6kinoko_a.dat',
    '6kinoko_b.dat',
    '6kinoko_c.dat'
)
foreach ($datName in $requiredDatNames) {
    $datPath = Join-Path $executableDirectory $datName
    if (-not (Test-Path -LiteralPath $datPath -PathType Leaf)) {
        throw "Required DAT is missing beside the executable: $datPath. Run stage_dat.ps1 after building."
    }
}

Write-Output ("EXECUTABLE {0}" -f $executablePath)
Write-Output 'DATA_MODE three-dat-files-already-beside-executable'
Write-Output 'WORKING_DIRECTORY unset'

$startParameters = @{
    FilePath = $executablePath
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

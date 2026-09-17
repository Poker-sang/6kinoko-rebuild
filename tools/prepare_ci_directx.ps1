[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Destination
)
$ErrorActionPreference = 'Stop'

# Official package linked by Microsoft Download Center, id=8109.
# This supplies the existing d3dx9_33 import, not a replacement game engine.
# Extract only; do not run DXSETUP or modify the runner's system directory.
$uri = 'https://download.microsoft.com/download/8/4/a/84a35bf1-dafe-4ae8-82af-ad2ae20b6b14/directx_Jun2010_redist.exe'
$Destination = [IO.Path]::GetFullPath($Destination)
New-Item -ItemType Directory -Force $Destination | Out-Null
$package = Join-Path $Destination 'directx_Jun2010_redist.exe'
$unpacked = Join-Path $Destination 'redist'
Invoke-WebRequest -Uri $uri -OutFile $package
$signature = Get-AuthenticodeSignature -FilePath $package
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation') {
    throw "DirectX package is not authenticated as Microsoft: $($signature.Status)"
}
Get-FileHash -Algorithm SHA256 $package | Format-List

$sevenZip = (Get-Command 7z.exe -ErrorAction Stop).Source
& $sevenZip x $package "-o$unpacked" -y | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'DirectX cabinet extraction failed.' }
$cabinets = @(Get-ChildItem $unpacked -Filter '*d3dx9_33_x86.cab' -Recurse)
if ($cabinets.Count -ne 1) { throw 'Expected exactly one x86 D3DX9_33 cabinet.' }
& "$env:SystemRoot/System32/expand.exe" $cabinets[0].FullName '-F:d3dx9_33.dll' $Destination
if ($LASTEXITCODE -ne 0) { throw 'D3DX9_33 extraction failed.' }
$dll = Join-Path $Destination 'd3dx9_33.dll'
if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) { throw 'D3DX9_33 runtime was not extracted.' }
Get-FileHash -Algorithm SHA256 $dll | Format-List

# A job-local search path is enough for tests and leaves packaged game assets
# untouched. No DAT, saves, downloaded Squirrel source or stub DLL is involved.
if ($env:GITHUB_PATH) { $Destination >> $env:GITHUB_PATH }
else { $env:PATH = "$Destination;$env:PATH" }

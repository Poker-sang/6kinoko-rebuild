param(
    [Parameter(Mandatory = $true)][string]$Tool,
    [Parameter(Mandatory = $true)][string]$Database,
    [string]$Arguments = '{}'
)

$ErrorActionPreference = 'Stop'
$toolArguments = ConvertFrom-Json -InputObject $Arguments -AsHashtable
if ($Database) { $toolArguments.database = $Database }
$payload = @{
    jsonrpc = '2.0'
    id = 1
    method = 'tools/call'
    params = @{ name = $Tool; arguments = $toolArguments }
} | ConvertTo-Json -Depth 30 -Compress
$response = Invoke-RestMethod 'http://127.0.0.1:13337/mcp' -Method Post `
    -Body $payload -ContentType 'application/json' -TimeoutSec 180
if ($response.error) { throw ($response.error | ConvertTo-Json -Compress) }
$response.result.content | Where-Object type -eq 'text' | ForEach-Object text
if ($response.result.isError) { exit 1 }

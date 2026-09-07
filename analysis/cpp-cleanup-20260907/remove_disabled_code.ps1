param(
    [Parameter(Mandatory = $true)][string]$Source,
    [switch]$Apply
)

$ErrorActionPreference = 'Stop'
$path = (Resolve-Path -LiteralPath $Source).Path
$content = [IO.File]::ReadAllText($path)
$directives = [regex]::Matches($content, '(?m)^[\t ]*#[\t ]*(if|ifdef|ifndef|else|elif|endif)\b[^\r\n]*(?:\r?\n|$)')
$blocks = [Collections.Generic.List[object]]::new()
$start = -1
$depth = 0
$alternative = $false
foreach ($directive in $directives) {
    $kind = $directive.Groups[1].Value
    if ($start -lt 0) {
        if ($directive.Value -match '^\s*#\s*if\s+0\s*$') {
            $start = $directive.Index
            $depth = 1
            $alternative = $false
        }
        continue
    }
    if ($kind -in 'if', 'ifdef', 'ifndef') { ++$depth }
    elseif ($kind -eq 'endif') {
        --$depth
        if ($depth -eq 0) {
            if (-not $alternative) {
                $blocks.Add(@{ Start = $start; Length = $directive.Index + $directive.Length - $start })
            }
            $start = -1
        }
    }
    elseif ($depth -eq 1) { $alternative = $true }
}
if ($start -ge 0) { throw 'Unterminated disabled preprocessor block' }
$removedLines = 0
for ($index = $blocks.Count - 1; $index -ge 0; --$index) {
    $block = $blocks[$index]
    $removedLines += [regex]::Matches($content.Substring($block.Start, $block.Length), '\n').Count
    $content = $content.Remove($block.Start, $block.Length)
}
if ($Apply) {
    [IO.File]::WriteAllText($path, $content, [Text.UTF8Encoding]::new($false))
}
[pscustomobject]@{ Blocks = $blocks.Count; RemovedLines = $removedLines; Applied = [bool]$Apply }

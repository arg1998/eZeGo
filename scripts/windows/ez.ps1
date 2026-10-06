# Windows side of the launcher (called by ez.cmd). Same commands as ./ez. Not yet run on Windows.
param([Parameter(Position = 0)][string]$Cmd = 'help', [Parameter(ValueFromRemainingArguments = $true)][string[]]$Rest)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..\..')
. (Join-Path $PSScriptRoot 'env.ps1')
if (-not $Rest) { $Rest = @() }
$first = if ($Rest.Count -gt 0) { $Rest[0] } else { $null }
$tail = if ($Rest.Count -gt 1) { $Rest[1..($Rest.Count - 1)] } else { @() }

switch ($Cmd) {
  'doctor'  { cmake -P scripts/doctor.cmake @Rest }
  'init'    { cmake -P scripts/init.cmake @Rest }
  'deps'    { cmake -P scripts/deps.cmake @Rest }
  'tools'   { cmake -P scripts/tools.cmake @Rest }
  'vscode'  { cmake -P scripts/vscode.cmake @Rest }
  'build'   { cmake --workflow --preset ($(if ($first) { $first } else { 'debug' })) }
  'test'    { ctest --preset ($(if ($first) { $first } else { 'debug' })) @tail }
  'check'   { cmake --workflow --preset check }
  'run'     { cmake -P scripts/run.cmake "--preset=$(if ($first) { $first } else { 'debug' })" -- @tail }
  'profile' { cmake -P scripts/profile.cmake @Rest }
  'clean'   { if (-not $first) { 'usage: ez clean <preset>'; exit 2 }; cmake -E rm -rf "build/$first" }
  default   { Get-Content (Join-Path $PSScriptRoot '..\..\ez') | Select-String -Pattern '^  [a-z]' | ForEach-Object { $_.Line } }
}
exit $LASTEXITCODE

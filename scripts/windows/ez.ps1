# Windows side of the launcher (called by ez.cmd). Same commands as ./ez. Not yet run on Windows.
# No param() block on purpose: the parameter binder rejects a bare `--`, which is how arguments
# are handed to the app (ez run debug -- --quit-after 5). $args passes everything through as typed.
$Cmd = if ($args.Count -gt 0) { $args[0] } else { 'help' }
[string[]]$Rest = @($args | Select-Object -Skip 1)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..\..')
. (Join-Path $PSScriptRoot 'env.ps1')
$first = if ($Rest.Count -gt 0) { $Rest[0] } else { $null }
[string[]]$tail = @($Rest | Select-Object -Skip 1)

switch ($Cmd) {
  'doctor'  { cmake -P scripts/doctor.cmake @Rest }
  'init'    { cmake -P scripts/init.cmake @Rest }
  'deps'    { cmake -P scripts/deps.cmake @Rest }
  'tools'   { cmake -P scripts/tools.cmake @Rest }
  'vscode'  { cmake -P scripts/vscode.cmake @Rest }
  'build'   { cmake --workflow --preset ($(if ($first) { $first } else { 'debug' })) }
  'test'    { ctest --preset ($(if ($first) { $first } else { 'debug' })) @tail }
  'check'   { cmake --workflow --preset check }
  'lint'    { cmake -P scripts/lint.cmake @Rest }
  'run'     { cmake -P scripts/run.cmake "--preset=$(if ($first) { $first } else { 'debug' })" -- @tail }
  'profile' { cmake -P scripts/profile.cmake @Rest }
  'clean'   { if (-not $first) { 'usage: ez clean <preset>'; exit 2 }; cmake -E rm -rf "build/$first" }
  default   { Get-Content (Join-Path $PSScriptRoot '..\..\ez') | Select-String -Pattern '^  [a-z]' | ForEach-Object { $_.Line } }
}
exit $LASTEXITCODE

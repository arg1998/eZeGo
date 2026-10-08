# Windows side of the launcher (called by ez.cmd): the same commands as ./ez, after loading the
# MSVC developer environment. Help text is scripts/help.cmake, shared with ./ez. Not yet run on
# Windows.
# No param() block on purpose: the parameter binder rejects a bare `--`, which is how arguments
# are handed to the app (ez run -- --app.quit_after_s=5). $args passes everything through as typed.
$Cmd = if ($args.Count -gt 0) { $args[0] } else { 'help' }
[string[]]$Rest = @($args | Select-Object -Skip 1)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..\..')
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  Write-Error 'ez: cmake was not found. Install it (see docs/getting-started.md), then run ez.cmd doctor.'
  exit 127
}
. (Join-Path $PSScriptRoot 'env.ps1')

$commands = 'doctor', 'init', 'deps', 'tools', 'vscode', 'build', 'run', 'clean', 'test', 'check', 'lint', 'bench', 'profile', 'help'
function Show-Help([string]$topic) {
  if ($topic -and $topic -notin $commands) { Write-Host "ez: no command '$topic'. commands: $commands"; exit 2 }
  cmake -P scripts/help.cmake $topic; exit $LASTEXITCODE
}
if ($Cmd -in 'help', '-h', '--help') { Show-Help ($(if ($Rest.Count -gt 0) { $Rest[0] } else { '' })) }
if ($Rest.Count -gt 0 -and $Rest[0] -in '-h', '--help' -and $Cmd -in $commands) { Show-Help $Cmd }

# A preset argument is optional for build, run and test: `ez test -L unit` means the debug tree.
$preset = 'debug'
[string[]]$tail = $Rest
if ($Rest.Count -gt 0 -and -not $Rest[0].StartsWith('-')) {
  $preset = $Rest[0]
  $tail = @($Rest | Select-Object -Skip 1)
}

switch ($Cmd) {
  'doctor'  { cmake -P scripts/doctor.cmake @Rest }
  'init'    { cmake -P scripts/init.cmake @Rest }
  'deps'    { cmake -P scripts/deps.cmake @Rest }
  'tools'   { cmake -P scripts/tools.cmake @Rest }
  'vscode'  { cmake -P scripts/vscode.cmake @Rest }
  'build'   {
    if ($tail.Count -eq 0) { cmake --workflow --preset $preset }
    else { cmake --preset $preset | Out-Null; if ($LASTEXITCODE -eq 0) { cmake --build --preset $preset @tail } }
  }
  'test'    {
    $tree = if ($preset -eq 'gpu') { 'debug' } else { $preset }   # a test preset on the debug tree
    cmake --workflow --preset $tree | Out-Null   # configure if needed, build; errors still print
    if ($LASTEXITCODE -eq 0) { ctest --preset $preset @tail }
  }
  'check'   { cmake --workflow --preset check }
  'lint'    { cmake -P scripts/lint.cmake @Rest }
  'bench'   { cmake -P scripts/bench.cmake @Rest }
  'run'     {
    if ($tail.Count -gt 0 -and $tail[0] -eq '--') { $tail = @($tail | Select-Object -Skip 1) }
    cmake -P scripts/run.cmake "--preset=$preset" -- @tail
  }
  'profile' { cmake -P scripts/profile.cmake @Rest }
  'clean'   {
    if ($Rest.Count -eq 0) { Write-Host 'usage: ez.cmd clean <preset>|all   (ez.cmd help clean)'; exit 2 }
    if ($Rest[0] -eq 'all') { cmake -E rm -rf build }
    elseif (-not (Test-Path "build/$($Rest[0])")) { Write-Host "ez: no tree build/$($Rest[0]) (ez.cmd help clean)"; exit 2 }
    else { cmake -E rm -rf "build/$($Rest[0])" }
  }
  default   {
    Write-Host "ez: unknown command '$Cmd'. commands: $commands"
    Write-Host 'try: ez.cmd help'
    exit 2
  }
}
exit $LASTEXITCODE

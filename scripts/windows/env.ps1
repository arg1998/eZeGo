# Load the MSVC developer environment (libraries + Windows SDK paths for clang-cl) into the
# current PowerShell session. Dot-source it:   . scripts\windows\env.ps1
# NOTE: written for the experiment but not yet run on Windows.
if ($env:VSCMD_VER) { return }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { Write-Error "Visual Studio Build Tools not found. Run: cmake -P scripts/doctor.cmake"; return }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
# VsDevCmd calls a bare vswhere.exe; without it on PATH it prints "'vswhere.exe' is not recognized".
$env:PATH = "$(Split-Path $vswhere);$env:PATH"
$dll = Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Import-Module $dll
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null

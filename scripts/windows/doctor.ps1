# Windows-only checks for scripts/doctor.cmake. Read-only. Output: LEVEL|name|detail|fix
# NOTE: written for the experiment but not yet run on Windows.
$ErrorActionPreference = 'SilentlyContinue'

$os = Get-CimInstance Win32_OperatingSystem
$build = [int]$os.BuildNumber
if ($build -ge 19041) { "OK|Windows|$($os.Caption) build $build|" }
else { "WARN|Windows|build $build (Windows 10 2004+ / 11 expected)|" }

if ([Environment]::Is64BitOperatingSystem) { "OK|architecture|x64|" } else { "ERROR|architecture|32-bit Windows is not supported|" }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = $null
if (Test-Path $vswhere) {
  $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
if ($vs) { "OK|MSVC build tools|$vs|" }
else { "ERROR|MSVC build tools|not found (clang-cl needs the MSVC libraries)|winget install Microsoft.VisualStudio.2022.BuildTools --override `"--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended`"" }

# The SDK registers under the 32-bit view; the 64-bit KitsRoot10 can point at a root that only
# holds side packages (GameInput, Application Verifier), so check both like vcvars does.
$sdk = $null
foreach ($key in 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots', 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots') {
  $root = (Get-ItemProperty $key -Name KitsRoot10).KitsRoot10
  if (-not $root) { continue }
  $ver = Get-ChildItem (Join-Path $root 'Include') -Directory |
    Where-Object { Test-Path (Join-Path $_.FullName 'um\Windows.h') } |
    Sort-Object { [version]$_.Name } | Select-Object -Last 1
  if ($ver) { $sdk = "$($ver.Name)  $($root.TrimEnd('\'))"; break }
}
if ($sdk) { "OK|Windows SDK|$sdk|" }
else { "ERROR|Windows SDK|not found|add the 'Windows 11 SDK' component in the Visual Studio Installer" }

$clang = Get-Command clang-cl -ErrorAction SilentlyContinue
if (-not $clang -and $vs) { $clang = Get-Item (Join-Path $vs 'VC\Tools\Llvm\x64\bin\clang-cl.exe') -ErrorAction SilentlyContinue }
if (-not $clang) { $clang = Get-Item (Join-Path $env:ProgramFiles 'LLVM\bin\clang-cl.exe') -ErrorAction SilentlyContinue }
if ($clang) { "OK|clang-cl|$($clang.Source)$($clang.FullName)|" }
else { "ERROR|clang-cl|not found|winget install LLVM.LLVM   (or add 'C++ Clang tools' in the Visual Studio Installer)" }

if ($env:VSCMD_VER) { "OK|developer environment|loaded (VS $env:VSCMD_VER)|" }
else { "WARN|developer environment|not loaded in this shell; ez.cmd loads it automatically|scripts\windows\env.ps1" }

"HINT|toolchain|winget install Kitware.CMake Ninja-build.Ninja LLVM.LLVM|"
"HINT|accelerators|winget install Mozilla.sccache|"
"HINT|clang_tools|winget install LLVM.LLVM|"
"HINT|clang_tidy|winget install LLVM.LLVM   (clang-tidy ships with LLVM; also in Visual Studio's 'C++ Clang tools')|"

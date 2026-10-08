#!/usr/bin/env sh
# macOS-only checks for scripts/doctor.cmake. Read-only. Output: LEVEL|name|detail|fix
# NOTE: written for the experiment but not yet run on a Mac.

arch=$(uname -m)
if [ "$arch" = "arm64" ]; then
  echo "OK|architecture|arm64 (Apple Silicon)|"
else
  echo "ERROR|architecture|$arch: only Apple Silicon is supported|"
fi

ver=$(sw_vers -productVersion 2>/dev/null)
major=${ver%%.*}
if [ -n "$major" ] && [ "$major" -ge 13 ] 2>/dev/null; then
  echo "OK|macOS|$ver|"
else
  echo "WARN|macOS|$ver (deployment target is 13.0; older versions are untested)|"
fi

if xcode-select -p >/dev/null 2>&1; then
  echo "OK|Xcode Command Line Tools|$(xcode-select -p)|"
else
  echo "ERROR|Xcode Command Line Tools|not installed (provides Apple Clang and the macOS SDK)|xcode-select --install"
fi

if command -v brew >/dev/null 2>&1; then
  echo "HINT|toolchain|brew install cmake ninja|"
  echo "HINT|accelerators|brew install ccache|"
  echo "HINT|clang_tools|brew install llvm   (for clangd / clang-format; Apple Clang stays the compiler)|"
  echo "HINT|clang_tidy|brew install llvm   (clang-tidy is found in \$(brew --prefix llvm)/bin; Apple Clang stays the compiler)|"
  echo "OK|Homebrew|$(brew --prefix)|"
else
  echo "HINT|toolchain|install CMake and Ninja (https://cmake.org/download, https://github.com/ninja-build/ninja/releases)|"
  echo "WARN|Homebrew|not found; the fix commands assume it|/bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
fi

#!/usr/bin/env sh
# Linux-only checks for scripts/doctor.cmake (B-5, B-6). Read-only: installs nothing.
# Output protocol, one finding per line:  LEVEL|name|detail|fix     (LEVEL = OK, WARN, ERROR)

# ---------------------------------------------------------------- package manager
if command -v apt-get >/dev/null 2>&1; then PM=apt; INSTALL="sudo apt install"
elif command -v dnf >/dev/null 2>&1; then PM=dnf; INSTALL="sudo dnf install"
elif command -v pacman >/dev/null 2>&1; then PM=pacman; INSTALL="sudo pacman -S --needed"
elif command -v zypper >/dev/null 2>&1; then PM=zypper; INSTALL="sudo zypper install"
else PM=unknown; INSTALL="install"
fi
. /etc/os-release 2>/dev/null
echo "OK|distribution|${PRETTY_NAME:-unknown} (package manager: $PM)|"

# pkg(apt dnf pacman) -> name for this distro
pkg() { case $PM in apt) echo "$1";; dnf|zypper) echo "$2";; pacman) echo "$3";; *) echo "$1";; esac; }

# ---------------------------------------------------------------- compiler toolchain hints
# (doctor.cmake checks the tools themselves; these are the install lines it prints)
echo "HINT|toolchain|$INSTALL $(pkg 'clang lld ninja-build cmake' 'clang lld ninja-build cmake' 'clang lld ninja cmake')|"
echo "HINT|accelerators|$INSTALL $(pkg 'mold ccache' 'mold ccache' 'mold ccache')|"
echo "HINT|clang_tools|$INSTALL $(pkg 'clangd-@MAJOR@ clang-format-@MAJOR@' 'clang-tools-extra' 'clang')|"
echo "HINT|clang_tidy|$INSTALL $(pkg 'clang-tidy-@MAJOR@' 'clang-tools-extra' 'clang')|"

if ! command -v pkg-config >/dev/null 2>&1; then
  echo "ERROR|pkg-config|not found (needed to locate windowing headers)|$INSTALL $(pkg pkg-config pkgconf-pkg-config pkgconf)"
  exit 0
fi

# check LEVEL label "pc modules" apt dnf pacman
check() {
  level=$1; label=$2; mods=$3
  missing=""
  for m in $mods; do pkg-config --exists "$m" 2>/dev/null || missing="$missing $m"; done
  if [ -z "$missing" ]; then
    echo "OK|$label|$(echo $mods | tr ' ' ',')|"
  else
    echo "$level|$label|missing:$missing|$INSTALL $(pkg "$4" "$5" "$6")"
  fi
}

# ---------------------------------------------------------------- app build (GLFW: X11 + Wayland)
check ERROR "X11 headers (GLFW)" "x11 xrandr xinerama xcursor xi" \
  "libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev" \
  "libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel" \
  "libx11 libxrandr libxinerama libxcursor libxi"
check ERROR "Wayland headers (GLFW)" "wayland-client wayland-cursor wayland-egl xkbcommon wayland-protocols" \
  "libwayland-dev libxkbcommon-dev wayland-protocols" \
  "wayland-devel libxkbcommon-devel wayland-protocols-devel" \
  "wayland libxkbcommon wayland-protocols"
if command -v wayland-scanner >/dev/null 2>&1; then
  echo "OK|wayland-scanner|$(command -v wayland-scanner)|"
else
  echo "ERROR|wayland-scanner|not found (GLFW generates Wayland protocol code)|$INSTALL $(pkg libwayland-bin wayland-devel wayland)"
fi

# ---------------------------------------------------------------- tool source builds (warnings)
check WARN "Tracy GUI source build" "libcurl freetype2 egl libffi" \
  "libcurl4-openssl-dev libfreetype-dev libegl-dev libffi-dev" \
  "libcurl-devel freetype-devel mesa-libEGL-devel libffi-devel" \
  "curl freetype2 libglvnd libffi"

# ---------------------------------------------------------------- device access (warnings)
groups_now=$(id -Gn 2>/dev/null)
serial_group=dialout
getent group uucp >/dev/null 2>&1 && ! getent group dialout >/dev/null 2>&1 && serial_group=uucp
case " $groups_now " in
  *" $serial_group "*) echo "OK|serial port access|member of '$serial_group'|" ;;
  *) echo "WARN|serial port access|not in '$serial_group'; USB-serial fixtures (Arduino/ESP32) will be unreachable|sudo usermod -aG $serial_group $USER   (then log out and back in)" ;;
esac

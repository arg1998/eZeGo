# Getting started

## 1. Clone and run `init`

```bash
git clone <repo> && cd eZeGo
./ez init                     # Windows: ez.cmd init      Plain CMake: cmake -P scripts/init.cmake
```

`init` does, in order, and is safe to re-run:

1. **doctor (system part)**: checks the compiler, Ninja, CMake, git and the OS packages GLFW needs.
   It installs nothing. Every problem comes with the exact install command for your OS.
2. **deps**: shallow-fetches each library in `dependencies.json` at its pinned commit into
   `third_party/_src/` (a few seconds, no history).
3. **tools**: installs the pinned Tracy profiler. If a verified prebuilt exists for your OS and you
   are at a terminal, it asks: download it (seconds) or build from source (about a minute).
   A tool that fails to install is a warning; it never blocks building the app.
4. **VS Code**: writes `eZeGo.code-workspace` with the paths it found.
5. **configure** the `debug` preset.

Options: `--tools=prebuilt|source|skip`, `--preset=<name>`, `--ignore-doctor`.

### If doctor reports errors

Install what it lists and run `init` again. On Ubuntu/Debian the typical first-time list is:

```bash
sudo apt install clang lld ninja-build cmake mold ccache clangd clang-format \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libwayland-dev libxkbcommon-dev wayland-protocols
```

Only for building Tracy from source (not needed with the prebuilt):
`libcurl4-openssl-dev libfreetype-dev libegl-dev libffi-dev`.

## 2. Build, test, run

```bash
./ez build              # cmake --workflow --preset debug
./ez test               # ctest --preset debug
./ez run                # build, then run build/debug/bin/ezego
./ez run debug -- --app.quit_after_s=5   # arguments after -- go to the app
./ez help test                        # each command has a page: presets, options, examples
```

The app is a Dear ImGui "Hello, eZeGo" window showing the build mode, platform, compiler and
frame time. `ezego --version` prints the version without opening a window.

## 3. Check everything at any time

```bash
./ez doctor             # toolchain, system packages, dependency pins, tools, build trees
```

Exit code 0 means ready. It is read-only, so it is safe in CI and in scripts.

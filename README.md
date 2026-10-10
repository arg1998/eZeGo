# eZeGo

> [!CAUTION]
> Early development. The foundations (build system, core, platform layer) are being built;
> the application itself is currently one window with eZeGo's own title bar and a Dear ImGui "hello world" panel.

eZeGo is a high-performance, low-latency show-control and visualization application for stage
lighting and other show machines (fog, lasers, motors), driving physical fixtures in a venue or
virtual ones inside a game engine. It aims for **progressive complexity**: anyone can start in
minutes, and experts can go as deep as they want. Licensed MIT; proprietary extensions go through
the plugin SDK.

## Quick start

```bash
git clone <repo> && cd eZeGo
./ez init       # check the machine, fetch pinned dependencies, install Tracy, configure
./ez build      # build the debug preset
./ez test       # run the tests
./ez run        # run the app
./ez help       # every command; ./ez help <command> for one in detail
```

Windows: `ez.cmd` with the same words. Every command is a thin alias for plain CMake, e.g.
`./ez build` is `cmake --workflow --preset debug` (see `./ez help`).

If `init` reports missing system packages, it prints the exact install command for your OS.
Supported: Linux (modern distros, x86-64), macOS 13+ on Apple Silicon, Windows 10/11 x64.
Compiler: Clang on every platform (Apple Clang on macOS, clang-cl on Windows).

## Build modes

| Preset | Use |
|---|---|
| `debug` | day-to-day development: assertions, all logs |
| `profile` | measuring: release code generation + Tracy (`./ez profile` opens Tracy and runs the app) |
| `release` | what ships: optimized, static C++ runtime, symbols split out |
| `asan`, `tsan` | debug + Address/UB or Thread sanitizer |

Each preset has its own build tree under `build/`, so switching modes never needs a clean.

## Repository layout

```
specs/                design intent and decisions (start at specs/README.md)
docs/                 contributor how-to: build, profiling, dependencies, tools, VS Code
modules.cmake         the module table: every module, its layer and dependencies
src/ez/<module>/      one directory per module: base, cvars, log, metrics, app (specs/code-organization.md)
src/apps/ezego/       the executable: main() only
tests/<module>/       doctest suites per module; tests/bench/ micro-benchmarks; tests/support/ helpers
assets/               fonts and other runtime files (copied next to the binary)
dependencies.json     every third-party library and dev tool, pinned to a commit
CMakePresets.json     the build modes
cmake/                build logic: modes, toolchain, dependency check, target helpers
third_party/*.cmake   how each dependency is built
tools/*.cmake         dev-tool recipes (Tracy)
scripts/              doctor, init, deps, tools, lint, bench, profile, run, vscode (+ per-OS parts)
ez, ez.cmd            optional launcher
```

## Documentation

| | |
|---|---|
| [Getting started](docs/getting-started.md) | clone to running app; what `init` does |
| [Build modes](docs/build-modes.md) | presets, personal variants, how modes work, measured build times |
| [Profiling](docs/profiling.md) | Tracy, headless captures, instrumenting code |
| [Dependencies](docs/dependencies.md) | the manifest, bumping a pin, adding a library |
| [Dev tools](docs/tools.md) | Tracy install, prebuilt vs source, provenance |
| [VS Code](docs/vscode.md) | everything without a terminal |
| [Lint](docs/lint.md) | naming rules, `ez lint`, `ez check`, suppressions, tool versions |
| [Testing](docs/testing.md) | `ez test`, `ez bench`, labels, writing a test, subprocess and temp-dir helpers |
| [Cvars](docs/cvars.md) | declaring, reading and setting runtime variables |
| [Logging](docs/logging.md) | log macros, levels, `--log=`, file logging, seams |
| [Design specs](specs/README.md) | philosophy, architecture, threading, plugins, linking, observability, UI |

## Platform status

| | Linux | macOS | Windows |
|---|---|---|---|
| Build scripts and presets | verified | written, not yet run | written, not yet run |
| Tracy prebuilt / source build | verified | prebuilt listed | prebuilt listed |

## License

[MIT](LICENSE)

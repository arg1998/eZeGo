# Profiling

Deep profiling uses [Tracy](https://github.com/wolfpld/tracy) in the `profile` preset
(specs/observability.md O-4). The always-on metrics tier (O-2) is designed but not implemented yet.

## One command

```bash
./ez profile                         # cmake -P scripts/profile.cmake
./ez profile -- --log=all:debug     # pass arguments to the app
```

This builds `profile`, opens the pinned Tracy GUI connected to `127.0.0.1`, and runs the app.
Tracy runs **on demand**: the app records only while Tracy is connected, so an idle profile
build stays light.

## Headless capture (no GUI: CI, a show machine, a bug report)

```bash
./ez profile --capture=run.tracy --seconds=10
"$(cmake -P scripts/tools.cmake --print=tracy:profiler)" run.tracy      # open it later
"$(cmake -P scripts/tools.cmake --print=tracy:csvexport)" run.tracy     # zone statistics as CSV
```

## What is instrumented

| In Tracy | Comes from |
|---|---|
| Frame marks, zone `Main Loop`, plot `frame ms` | `src/ez/app/app.cpp` |
| Zones `Input`, `Begin Frame`, `Render Frame` | `src/ez/app/detail/shell.cpp` |
| Messages, coloured by level | every log line, through the logger's profiler hook |
| Memory pool `untracked/operator-new` | profile-only `operator new` hooks (`src/ez/metrics/new_hooks.cpp`) |

Instrument code through `ez/metrics/profiler.hpp` only: `EZ_PROF_ZONE("name")`,
`EZ_PROF_FUNCTION()`, `EZ_PROF_FRAME()`, `EZ_PROF_FRAME_START/END("series")`,
`EZ_PROF_PLOT(name, value)`. They compile to nothing outside the `profile` preset, and Tracy's
headers are not on the include path there, so nothing can use Tracy by accident. Inside `profile`
they are inert until `ez::metrics::start_profiler()` and after `stop_profiler()`, so instrumented
code may run at any time.

Startup order (manual lifetime, observability.md §3): settings, `start_profiler()`, `log::init()`,
then everything else; the reverse on shutdown. `src/apps/ezego/main.cpp` shows it.

## Personal settings

`scripts/profile.user.cmake` (git-ignored):

```cmake
set(EZ_TRACY_ADDRESS 192.168.1.20)   # app built with -DEZ_TRACY_REMOTE=ON on another machine
set(EZ_TRACY_PORT 8086)
```

## Platform notes

- macOS: Tracy has no OpenGL GPU zones on Apple platforms (it disables them; GL timestamps are
  unreliable there). CPU zones work.
- Linux: the prebuilt GUI is an AppImage; `tools` extracts it once so it runs without FUSE.

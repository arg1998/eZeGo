# Profiling

Deep profiling uses [Tracy](https://github.com/wolfpld/tracy) in the `profile` preset
(specs/observability.md O-4). The always-on metrics tier (O-2) is designed but not implemented yet.

## One command

```bash
./ez profile                         # cmake -P scripts/profile.cmake
./ez profile -- --quit-after 30      # pass arguments to the app
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
| Frame marks, zone `Main Loop` | `src/application/main.cpp` |
| Zones `Input Processing`, `Begin Frame Generation`, `Render Frame` | `src/application/application.cpp` |
| Plot `frame ms` | main loop |
| Messages (colored by level) | every log line (`src/core/logger`) |
| Memory pool `ez/general` | `ezAllocate` / `ezFree` (`src/core/memory`) |
| Memory pool `untracked/operator-new` | profile-only `operator new` hooks (`src/core/memory/new_hooks.cpp`) |

Instrument code through `src/core/profiler/profiler.hpp` only: `EZ_PROFILE_ZONE("name")`,
`EZ_PROFILE_FUNCTION()`, `EZ_PROFILE_FRAME()`, `EZ_PROFILE_PLOT(name, value)`. They compile to
nothing outside the `profile` preset, and Tracy's headers are not even on the include path there,
so nothing can use Tracy by accident.

Startup order matters (manual lifetime): `startProfiler()` → `initLoggingSystem()` → platform →
application, and the reverse on shutdown. A thread must not register with Tracy before the
profiler starts.

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

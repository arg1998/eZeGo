# Build modes

Each mode is a CMake preset with **its own build tree** (`build/<preset>/`). Switching never
needs a cache wipe; switching to a mode that is already built rebuilds nothing.

| Preset | For | Optimization | Asserts | Logs | Tracy | Memory events |
|---|---|---|---|---|---|---|
| `debug` | writing and debugging code | first-party off, third-party `-O2` | on | all | off | off |
| `profile` | measuring | **identical to release** | off | info+ | on | on (+ `operator new` hook) |
| `release` | shipping | `-O3` | off | warnings+ | compiled out | off |
| `asan` | memory errors | debug + Address/UB sanitizers | on | all | off | off |
| `tsan` | data races (not on Windows) | debug + Thread sanitizer | on | all | off | off |

Every mode keeps frame pointers and full symbols. `release` splits symbols into `ezego.debug`
next to the binary (Linux) and links the C++ runtime statically.

## Commands

```bash
./ez build profile                # cmake --workflow --preset profile
./ez test profile                 # ctest --preset profile
./ez test debug -L unit           # only one label: unit | scenario | smoke
ctest --test-dir build/debug -L gpu   # GUI smoke test (needs a display), excluded by default
./ez check                        # pre-merge gate: configure + build + test debug
./ez clean profile                # delete build/profile
```

## Personal variants

Make your own presets in `CMakeUserPresets.json` (git-ignored). Example: debug with Tracy on.

```json
{
  "version": 6,
  "configurePresets": [
    { "name": "debug-tracy", "inherits": "debug", "cacheVariables": { "EZ_FORCE_PROFILER": "ON" } }
  ],
  "buildPresets": [ { "name": "debug-tracy", "configurePreset": "debug-tracy" } ]
}
```

Other switches: `EZ_OPTIMIZE_THIRD_PARTY=OFF` (fully unoptimized debug), `EZ_TRACY_REMOTE=ON`
(profile a show machine over the network), `EZ_COMPILER_CACHE=OFF`, `EZ_ALLOW_GCC=ON` (unsupported).

## How it works

- A preset sets one project variable, `EZ_MODE`. `cmake/modes.cmake` derives everything else on
  every configure, so nothing mode-dependent can go stale in the cache.
- Code checks features (`EZ_ASSERTS`, `EZ_PROFILER`, `EZ_MEM_TRACE`, `EZ_LOG_LEVEL`), never the mode.
- `cmake/toolchain.cmake` picks the compiler and linker per OS (Clang + mold on Linux, Apple Clang
  on macOS, clang-cl + lld-link on Windows) and a compiler cache if one is installed.

## Measured on the reference Linux machine (2026-10-05, 64 cores, Clang 19, mold, ccache)

| | Time |
|---|---|
| Clean build of one mode, cold | 11–14 s |
| Clean build of one mode, warm compiler cache | 2.5 s |
| Switch to an already-built mode | 0.02 s |
| Edit one `.cpp`, rebuild debug | 0.1 s |

# eZeGo — Design Documents

This folder (`specs/`, formerly `.agent/`) holds the design intent for eZeGo. Start here. How-to pages for contributors are in [`../docs/`](../docs/).

## Documents

| Document | Covers | Status |
|---|---|---|
| [`things I want.md`](./things%20I%20want.md) | The original wishlist; source of intent | Raw notes |
| [`philosophy.md`](./philosophy.md) | Mission, engineering tenets, metrics, definition of done | Living |
| [`application-architecture.md`](./application-architecture.md) | Layers, frame pipeline, platform layer, capabilities | Living |
| [`threading-and-timing.md`](./threading-and-timing.md) | Timing domains, clocks, lock-free handoffs | Living, working hypothesis |
| [`plugins.md`](./plugins.md) | Native plugin system over a C ABI | Living, proposal |
| [`build-system.md`](./build-system.md) | Build, modes, commands, dependency manifest, `doctor` | **Accepted and implemented 2026-10-05** |
| [`linking.md`](./linking.md) | Static vs dynamic linking, runtimes, plugin isolation | **Proposed 2026-10-04** |
| [`observability.md`](./observability.md) | Profile build, always-on telemetry, memory tracking | **Proposed 2026-10-04** |
| [`ui-system.md`](./ui-system.md) | UI API boundary: immediate mode, own vocabulary, widgets / canvas / viewport layers | **Proposed 2026-10-05** |
| [`logging.md`](./logging.md) | Log API, levels, categories, per-thread rings and log thread, sinks, runtime control, costs | **Proposed 2026-10-08, not implemented** |
| [`cvars.md`](./cvars.md) | Runtime variables: one registry, zero-cost reads, mutability, validation, persistence, tiers and developer mode, console and panel | **Accepted and implemented 2026-10-08** |
| [`naming.md`](./naming.md) | Naming and namespaces: three shapes, prefixes and suffixes, files, registry names, the C SDK mapping, enforcement by clang-tidy and a lint script | **Accepted and enforced 2026-10-08** |
| [`code-organization.md`](./code-organization.md) | The repository tree, the inside of a module, the module table, the module inventory, how the tree grows | **Accepted 2026-10-08, being applied** |
| [`testing.md`](./testing.md) | Every kind of test, the phased rollout, doctest plus small purpose harnesses, the developer contract, fakes, hardware tiers; CI and reference machines deferred | **Accepted 2026-10-08, phase 1 implemented** |
| [`base.md`](./base.md) | The base layer: detection, types, macros, assertion seam, fixed string, stable hash, module enum | **Accepted and implemented 2026-10-08** |

## Decision register

Status values: **Proposed** (written up with trade-offs, awaiting confirmation), **Accepted** (confirmed by Amir), **Rejected**, **Superseded**. Each row links to the full reasoning.

### Build system — [`build-system.md`](./build-system.md)

| ID | Decision | Status |
|---|---|---|
| B-1 | Keep CMake + Ninja; minimum CMake 3.28 | Accepted 2026-10-05 |
| B-2 | A mode is a CMake preset with its own build tree; no custom build type | Accepted 2026-10-05 |
| B-3 | Three modes (`debug`, `profile`, `release`) plus sanitizer variants; profile uses release code generation | Accepted 2026-10-05 |
| B-4 | Canonical commands are plain CMake; optional `ez` launcher maps short words onto them | Accepted 2026-10-05 |
| B-5 | `scripts/` with portable CMake entry points and `linux/`, `macos/`, `windows/` for OS-specific parts only | Accepted 2026-10-05 |
| B-6 | Read-only `doctor`; idempotent `init`; every configure re-checks dependencies | Accepted 2026-10-05 |
| B-7 | One manifest (`dependencies.json`), explicit shallow git fetch of the pinned commit with no history, no submodules, no network during configure or build | Accepted 2026-10-05 |
| B-8 | We own the build description of every dependency | Accepted 2026-10-05 |
| B-9 | One static-library target per module; the build graph enforces the layer rule | Accepted 2026-10-05 |
| B-10 | Fast-build measures and build-time budgets | Accepted 2026-10-05 |
| B-11 | Clang on all platforms: upstream Clang, Apple Clang, `clang-cl` | Accepted 2026-10-05 |
| B-12 | Tests inside the main project, run through `ctest` presets with labels | Accepted 2026-10-05 |
| B-13 | VS Code as a complete UI: tracked tasks/launch/extensions with no machine paths; `init` generates a workspace file with discovered paths | Accepted 2026-10-05 |
| B-14 | Dev tools pinned in the manifest; verified prebuilt if available, else built from source; installed per user with a provenance file | Accepted 2026-10-05 |
| B-15 | Contributor how-to documentation lives in `docs/`, separate from design intent in `specs/` | Accepted 2026-10-05 |

### Linking — [`linking.md`](./linking.md)

| ID | Decision | Status |
|---|---|---|
| L-1 | Everything built from source links statically into one executable, in every mode | Proposed |
| L-2 | Only system boundaries stay dynamic (OS, GPU driver, windowing, audio) | Proposed |
| L-3 | Static C runtime on Windows; static C++ runtime on Linux releases; system runtime on macOS | Proposed |
| L-4 | Plugins are the only first-class dynamic code; host exports no symbols | Proposed |
| L-5 | Stateful libraries exist once per process, in the host | Proposed |
| L-6 | Explicit registration; no static-initializer registration | Proposed |

### Observability — [`observability.md`](./observability.md)

| ID | Decision | Status |
|---|---|---|
| O-1 | Deep profiling and always-on telemetry are separate mechanisms behind one facade | Proposed |
| O-2 | Always-on tier: fixed-size metrics with a per-frame cost budget | Proposed |
| O-3 | Telemetry is local; nothing leaves the machine unless the user exports it | Proposed |
| O-4 | Profile build = release code generation + Tracy behind the facade; Tracy never ships | Proposed |
| O-5 | Memory tracked in three rings; catch-all hooks only in the profile build | Proposed |
| O-6 | Frame pointers and symbols in every mode so external profilers work | Proposed |
| O-7 | Regression gate from headless scenario metrics | Proposed, deferred |

### UI system — [`ui-system.md`](./ui-system.md)

| ID | Decision | Status |
|---|---|---|
| U-1 | Application and plugins never call Dear ImGui; everything goes through `ez_ui`, enforced by the build graph | Proposed |
| U-2 | Immediate mode remains the paradigm for the application and plugins | Proposed |
| U-3 | eZeGo's own vocabulary, not a one-to-one ImGui veneer | Proposed |
| U-4 | Three layers: widgets and layout, canvas, viewport | Proposed |
| U-5 | High-level widgets expressible as data where cheap; not a retained framework | Proposed |
| U-6 | A viewport is a render target; scene and gizmo logic live in render and editor-tool systems | Proposed |
| U-7 | First-party `ui_native` escape hatch, never for plugins or stabilized screens; design to be revisited | Proposed |
| U-8 | Widget vocabulary designed after the UX designs are provided | Proposed |

### Logging — [`logging.md`](./logging.md)

| ID | Decision | Status |
|---|---|---|
| LG-1 | One API: `EZ_LOG_<LEVEL>(category, "literal", args...)`, flood guards, and a realtime variant; printf-style; a macro over a variadic template so argument types are known | Proposed |
| LG-2 | Caller rules enforced at compile time: literal format string, plain-value arguments, single evaluation, truncation; they make the implementation swappable | Proposed |
| LG-3 | Six levels defined by audience and volume; logs are events, metrics are rates; Trace is the only per-frame level; release floor and runtime default are Info | Proposed; the release floor accepted 2026-10-08 |
| LG-4 | Categories from a central compile-time table plus a runtime range for plugins; a runtime level per category, `all` and prefix matching | Proposed |
| LG-5 | Asynchronous: one SPSC byte ring per thread with variable-length records, a preallocated pool for foreign threads, one low-priority log thread draining periodically; no allocation after init, no I/O on callers, drop-and-count when full | Proposed |
| LG-6 | First implementation formats at the call site; deferred formatting is a later swap behind the same API; profile builds always format at the call site for Tracy | Proposed |
| LG-7 | Sinks: stderr never stdout; file optional and off by default with periodic flush and immediate flush on Warn+; history ring; Tracy messages in profile; synchronous console when a debugger is attached | Proposed |
| LG-8 | Record fields fixed: ticks, frame, level, category, thread, file, line, payload; encoding free to change | Proposed |
| LG-9 | Sizes: build-time hard cap `EZ_LOG_MAX_LINE`, runtime `log.max_line` (512 default), ring and history sizes read at init | Proposed |
| LG-10 | Runtime control through one mechanism with four doors: defaults < settings file < environment `EZ_LOG` < command line `--log` < in-app console | Proposed |
| LG-11 | Fatal is synchronous and routes to the crash reporter; the crash path reads rings directly; start after memory and profiler, shut down in reverse | Proposed |
| LG-12 | Plugins log printf-style through the host table into their own `plugin.<id>` category | Proposed |

### Runtime variables — [`cvars.md`](./cvars.md)

| ID | Decision | Status |
|---|---|---|
| CV-1 | One registry of typed application settings with metadata; command line, environment, file, console, panel, reports and docs are views over it | Accepted and implemented 2026-10-08 |
| CV-2 | A cvar is a `constinit` static object with its value at offset 0; a read is one relaxed load at a fixed address; metadata is reached from the object only on cold paths | Accepted and implemented 2026-10-08 |
| CV-3 | Metadata: name with the module as first segment, type, default, range, mutability, tier, flags, mandatory help, aliases | Accepted and implemented 2026-10-08 |
| CV-4 | Mutability `Const` / `Startup` / `Live`; `Startup` locked after init, editable in the panel as a persisted value marked "after restart" | Accepted and implemented 2026-10-08 |
| CV-5 | Main thread is the only writer; console and panel writes are queued and applied at the frame boundary; a generation counter per cvar; no callbacks out of the registry | Accepted and implemented 2026-10-08 |
| CV-6 | Scalars are relaxed atomics readable from any thread; strings are `Startup` or main-thread only | Accepted and implemented 2026-10-08 |
| CV-7 | Validation: startup errors are collected and fail fast with a readable message and `--reset-settings`; runtime errors are rejected, the previous value kept, a warning shown; unknown names are preserved, not errors | Accepted and implemented 2026-10-08 |
| CV-8 | Precedence at startup defaults < file < environment < command line; the user wins afterwards; provenance stored per cvar and listed in reports | Accepted and implemented 2026-10-08 |
| CV-9 | Persist only explicit overrides, flat `name = value` text file per user, atomic debounced write off the frame thread, aliases for renames | Accepted and implemented 2026-10-08 |
| CV-10 | Tiers `User` / `Advanced` / `Developer` / `Hidden` control visibility only; developer mode is the cvar `app.developer`, off by default, user-enabled | Accepted and implemented 2026-10-08 |
| CV-11 | Explicit per-module registration; the first name segment must exist in the central module table shared with log categories; self-checks at registration | Accepted and implemented 2026-10-08 |
| CV-12 | Plugins declare cvars through the host table under `plugin.<id>.`, removed on unload, persisted values retained | Accepted and implemented 2026-10-08 |
| CV-13 | Application configuration only; project data stays in the project model; a second instance of the engine is the path if project settings need it | Accepted 2026-10-08; whether project settings reuse the engine is deferred |
| CV-14 | `--name=value`, `EZ_NAME` and `--help` are generated from the registry; curated shorthands sit on top | Accepted and implemented 2026-10-08 |

### Naming — [`naming.md`](./naming.md)

| ID | Decision | Status |
|---|---|---|
| N-1 | Three shapes: `lower_snake_case` for everything that is not a type or macro, `PascalCase` for types, `UPPER_SNAKE_CASE` for macros and C constants | Accepted 2026-10-08, enforced |
| N-2 | Namespace path equals directory path under `src/ez/`, with `base/` mapping to `ez` itself (CO-9); at most `module::sub::detail`; no `using namespace` | Accepted 2026-10-08, enforced |
| N-3 | One module name in six places: directory, namespace, target, log category, cvar prefix, memory tag, all checked against the central module table | Accepted 2026-10-08, enforced |
| N-4 | Types `PascalCase`; `enum class` with explicit type; acronyms as words; the primitive aliases are the one lowercase exception; `i32` replaces `s32` | Accepted 2026-10-08, enforced |
| N-5 | Functions `lower_snake_case`; verbs for actions, nouns for accessors, `set_` for mutators, `is_`/`has_` for predicates; fixed lifecycle verb pairs; no module name inside the identifier | Accepted 2026-10-08, enforced |
| N-6 | Public members plain; private members trailing `_`; mutable globals `g_`; `thread_local` `t_`; cvars `cv_`; constants plain; unit suffixes | Accepted 2026-10-08, enforced |
| N-7 | Macros `EZ_` and `UPPER_SNAKE_CASE`; feature macros always 0 or 1 with `-Wundef`; no reserved identifiers | Accepted 2026-10-08, enforced |
| N-8 | Files `lower_snake_case`; `.hpp` for C++, `.h` for C; platform suffixes `_linux` `_macos` `_windows` `_posix`; tests `<topic>_test.cpp`; `#pragma once`; includes rooted at `src/` | Accepted 2026-10-08, enforced |
| N-9 | Registry names: dotted lowercase segments, module first, one separator; memory tags move from `/` to `.` | Accepted 2026-10-08, enforced |
| N-10 | The C SDK is a mechanical mapping of the C++ name: `ez_` + path, `EZ_` for constants, no `_t` | Accepted 2026-10-08, enforced |
| N-11 | CMake: `ez_<module>` targets with `ez::` aliases, `ez_<verb>` functions, `EZ_` cache variables, upstream names for third-party | Accepted 2026-10-08, enforced |
| N-12 | American spelling; a fixed abbreviation list; acronyms as words; positive names | Accepted 2026-10-08, enforced |
| N-13 | Enforcement: `clang-tidy` through `clangd` inline and in `check`, compiler warnings, a CMake lint script, `clang-format`; warnings locally, errors in `check` and CI; pinned tool versions | Accepted 2026-10-08, enforced |

### Code organization — [`code-organization.md`](./code-organization.md)

| ID | Decision | Status |
|---|---|---|
| CO-1 | One directory is one module, one namespace and one static library, flat under `src/ez/`; the layer is a table column, never a path segment | Accepted 2026-10-08 |
| CO-2 | Dependencies declared per module and checked at configure time: same or lower layer only, no cycles | Accepted 2026-10-08 |
| CO-3 | Public API is a module's top-level headers; `detail/` is private and enforced by lint | Accepted 2026-10-08 |
| CO-4 | Platform code by file suffix, in place, selected by CMake | Accepted 2026-10-08 |
| CO-5 | Tests mirror the tree under `tests/<module>/` | Accepted 2026-10-08 |
| CO-6 | Executables contain only `main`, under `src/apps/`; first-party utilities under `src/tools/` | Accepted 2026-10-08 |
| CO-7 | The SDK lives at `sdk/` outside `src/`; first-party plugins under `plugins/` are built against it | Accepted 2026-10-08 |
| CO-8 | `modules.cmake` is the single module table; it creates targets, checks layers and generates `modules.gen.hpp` for log categories, cvar prefixes and memory tags | Accepted 2026-10-08 |
| CO-9 | `src/ez/base/` maps to namespace `ez` itself, the one exception to N-2 | Accepted 2026-10-08 |

### Base layer — [`base.md`](./base.md)

| ID | Decision | Status |
|---|---|---|
| BA-1 | Base is cheap to include, depends on nothing, uses namespace `ez`, grows only when a second module needs something | Accepted and implemented 2026-10-08 |
| BA-2 | Contents: detection, build switches, types, macros, debug assert, fixed string, stable hash, module enum | Accepted and implemented 2026-10-08 |
| BA-3 | Detection macros always 0 or 1, in a C header shared with the SDK | Accepted and implemented 2026-10-08 |
| BA-4 | The debug assertion has a replaceable handler, the seam for the crash reporter | Accepted and implemented 2026-10-08 |
| BA-5 | The module table is generated into the build tree and exposed as an enum | Accepted and implemented 2026-10-08 |

### Testing — [`testing.md`](./testing.md)

| ID | Decision | Status |
|---|---|---|
| T-1 | doctest, pinned; purpose-built harnesses only for a subprocess runner, a micro-benchmark helper, the scenario runner and fuzz targets | Accepted 2026-10-08 |
| T-2 | Thirteen kinds of test with CTest labels, rolled out in phases; a module ships with unit, integration and a benchmark if its spec claims a cost | Accepted 2026-10-08 |
| T-3 | One test executable per module plus a support library; `tests/<module>/`, `integration/`, `smoke/`, `scenarios/`, `fuzz/`, `bench/`, `support/` | Accepted 2026-10-08 |
| T-4 | `ez test` runs unit and integration of the debug tree in under 30 s, headless; everything else opt-in; a missing environment means skipped, never failed | Accepted 2026-10-08 |
| T-5 | Fakes, not mocks: virtual clock, captured log sink, counting and fault-injecting allocator, loopback transport, null audio, temp directory, virtual device pack | Accepted 2026-10-08 |
| T-6 | Test hygiene: independent cases, random order in `check`, no sleeps or wall clock, per-label timeouts, readable sentence names, exceptions on in test files only | Accepted 2026-10-08 |
| T-7 | Smoke through a `--smoke=<frames>` flag, label `gpu`, opt-in | Accepted 2026-10-08 |
| T-8 | Sanitizer runs reuse the presets: `ez test asan` | Accepted 2026-10-08 |
| T-9 | Fuzz through libFuzzer with committed corpora; crashes become regression tests | Accepted 2026-10-08 |
| T-10 | Bench as a feature: `ez bench` on the release tree, local JSON-lines history with deltas, no gate until a reference machine exists | Accepted 2026-10-08 |
| T-11 | Scenario tests with golden per-frame hashes, ABI tests with a second compiler, soak against the virtual rig; arrive with the engine, SDK and rig | Accepted 2026-10-08 |
| T-12 | Hardware in four tiers: software twins beside each module, a virtual rig, firmware on the host, real devices that skip when absent | Accepted 2026-10-08 |
| T-13 | CI, the performance reference machine and the hardware bench machine are deferred; the plan is recorded, nothing is built | Accepted 2026-10-08 |

## Earlier statements these proposals change

| Earlier statement | Where | Changed to |
|---|---|---|
| `make`-level developer commands | `philosophy.md` §2.9, `application-architecture.md` §5, `things I want.md` | CMake script mode and presets, optional `ez` launcher (B-4) |
| Custom build type `RelWithTracyProfiler`; a separate `RelWithDebInfo` mode | `philosophy.md` §3.6, `application-architecture.md` §5 | `profile` preset; `release` always produces symbols (B-2, B-3) |
| Dependencies via CMake `FetchContent` | `application-architecture.md` §5 | Explicit fetch step driven by the manifest (B-7) |
| Vendor linking "undecided" | `philosophy.md` §6, `application-architecture.md` §9, `plugins.md` §7 | Static (L-1 to L-5) |
| Synchronous logger writing to the platform console; reversed level enum; fixed 20 KiB format buffer | prototype `src/core/logger/`, `configs.hpp` | Per-thread rings and a log thread, ascending levels matching `EZ_LOG_LEVEL`, runtime line size (LG-3, LG-5, LG-9) |
| `release` log floor at Warn | `build-system.md` B-3 table, `observability.md` §6, `cmake/modes.cmake`, `docs/build-modes.md` | Info floor and Info runtime default (LG-3), accepted 2026-10-08; the two specs are corrected, the CMake table and how-to page are updated when the logger is implemented |
| Target names in the B-9 graph: `ez_plugin_host`, `ez_sdk` as a target under `src/`, one `ez_base` holding memory, platform, logging and metrics | `build-system.md` B-9 | A `plugin` module, `sdk/` at the repository root, one module per directory with `base` holding only the primitives (CO-1, CO-7, CO-9) |
| Testing frameworks "open" | `philosophy.md` §6, `application-architecture.md` §8 and §9, `build-system.md` B-12 and §6 | doctest plus small purpose harnesses (T-1); the test kinds and labels in T-2 extend B-12's list |

## Tooling (implemented 2026-10-05)

The build-system proposals were prototyped in a throwaway `experiments/` folder around an audio-visualizer app, then promoted to the repository root. The app was replaced by a minimal Dear ImGui window on top of the original core (definitions, configs, assertions, logger, memory, platform). How-to pages live in [`../docs/`](../docs/).

Results measured during the experiment on Linux (Clang 19, mold, ccache); macOS and Windows scripts are written but not yet run:

| Proposal | Result |
|---|---|
| B-2/B-3 preset per mode, own build tree | All five presets build and pass tests. Switching to an already-built mode: 0.02 s. No cache wipes. |
| B-7 manifest + shallow fetch | One commit per dependency, seconds to fetch. Configure never touches the network. |
| B-6 doctor / init | Fresh clone to configured tree in under 10 s. Doctor prints exact install commands. |
| B-10 fast builds | Cold clean build ~11–14 s per mode; warm ccache 2.5 s; one-file edit 0.1 s. |
| B-13 VS Code | Generated workspace + tracked tasks/launch with no machine paths. |
| B-14 tools | Tracy prebuilt (SHA-256 verified, tamper rejected) and source build both work; both report the pinned commit. |
| L-3 runtime | Release binary has no dynamic libstdc++/libgcc dependency; symbols split to `ezego.debug`. |
| O-4/O-5 observability | Tracy captured zones across threads; profile-only `operator new` hooks work without recursion. The always-on metrics, tagged memory system and crash reports were prototyped too, then removed with the app; they return when those systems are designed. |

Corrections it caused are marked in [`build-system.md`](./build-system.md) (B-14 prebuilts, B-10 module scanning) and [`observability.md`](./observability.md) (profiler start order).

## Product context recorded on 2026-10-04 (not yet designed)

- Targets are not only physical venues. The same show should be able to drive **virtual fixtures inside a game or game engine**, through an engine-side plugin.
- Outputs are not only lights: other show machines, such as **fog and smoke machines**, are in scope.
- **TouchDesigner** is a reference for performance and for driving many computations and many screens at once. Its steep learning curve is what eZeGo should avoid: **progressive complexity**, easy to start, deep when wanted.
- UX and UI design are not ready and are deliberately out of scope for the current low-level design work. Amir will provide the designs and UX before the UI widget vocabulary is designed ([`ui-system.md`](./ui-system.md) U-8).

## Suggested next design topics

1. Memory system API (tags, arenas, platform capabilities): everything else allocates through it.
2. Platform layer, starting with the reference clock.
3. Instrumentation facade and metrics registry ([`observability.md`](./observability.md)); the in-app storage design was sketched on 2026-10-08 and is to be written up as the concrete form of O-2.
4. Game loop, frame pipeline and the render-packet seam.
5. Audio library choice and the audio thread contract.
6. Plugin SDK surface; the plugin UI table follows the widget vocabulary (U-8).
7. Networking and show-output transports.
8. Errors and crash reporting: exceptions off, status codes, in-process crash handler, assertion tiers (brainstormed 2026-10-08, not yet written up).
9. Platform module: clock, paths, threads, process; the logger and cvars carry stubs for these until it exists.

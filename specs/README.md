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

## Earlier statements these proposals change

| Earlier statement | Where | Changed to |
|---|---|---|
| `make`-level developer commands | `philosophy.md` §2.9, `application-architecture.md` §5, `things I want.md` | CMake script mode and presets, optional `ez` launcher (B-4) |
| Custom build type `RelWithTracyProfiler`; a separate `RelWithDebInfo` mode | `philosophy.md` §3.6, `application-architecture.md` §5 | `profile` preset; `release` always produces symbols (B-2, B-3) |
| Dependencies via CMake `FetchContent` | `application-architecture.md` §5 | Explicit fetch step driven by the manifest (B-7) |
| Vendor linking "undecided" | `philosophy.md` §6, `application-architecture.md` §9, `plugins.md` §7 | Static (L-1 to L-5) |

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
3. Instrumentation facade and metrics registry ([`observability.md`](./observability.md)).
4. Game loop, frame pipeline and the render-packet seam.
5. Audio library choice and the audio thread contract.
6. Plugin SDK surface; the plugin UI table follows the widget vocabulary (U-8).
7. Networking and show-output transports.

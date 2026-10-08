# eZeGo — Code Organization

> **Status:** Accepted and applied, 2026-10-08: `modules.cmake`, `src/ez/` with base, cvars, log, metrics and app, `src/apps/ezego/`, and per-module tests. The prototype (`src/core/`, `src/application/`) was removed the same day. Decision IDs (`CO-n`) are indexed in [`README.md`](./README.md).
> **Companions:** [`application-architecture.md`](./application-architecture.md) §2 (the layers this tree implements) · [`build-system.md`](./build-system.md) B-9, B-12 · [`naming.md`](./naming.md) N-2, N-3, N-8 · [`linking.md`](./linking.md) L-6 · [`plugins.md`](./plugins.md)
> **Scope:** where code lives: the repository tree, the inside of a module, the rules that keep the tree stable while the domain and the UX are still open, the module inventory, and how the tree grows. What a module does is its own spec's business.

---

## 1. The tree

```text
eZeGo/
├─ CMakeLists.txt  CMakePresets.json  dependencies.json  modules.cmake   # modules.cmake: the module table (§5)
├─ cmake/          build logic, one job per file                          # exists
├─ scripts/        doctor, init, deps, tools, lint; per-OS parts          # exists; lint is new (naming.md §13)
├─ specs/  docs/   design intent; contributor how-to                      # exist
├─ assets/         fonts, icons, default theme, shipped data              # exists
├─ third_party/    <name>.cmake per dependency, patches/, _src/ ignored   # exists
├─ tools/          <name>.cmake per pinned developer tool                 # exists
├─ sdk/            the public plugin SDK: include/ezego/*.h, a CMake helper for plugin authors,
│                  its own .clang-tidy. The only directory other people compile against.   # new
├─ plugins/        first-party plugins built against sdk/: examples, device packs            # new, later
├─ src/
│  ├─ ez/          one directory = one module = one namespace = one static library
│  │  ├─ base/       namespace ez: types, platform detection, macros, handles, result, containers
│  │  ├─ platform/   ez::platform: clock, paths, filesystem, threads, process, loader, console
│  │  ├─ mem/  log/  cvars/  crash/  metrics/                                   # layer 0
│  │  ├─ engine/  state/  timebase/  serialize/                                 # layer 1
│  │  ├─ render/  window/  audio/  net/  hw/  ui/                               # layer 2
│  │  └─ plugin/  nodes/  lighting/  app/                                       # layer 3
│  ├─ apps/        executables, nothing but main: ezego/main.cpp, ezego-headless/main.cpp
│  └─ tools/       first-party utilities built in every tree: a fake Art-Net receiver, a fixture viewer
├─ tests/          tests/<module>/<topic>_test.cpp, mirroring src/ez/
└─ experiments/    throwaway prototypes, outside the build
```

Layers are the ones in [`application-architecture.md`](./application-architecture.md) §2. They are shown as comments because they are not directories (CO-1).

---

## 2. Inside a module

```text
src/ez/log/
├─ CMakeLists.txt        ez_module(log SOURCES ...): layer and dependencies come from the table
├─ log.hpp  log.cpp      every .hpp at this level is the module's public API
├─ categories.hpp
├─ sink_console.cpp  sink_file.cpp
├─ console_windows.cpp   platform file, selected by CMake from the suffix, never by #ifdef around the file
└─ detail/               ez::log::detail: in headers because templates need it; not API
   └─ ring.hpp
```

A module has an explicit source list (B-9), one namespace that is its directory path ([`naming.md`](./naming.md) N-2), public headers at its top level, private headers under `detail/`, and nothing else that other modules may include.

---

## 3. The rules (CO-1 to CO-9)

| ID | Rule | What it buys |
|---|---|---|
| **CO-1** | **One directory is one module, one namespace and one static-library target.** Modules sit flat under `src/ez/`. **The layer is a column in the module table and an argument to `ez_module`, never a path segment.** | Moving a module between layers is a one-line change. A layer directory would put the layer into every path, namespace and include, which is the churn to avoid while the design settles. The build graph enforces the layer rule regardless. |
| **CO-2** | **Dependencies are declared per module** in its `CMakeLists.txt` and **checked at configure time**: a module may depend only on modules of the same or a lower layer, and the graph must be acyclic. | A layer violation is a configure error naming both modules, not a review comment. |
| **CO-3** | **Public is the top level, private is `detail/`.** Other modules include only a module's top-level headers. | One look at a directory shows the API. The lint script refuses `ez::x::detail` outside module `x`. |
| **CO-4** | **Platform code by suffix, in place:** `clock_linux.cpp`, `clock_macos.cpp`, `clock_windows.cpp`, `clock_posix.cpp`; CMake selects by suffix. | A topic's OS versions sit together, and no subdirectory invents a namespace that is not one. |
| **CO-5** | **Tests mirror the tree** under `tests/<module>/`, as `<topic>_test.cpp`, linking the module and nothing above it. | Finding a module's tests is mechanical; headless testability stays a link-time fact. |
| **CO-6** | **Executables contain nothing but `main`.** `src/apps/<name>/main.cpp` wires modules together; `src/tools/<name>/` are first-party utilities built in every tree. | The application, the headless runner and the tests link the same code. |
| **CO-7** | **The SDK lives at `sdk/`, outside `src/`**, with `include/ezego/*.h`, a CMake helper for plugin authors and its own `.clang-tidy`. First-party plugins live under `plugins/` and are built against it like any third party would. | The one directory other people compile against packages as it is and cannot accidentally include a host header. First-party plugins are the SDK's permanent test. |
| **CO-8** | **`modules.cmake` is the single module table:** name, layer, dependencies, one-line description. CMake creates the targets from it and generates `ez/base/modules.gen.hpp`, the X-macro behind log categories, cvar prefixes and memory tags. The lint script checks that directories, namespaces and targets match it. | The six places that must agree ([`naming.md`](./naming.md) N-3) collapse into one file. Fallback if generation ever feels like too much: a hand-written header that the lint script cross-checks against CMake. |
| **CO-9** | **`src/ez/base/` maps to namespace `ez` itself**, not `ez::base`: `ez::u32`, `ez::Handle`, `ez::Status`, `ez::Pool`. It is the one exception to N-2 and is written into that rule. | The primitives read as the language of the codebase rather than as a module's exports. |

The prototype's `src/core/` and `src/application/` were removed on 2026-10-08; what they did lives in `base`, `log`, `metrics` and `app`. `experiments/` is for throwaway prototypes and is not part of the build.

**One documented exception.** Until the `window`, `render` and `ui` modules are designed, `app` holds a placeholder shell (`src/ez/app/detail/shell.cpp`): one GLFW window with Dear ImGui and an information panel. It is the only code that calls GLFW or ImGui directly, which [`ui-system.md`](./ui-system.md) U-1 otherwise forbids; it is replaced, not extended, when those modules exist.

---

## 4. Module inventory

`Has spec` is `true` when a document in `specs/` defines the module's API and behaviour, and `false` when only its seat is reserved: the name, the layer and a one-line responsibility. A `false` module gets its spec before any code.

| Layer | Module | Responsibility | Has spec |
|---|---|---|---|
| 0 | `base` | Types, platform detection, macros, assertions, strings, hashing, the module table | true, [`base.md`](./base.md) |
| 0 | `platform` | Clock, paths, filesystem, threads, process, dynamic loader, console, message box | false |
| 0 | `mem` | The memory system: tags, arenas, pools, platform capabilities | false |
| 0 | `log` | Logging | true, [`logging.md`](./logging.md) |
| 0 | `cvars` | Runtime variables | true, [`cvars.md`](./cvars.md) |
| 0 | `crash` | Status codes, assertion tiers, the crash and hang reporter | false |
| 0 | `metrics` | Always-on telemetry, the instrumentation facade, the Tracy backend behind it. Today: the profiler half (`EZ_PROF_*`) and the profile-only `operator new` hooks | true, [`observability.md`](./observability.md) |
| 1 | `engine` | The loop, evaluation, the layer compositor | false |
| 1 | `state` | Handle and POD pools, commands and undo | false |
| 1 | `timebase` | Transport and show-time over the reference clock | false |
| 1 | `serialize` | Project and settings formats | false |
| 2 | `render` | GPU backend behind a seam; the render packet consumer | false |
| 2 | `window` | Windows, displays, input, through GLFW | false |
| 2 | `audio` | Playback and analysis on the audio thread | false |
| 2 | `net` | Transports: Art-Net, sACN, general UDP | false |
| 2 | `hw` | Hardware service layer: discovery, capabilities, flashing, streaming | false |
| 2 | `ui` | The UI system in eZeGo's own vocabulary | true, [`ui-system.md`](./ui-system.md) |
| 3 | `plugin` | Host side: loader, registry, the host table | true, [`plugins.md`](./plugins.md) |
| 3 | `nodes` | The node-graph runtime | false |
| 3 | `lighting` | The lighting domain | false |
| 3 | `app` | Wiring, screens, the application object. Today: the main loop, the app cvars, and the placeholder shell | false; exists as a placeholder (§3) |

---

## 5. The module table

```cmake
# modules.cmake : the single list of modules. Everything else derives from it (CO-8).
#            name       layer  depends on                       description
ez_declare_module(base      0  ""                               "types, macros, handles, containers")
ez_declare_module(platform  0  "base"                           "clock, paths, fs, threads, process")
ez_declare_module(cvars     0  "base;platform"                  "runtime variables")
ez_declare_module(log       0  "base;platform;cvars"            "logging")
# ...
```

At configure time this list produces the `ez_<module>` targets and `ez::<module>` aliases, checks every dependency against the layer rule and for cycles, and writes `ez/base/modules.gen.hpp`:

```cpp
#define EZ_MODULES(X) X(base, "base", 0) X(platform, "platform", 0) X(cvars, "cvars", 0) /* ... */
```

Log categories, cvar prefixes and memory tags are generated from that macro, and the lint script walks the same list.

---

## 6. How it grows

| Change | What it takes |
|---|---|
| **Add a module** | One line in `modules.cmake`; a directory with `CMakeLists.txt` and the public headers; `tests/<module>/`. The log category and cvar prefix exist by themselves. The lint script reports any of the six places that disagree. |
| **Move a module between layers** | Change the column. The configure-time check reports every dependency the move breaks. |
| **Split a module** | Create the second directory and table entry. The lint script and the compiler list every include and namespace that must follow. Nothing else is coupled to the old shape. |
| **Remove a module** | Delete the line and the directory; the build reports every remaining include. |

Splits that look likely once the domain and UX are concrete: `ui` into widgets, canvas and viewport per U-4; `net` into per-protocol subdirectories such as `net/artnet/`; `hw` into device packs.

---

## 7. What this depends on

| Needs | From |
|---|---|
| `ez_declare_module`, the layer and cycle check, the generated header | `cmake/targets.cmake`, extending the existing `ez_module` |
| The lint checks for directories, namespaces and targets | [`naming.md`](./naming.md) §13 |
| The module table's C++ side | The base-layer spec |

---

## 8. Open questions

### 8.1 To clarify before the tree is created

Nothing blocks; the open names are the placeholder modules, which are confirmed with their own specs.

### 8.2 Deferred

| Topic | Default | Revisit when |
|---|---|---|
| Generated `modules.gen.hpp` versus a hand-written header cross-checked by lint | Generated | The first time it surprises someone |
| Root `tools/` (developer-tool recipes) next to `src/tools/` (first-party utilities) share a word | Keep both; the paths differ | If it confuses in practice; `devtools/` is the rename |
| `experiments/` tracked or ignored | Ignored, like `scratchpad/` | A prototype is worth keeping |
| `plugins/` creation | When the SDK has its first function table | SDK design |
| Co-locating tests inside modules instead of `tests/` | `tests/` | The mirror becomes a burden |

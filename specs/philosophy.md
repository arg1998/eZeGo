# eZeGo — Engineering Philosophy

> **Status:** Living document. Last updated 2026-10-04.
> **Source of intent:** [`things I want.md`](./things%20I%20want.md). This file distills that wishlist into the *ideology, metrics, and assessment rules* we hold ourselves to.
> **Companions:** [`application-architecture.md`](./application-architecture.md) · [`threading-and-timing.md`](./threading-and-timing.md) · [`plugins.md`](./plugins.md) · [`build-system.md`](./build-system.md) · [`linking.md`](./linking.md) · [`observability.md`](./observability.md) · index: [`README.md`](./README.md)

---

## 1. Mission — the *why*

eZeGo exists to **collapse the barrier to entry for stage lighting**. The incumbent tools (grandMA, Hog, etc.) are powerful but assume expensive DMX hardware and deep technical literacy. eZeGo instead:

- **Starts at the consumer floor** — a kid with an LED strip, a power supply, and an Arduino/ESP32/Pi plugs into eZeGo and it just works: detect → program → compile → deploy onto the connected hardware.
- **Scales up to the professional ceiling** — MIDI controllers, DMX boards, moving heads, lasers, fog. Complexity grows only as the *stage* grows, never as a fixed up-front tax.
- **Speaks the user's native interaction language** — modern UX borrowed from video games and touch devices, not from 1990s lighting desks. Minimal jargon. Innovative, **not** a clone of existing products.
- **Is extensible by its users** — a node/graph "blueprint" editor (WYSIWYG, data-driven) for end users, and a native C ABI plugin SDK for developers (see [`plugins.md`](./plugins.md)).

> The product test for any feature: *does it lower the floor without lowering the ceiling?*

---

## 2. Engineering tenets

These are the non-negotiable defaults. The tone is **"justify the exception,"** not "forbidden forever" — anything within reason can be used to reach the goal, but the burden of proof is on the deviation.

1. **Performance is a feature.** Think like a game engine. **60 FPS floor, 120 FPS target** on machines with a dedicated GPU. Budget is **16.6 ms (60 Hz) / 8.3 ms (120 Hz)** per frame, sub-divided per system. A single 20 ms hitch is worse than a slightly lower average.

2. **Justify every dependency and every C++ feature.** The prime resource metric is **memory**. On-the-fly heap allocation in the hot path via smart pointers is forbidden by default. Virtual dispatch in the render/hot loop is forbidden unless the call is genuinely rare and the cost is justified (e.g. a context-menu action). Every `std::` use passes scrutiny: *what does it allocate, when, and can the compiler see through it?*

3. **Data-oriented core (the keystone).** The domain is modeled as **handles/IDs into POD pools**, not a pointer-graph of heap objects. This one decision simultaneously serves performance (cache-friendly, SIMD-able), serialization (IDs serialize trivially), threading (snapshots are `memcpy` of arrays), plugin safety (cross-ABI references are stable IDs, never raw pointers), and undo/redo (record old/new values by ID — no dangling `this`).

4. **Seams over implementations.** Make the *boundary* permanent and rigid; implement the minimum behind it; expand when real use cases apply pressure. Applies to: the memory allocator interface, the graphics backend, the windowing layer, the clock/timebase, and the plugin ABI. *The seam is forever; the implementation churns.*

5. **Multithread along timing domains, not code modules.** You parallelize because two things own different clocks (display vs. audio vs. show output), not because two things live in different files. See [`threading-and-timing.md`](./threading-and-timing.md).

6. **The engine core is headless and renderer-agnostic.** Dependency arrows point **inward**: UI → engine, never engine → UI. The engine must run, simulate, and be tested with no GPU, no window, and no ImGui present. This is free to adopt now and brutal to retrofit.

7. **Abstract the platform, but exploit it.** The platform layer is **capability-based with graceful fallback**, not lowest-common-denominator. If an OS offers a faster path (huge pages, a better timer, native windowing), the API exposes it and falls back cleanly where it's absent.

8. **Extensibility is first-class but coarse-grained at the boundary.** Plugins cross a C ABI at frame/event granularity, never per-fixture/per-sample, *precisely because* the boundary is an optimization barrier (the same reason we ban virtuals in the hot loop).

9. **Reproducible, self-maintaining builds.** Minimal vendored dependencies, **a single dependency manifest** (name, version, source, branch/commit), no git submodules. One-word developer commands (CMake presets plus thin per-OS scripts) instead of hand-typed CMake invocations. Design: [`build-system.md`](./build-system.md) *(proposed 2026-10-04; replaces the earlier `make`-based idea)*.

10. **MIT core; proprietary tech via plugins.** The application is MIT-licensed. The plugin system is the sanctioned path for closed-source / proprietary extensions.

---

## 3. Metrics — what we measure and how we assess it

We optimize against **numbers, not vibes**. Every optimization must move a measured metric; every intrinsic/fast path must beat its scalar fallback by a measured margin or be deleted.

### 3.1 Frame time (not "FPS")

| Metric | Target | Notes |
|---|---|---|
| Frame time p50 | ≤ 8.3 ms | Steady-state at 120 Hz |
| Frame time p99 | ≤ 16.6 ms | A rare frame may dip toward the 60 Hz floor |
| Frame time max | ≤ ~16 ms | Hard ceiling; anything above is a *hitch* and is tracked |
| Per-system sub-budget | declared per system | Input, logic, render-submit, ImGui draw-list build each get a slice |

We report **frame time in milliseconds with percentiles**, never a single averaged FPS number (averages hide jank).

### 3.2 Memory

- **Allocations per frame on the hot path: target 0** in steady state.
- **Peak and steady-state working set** tracked over a session.
- **Allocation count & bytes by tag** (what *kind* of memory) via the memory system.
- **Allocation lifetime distribution** (time between alloc and free) — surfaces leaks and churn.
- **Fragmentation** where arenas/pools are used.

The memory system (see [`application-architecture.md`](./application-architecture.md) §Foundations) is the single choke point that makes these observable **without** a global `malloc` hook in normal builds; 3rd-party allocations are instrumented in the profiling build by injecting allocators where libraries allow it.

### 3.3 Latency & sync

- **Input → photon** latency (control change to light change).
- **Audio → light sync error** in milliseconds (music visualization / beat alignment).
- **Per-timing-domain phase offset** against the reference clock (see [`threading-and-timing.md`](./threading-and-timing.md)).

### 3.4 Robustness

- **Dropped audio buffers: must be 0.** Audio underruns are a hard failure.
- **Frames over budget:** counted and trended.
- **Blocking-IO stalls** on the frame thread: must be 0 (IO lives on worker/output threads).
- **Determinism:** identical project + identical inputs ⇒ identical output (this is what makes scenario/simulation testing possible).

### 3.5 Lifecycle

- **Startup time**, **project load/save time** — tracked as user-facing latencies.

### 3.6 Assessment cadence

- **Deep dives:** the `profile` build mode + Tracy (CPU zones, memory, frame marks). *(Was the custom `RelWithTracyProfiler` build type; see [`build-system.md`](./build-system.md) B-2/B-3.)*
- **Always-on:** a lightweight in-app performance HUD (frame time, alloc/frame, memory by tag). Design: [`observability.md`](./observability.md).
- **Gates (later):** CI performance regression checks. **No merge to `main` may regress frame-time p99 or introduce hot-path allocations** without an explicit, justified waiver.

---

## 4. Decision rules — "definition of done" for a system

A system is not done until it:

1. Has a **clear boundary and single responsibility**.
2. Is **headless-testable** (no GPU/window dependency in the core logic).
3. Has **no hot-path heap allocation** (or a justified, measured exception).
4. Exposes a **documented API with room to grow** (versioned where it's a public/plugin seam).
5. Provides a **plugin seam** if it is a system users should be able to extend.
6. Is **measured against its declared budget**.

---

## 5. Explicit non-goals (for the current build)

These are deliberately *out of scope now* to keep us honest; each has a deferred path.

- **Photorealistic 3D.** We want low-poly geometry with *good* lights/shadows — "close enough to reality," not a renderer arms race.
- **Scripting languages (TypeScript/Python).** The node-graph "blueprint" editor covers user-authored logic for now; embedded scripting is a later tier.
- ~~**Custom window chrome.** We ship with OS-native title bars/frames now; custom chrome is a contained, later change behind the windowing seam.~~ *Superseded 2026-10-09:* custom chrome ships from the first window on every platform ([`windowing.md`](./windowing.md) W-2), and SDL3 replaced GLFW (W-1).
- **Full master-clock transport & external sync** (SMPTE/MTC/MIDI-clock/Ableton Link). We build the **monotonic reference clock now** and a *thin* timebase seam; the pluggable external sources come later.
- **A second graphics backend (Vulkan/DirectX).** OpenGL 4.1 now; the backend stays behind a seam so a swap is possible if/when the API becomes the bottleneck.

---

## 6. Assumptions & things not yet settled

> These are written down precisely so we don't mistake them for decisions.

- **C++ standard:** C++20, no C++23 features ([`build-system.md`](./build-system.md) B-16, decided 2026-10-08).
- **Vendor library linking (static vs dynamic):** **proposed: static** for everything built from source; plugins are dynamic by nature. Reasoning and trade-offs in [`linking.md`](./linking.md) *(proposed 2026-10-04, awaiting confirmation)*.
- **The domain model's concrete shape** (fixtures, groups, scenes, cues, patches, the node-graph component format) is **not yet designed** — only the *style* (handle/POD) is decided.
- **The novel UX** beyond "game/touch-native, scales with expertise" is **largely unspecified** and will shape Layer-1 data decisions when detailed.
- **Audio library** (RtAudio — sibling of the already-vendored RtMidi — vs. miniaudio) is **open**.
- **Networking stack** (for Art-Net/sACN/transport) is **open**; leaning toward a minimal UDP layer over pulling in Boost.Asio.
- **Testing frameworks:** doctest plus four small purpose harnesses ([`testing.md`](./testing.md) T-1, decided 2026-10-08).
- **GCC support:** unsupported but unblocked. The toolchain accepts `-DEZ_ALLOW_GCC=ON`, detection never rejects it, and nothing promises it stays green (B-11).
- The **performance budgets in §3 are starting hypotheses** to be validated by measurement, not laws handed down.

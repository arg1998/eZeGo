# eZeGo — Testing

> **Status:** Accepted, 2026-10-08, with doctest. **Phase 1 implemented 2026-10-08**: doctest, `ez_test()`, per-case discovery, `tests/support/` (subprocess runner, temp directory); how-to in [`../docs/testing.md`](../docs/testing.md). Phase 4 (bench) implemented the same day. Later phases are not implemented. Decision IDs (`T-n`) are indexed in [`README.md`](./README.md).
> **Companions:** [`build-system.md`](./build-system.md) B-3 (sanitizer presets), B-12 (tests through CTest) · [`code-organization.md`](./code-organization.md) CO-5 · [`naming.md`](./naming.md) N-8 · [`observability.md`](./observability.md) §7 (regression pipeline) · [`application-architecture.md`](./application-architecture.md) §3.2, §8 · [`philosophy.md`](./philosophy.md) §3.4, §4
> **Scope:** every kind of test eZeGo will have, which ones exist now, how a developer runs them, the framework and the few purpose-built harnesses, and the strategy for hardware. Continuous integration, the performance reference machine and the hardware bench are **deferred** (§12): all development is local for now and GitHub only hosts the source.

---

## 1. Intent

- **Everything is rolled out slowly.** The taxonomy in §2 is the whole picture so that each piece is built with the others in mind. Only the first phase of §3 is built with the first modules; the rest arrives with the module it tests. The testing system itself is infrastructure to gain experience with, and it will be refactored once that experience exists.
- **The default run is fast, headless and boring.** `ez test` finishes in under half a minute, needs no display, no network, no device, and never asks about labels. Everything slower or environment-dependent is opt-in (T-4).
- **Determinism is the net.** The engine is deterministic by design (philosophy §3.4); tests turn that into a regression check that catches a changed output anywhere in the pipeline.
- **No mocks.** Seams are function tables and POD; the test doubles are hand-written fakes that behave like the real thing without hardware (T-5).

---

## 2. The kinds (T-2)

| Kind | Question it answers | Scope | Speed | CTest label |
|---|---|---|---|---|
| **Unit** | Does this module do what its spec says? | One module, headless, no I/O beyond a temp directory | Milliseconds | `unit` |
| **Integration** | Do these modules agree with each other? | A few modules, headless: cvars feeding the logger at startup, engine plus serialize round trip, net over a loopback receiver, the plugin host loading a test plugin | Under a second | `integration` |
| **Smoke** | Does the real binary start, draw frames and exit cleanly? | The app with a window, N frames, exit code 0 | Seconds | `gpu` |
| **Sanitizer runs** | Memory errors, data races, realtime violations | The whole suite under the `asan`, `tsan`, `rtsan` presets | Minutes | by preset |
| **Fuzz** | Does hostile input crash a parser? | Project files, settings files, Art-Net and sACN packets, plugin manifests, through libFuzzer | Bounded by a time limit | `fuzz` |
| **Bench** | What does this cost, and did it change? | Micro-benchmarks of hot operations plus scenario metrics | Seconds; needs a quiet machine for stable numbers | none; run by `ez bench` |
| **Scenario** | Same input, same output? | A recorded project and input stream replayed by the headless runner; per-frame output hashed against a golden file | Seconds, faster than real time with the virtual clock | `scenario` |
| **ABI** | Can a plugin built with another compiler still load? | A test plugin built with GCC or MSVC, loaded by the Clang host; the SDK compiled as C | Seconds | `abi` |
| **Soak** | Does it survive a show? | The headless engine driving a virtual rig for hours: memory growth, audio dropouts, output jitter | Hours | `soak` |
| **End-to-end** | Does a user flow work? | Input events replayed through the platform seam; UI built, not necessarily drawn | Seconds | `e2e` |
| **Visual** | Does it still look the same? | A screenshot of a known scene compared with a golden image within a tolerance | Seconds | `visual` |
| **Hardware-in-the-loop** | Does it work with the real thing? | Real devices, on a bench machine or plugged into a developer's machine | Minutes | `hardware` |
| **Manual release protocol** | Does it work in a venue? | A checklist with real gear, diagnostics export attached | Hours | none |

Three kinds deserve a sentence on why they exist for a live-show tool. **Scenario** tests make determinism a regression check. **Soak** tests are the real acceptance test, because the failure mode that matters is the one that appears in the second hour. **Fuzz** tests cover the two inputs we do not control, user files and network packets.

---

## 3. Rollout (T-2)

| Phase | What | Arrives with | Status |
|---|---|---|---|
| **1** | Framework, `ez test`, unit and integration, the support library, the subprocess harness, labels | The first modules: base, cvars, log | **Done 2026-10-08** |
| 2 | Smoke through a `--smoke` flag; sanitizer runs through the existing presets | The first window | Next |
| 3 | Fuzz targets and `ez fuzz` | The first parsers: settings file, project file | When they exist |
| 4 | Bench as a feature: micro-benchmarks, `ez bench`, local history | The logger and cvars, whose specs promise numbers | **Done 2026-10-08** |
| 5 | Scenario tests, the virtual clock, soak | The engine and the headless runner | Later |
| 6 | ABI tests | The plugin SDK | Later |
| 7 | Hardware software twins and the virtual rig | The net, hw, audio and midi modules, each with its twin | Later |
| 8 | End-to-end and visual | The UI vocabulary (U-8) | Deferred |
| 9 | CI, the reference machine, the bench machine | A decision to have CI | Deferred (§12) |

What this means for a module being written today: it ships with unit tests, integration tests where it meets another module, and a benchmark if its spec claims a cost. Nothing else is expected of it.

---

## 4. Framework (T-1)

The requirement, in Amir's words: fast, easy to run, lightweight but proper, nothing more and nothing less; a purpose-built harness is fine for one or two things no framework offers, as long as it stays small.

| Option | For | Against |
|---|---|---|
| **doctest** | One MIT header; the fastest-compiling of the common frameworks, which matters under the B-10 budgets; subcases suit table-driven tests over POD; expression decomposition in assertions; filters, random order, per-case CTest discovery, JUnit output | Slow release cadence; `REQUIRE` needs exceptions enabled in the test files; no death tests; no mocks |
| Catch2 v3 | Actively maintained; matchers, generators, built-in micro-benchmarks | Heavier compile; its benchmarks overlap with the metrics export |
| GoogleTest | Everyone knows it; death tests | Slowest compile; gmock pulls toward mock-based design |
| Own framework | Zero dependencies | Reporters, filters, discovery and output formats are weeks of work that buy nothing specific to eZeGo |

**Decision: doctest**, pinned in `dependencies.json` like every other dependency, `used_by: tests`. Catch2 is the fallback if doctest's maintenance ever becomes a problem; the test files would need a mechanical rename of macros and nothing else.

**Purpose-built harnesses**, each small, each for something no framework provides:

| Harness | Size | Replaces | Used by |
|---|---|---|---|
| **Subprocess runner** | ~150 lines over the platform's process primitive: spawn, arguments, capture stdout and stderr, exit code, timeout | Death tests | The crash reporter, the cvars fail-fast path, smoke |
| **Micro-benchmark helper** | ~150 lines: warm-up, run until a minimum duration, report min, median and p99 per operation, a `do_not_optimize` barrier, JSON-lines output | Catch2's benchmark | `ez bench` (§9) |
| **Scenario runner** | Not test code: the headless runner is product code; the test wraps it with replay and hash comparison | Nothing | Scenario tests (§10) |
| **Fuzz targets** | One function per target; libFuzzer provides the runner | Nothing | `ez fuzz` (§8) |

Anything beyond this list is a reason to revisit the framework choice, not to grow the harnesses.

**Exceptions.** When the errors spec turns exceptions off in modules (proposed in discussion, not decided), test files keep them on, because doctest's `REQUIRE` leaves a test case by throwing. The exception never crosses into module code; a failing assertion inside a module takes the crash path and is tested through the subprocess runner.

---

## 5. Layout and targets (T-3)

```text
tests/
├─ CMakeLists.txt
├─ support/              ez_test_support: doctest main, fakes, temp dir, subprocess runner, bench helper
├─ <module>/             unit and single-module integration tests, mirroring src/ez/ (CO-5)
│   └─ <topic>_test.cpp
├─ integration/          tests that span modules and belong to none
├─ smoke/                drives the real binaries through the subprocess runner
├─ scenarios/            recorded projects, input streams and golden hashes
├─ fuzz/                 one target per parser, corpus/ committed beside it
└─ bench/                micro-benchmarks, one file per module
```

| Decision | Detail |
|---|---|
| One executable per module | `ez_test_<module>`, linking that module and nothing above it. A one-file edit relinks one small binary. Headless testability stays a link-time fact (B-9). |
| One support library | `ez_test_support` holds doctest's implementation compiled once, so test files include a header and nothing else. |
| Registration | `ez_test(<module> SOURCES ... [LABEL ...])` in `cmake/testing.cmake`, wrapping `add_executable` and the support link, and registering one CTest test per doctest case through our own discovery script, `cmake/test_discovery.cmake`, so VS Code's Test Explorer shows cases, not binaries. *(doctest's own discovery script was not used: it overwrites labels with suite names and starts one process per case to learn them.)* |
| Labels | Default label from the directory: `tests/<module>/` is `unit`, `tests/integration/` is `integration`. A file that holds integration tests for one module declares `TEST_SUITE("integration")`; the suite name overrides the label. |
| Build | Tests build in every mode's tree (B-12). `ez test <mode>` runs the tests of that tree; `ez test` means the `debug` tree. |
| Temp files | Every test that touches the filesystem gets its own directory from the support library, under the build tree, removed on success and kept on failure. |

---

## 6. The developer contract (T-4, T-6)

| Rule | Decision |
|---|---|
| `ez test` | Builds and runs `unit` and `integration` of the `debug` tree. Target: under 30 seconds on the reference laptop, to be measured. No display, no network beyond loopback, no device. |
| Opt-in | `ez test gpu`: a test preset that includes only that label, because a preset's exclusion cannot be undone with `-L`; `ez test asan` by preset; `ez fuzz`, `ez bench` by command. A `gpu` or `hardware` test whose environment is missing reports **skipped**, never failed. |
| Editor | Test Explorer through CMake Tools (B-13): run or debug one case with a click; the same CTest entries as the terminal. |
| Failure output | doctest's decomposed expression, file and line, clickable through the problem matcher. Tests run with the log file sink on, so a failure has its log beside it. |
| Independence | A test case shares no mutable state with another and does not depend on order. The `check` preset runs cases in random order (CTest's scheduler). No seed is needed: each case is its own process with its own temp directory, so order could only leak through shared files, and those are per case. |
| Time | No sleeps; time comes from the virtual clock (T-5). A `unit` case over one second and an `integration` case over ten fail by CTest timeout. |
| Determinism | No wall clock, no real network except loopback, no reliance on the machine's locale or environment. |
| Naming | A test case name is a sentence a human can read: `TEST_CASE("ring: drops the line when full")`. Module first, colon, behaviour. |
| Assertions | `CHECK` by default so one case reports every failure; `REQUIRE` only when continuing is meaningless. |
| Flakiness | A flaky test is a bug. No automatic retries. |

---

## 7. Fakes, not mocks (T-5)

The seams the architecture already has are what the fakes plug into: function tables, POD snapshots, cvars read at init. A fake is a real implementation of a seam that happens to be cheap and controllable. Each lives in `tests/support/` and is written when the seam exists.

| Fake | Replaces | Enables |
|---|---|---|
| **Virtual clock** | The reference clock | Tests that advance time by calling a function; scenario tests faster than real time. **Design consequence:** the platform clock must have a source selectable at init. |
| **Captured log sink** | The console sink | Asserting that a module logged what its spec says, without parsing stderr |
| **Counting and fault-injecting allocator** | The memory system's backing allocation | "Zero allocations on this path" as a test; "the Nth allocation fails" for error paths without exceptions |
| **Loopback transport** | UDP to a device | Art-Net and sACN tests that validate packets, sequence numbers and timing in-process |
| **Null audio backend** | The audio driver | Deterministic analysis tests from a WAV file |
| **Temp directory** | The OS state and config directories | Settings and project files without touching the user's |
| **Virtual device pack** | Real hardware | §11 |

---

## 8. Smoke, sanitizers, fuzz (T-7, T-8, T-9)

**Smoke (T-7).** The application grows a `--smoke=<frames>` flag: open the window, run that many frames, exit 0. The test in `tests/smoke/` runs it through the subprocess runner and checks the exit code and the absence of Error lines in its log. Label `gpu`, opt-in locally, since it needs a display and a GL 4.1 context.

**Sanitizers (T-8).** Nothing new: the `asan`, `tsan` and `rtsan` presets already exist (B-3). `ez test asan` builds that tree and runs the same suite. The realtime sanitizer is the mechanical check of the audio thread's "zero alloc, zero lock" rule and of the logger's RT variant ([`logging.md`](./logging.md) §4.6).

**Fuzz (T-9).** One target per parser in `tests/fuzz/<name>_fuzz.cpp` exporting `LLVMFuzzerTestOneInput`, built by a `fuzz` preset that is `asan` plus `-fsanitize=fuzzer`. `ez fuzz <target> [seconds]` runs it with the committed corpus under `tests/fuzz/corpus/<target>/`. A crash found by fuzzing is committed as a regression unit test with the offending input. Clang-only, which the toolchain already is (B-11); Linux and macOS first, Windows when libFuzzer under `clang-cl` is verified.

---

## 9. Bench as a feature (T-10)

Benchmarks exist now as a testing feature to learn from, not as a gate. They run anywhere; the reference machine, when there is one, only decides where the numbers are trusted.

| Aspect | Decision |
|---|---|
| What | Micro-benchmarks in `tests/bench/<module>_bench.cpp` through the bench helper (§4), for the operations whose specs claim a cost: an enabled log line, a filtered one, the RT variant, a cvar read, a ring push. Later, scenario-level numbers from the headless runner's metrics export. |
| Where it runs | The `release` tree, because it must measure what ships. Not `profile`: its allocation hooks and Tracy client add measurable overhead to exactly the paths being measured. |
| Command | `ez bench [filter]`: builds `release`, runs the matching benchmarks, prints a table of min, median and p99 per operation. |
| History | Every run appends JSON lines to `build/bench/history.jsonl`, tagged with commit, mode, CPU, OS and a machine id. The next run prints the delta against the previous run on the same machine. |
| Gate | **None.** Numbers are informative until a reference machine exists (§12). |
| Noise | The helper runs each benchmark until a minimum wall time and reports the distribution, not an average, so a noisy laptop still gives a usable median and an honest p99. |

---

## 10. Scenario, ABI, soak (T-11)

These arrive with the engine, the SDK and the virtual rig. Recorded here so the modules they depend on leave room for them.

| Kind | Mechanism | Needs |
|---|---|---|
| **Scenario** | A directory under `tests/scenarios/<name>/` holds a project, a recorded input stream and the expected per-frame output hash. The headless runner replays it under the virtual clock; the test compares hashes and, on mismatch, writes the actual output beside the expected for inspection. Bit-exact on one platform; cross-platform within a tolerance, since fused multiply-add differs between x86-64 and arm64. | Headless runner, virtual clock, input recording at the platform seam |
| **ABI** | `plugins/` holds a test plugin. When a second compiler is present, `doctor` reports it and the test builds the plugin with it and loads it in the Clang host; otherwise skipped. The SDK headers compile as C in the test build (B-9). | The SDK |
| **Soak** | `ez soak <scenario> <minutes>` runs the headless engine against the virtual rig and fails on memory growth beyond a bound, any audio dropout, or output jitter beyond its budget, all read from the metrics export. | Virtual rig, metrics export |

---

## 11. Hardware (T-12)

Hardware is tested in four tiers. The first three need no hardware and are what make the fourth small.

1. **Every hardware boundary has a software twin, built alongside the module it fakes.** UDP: the loopback receiver that validates Art-Net and sACN, which is also a first-party tool under `src/tools/`. Serial: a virtual port pair, `socat` on Linux and macOS, com0com on Windows, with a small process on the other end speaking the device protocol. MIDI: a virtual port, ALSA virtual MIDI, the macOS IAC bus, loopMIDI on Windows. Audio: the null backend. The seam is the platform layer, selected at init by a cvar such as `hw.backend=virtual`, through function tables.
2. **A virtual rig.** A headless consumer of the output stream that knows what every fixture would show. The show, the engine, the output thread and the transport run for real; only the last metre is simulated. This is product code too: it is the same thing the game-engine fixtures need. It drives scenario and soak tests.
3. **Firmware on the host.** The device packs flash firmware we build. With the firmware's hardware layer compiled for the host, the device protocol runs as a normal process on the other end of the virtual serial port. Emulators, QEMU for ESP32 and simavr for AVR, are a later tier if that is not faithful enough.
4. **Real devices.** `hardware`-labelled tests run against whatever is plugged into the developer's machine and skip what is absent: discover, flash a known firmware, stream, verify through a loopback receiver. The bench machine that runs them unattended is deferred (§12).

Design consequence for the `hw` module: the device descriptor, discover, capabilities, flash and stream, must be implementable by a virtual device pack, which also makes it the SDK's first real plugin and its permanent test.

---

## 12. Deferred (T-13)

All development is local; GitHub hosts the source. The plan below is recorded so that nothing built now contradicts it, and nothing of it is built now.

| Topic | Plan when it comes | Revisit when |
|---|---|---|
| **Continuous integration** | GitHub Actions, the same canonical commands as developers (`cmake --workflow --preset check`), a matrix of three OSes by `debug` and `release`, compiler cache restored between runs, JUnit from doctest in the job summary, the log file and any crash report uploaded on failure. Per pull request: unit, integration, smoke with a virtual display (Xvfb and Mesa llvmpipe on Linux, Mesa's `opengl32.dll` on Windows, native on macOS), `asan` on Linux, fuzz for 60 s per target, lint. Nightly: `tsan`, `rtsan`, fuzz for 30 minutes with the corpus committed back, soak. | A decision to have CI |
| **Reference machine for performance** | Either Amir's machine through `ez bench` with results committed, or a dedicated box. Time-based gates run only there; counter-based gates, zero hot-path allocations and bytes by tag within bounds, can run anywhere. | When one is chosen; §9 then gains a gate and this document is updated |
| **Bench machine for hardware** | A small Linux box as a self-hosted runner labelled `hardware`: an ESP32, an Arduino, a USB-DMX interface looped into a receiver, a MIDI loopback, a light sensor for input-to-photon latency, a USB hub with per-port power control to recover a bricked device. Runs nightly and on demand, never on pull requests from forks, never blocks a merge. | Devices are chosen and CI exists |
| **Which devices first** | Decides which twin and which device pack are built first | Hardware design |
| **End-to-end** | Input recording and replay at the platform seam; programmatic widget access through `ez_ui`; Dear ImGui's test engine evaluated for licence fit | UI vocabulary (U-8) |
| **Visual** | Goldens per renderer, since a software renderer and a GPU differ; few of them; a tolerance, not equality | After end-to-end |
| **Manual release protocol** | A checklist in `docs/` | First release candidate |
| **Property-based tests** for the compositor and parsers | Fuzz covers parsers; a generator library is a dependency to justify | A need appears |
| **`ez test --changed`** | Run only the tests of modules whose files changed | The suite stops fitting in 30 seconds |
| **Pre-push hook** | Optional, never forced: `ez check` before push | A second contributor |

---

## 13. Open questions

### 13.1 To clarify before implementation starts

Nothing blocks. doctest is the one choice to confirm or swap for Catch2; the test files are the same either way.

### 13.2 Deferred

See §12. In addition: the 30-second budget for `ez test` and the per-label timeouts are starting hypotheses to measure once the first modules have tests.

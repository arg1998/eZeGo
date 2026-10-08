# Testing

The design is [`specs/testing.md`](../specs/testing.md). This page is how to run and write tests.

## Running

```bash
./ez test                     # debug tree: unit + integration + smoke, headless, fast
./ez test release             # the same tests in another tree
./ez test debug -L unit       # one label: unit | integration | smoke | lint (check tree) | gpu
./ez test debug -R "base:"    # cases whose name matches a regex
./ez test asan                # same tests under Address and UB sanitizers (builds build/asan)
./ez check                    # the gate: lint + clang-tidy + -Werror + every test, random order
ctest --preset debug -L gpu   # opt-in: needs a display
```

In VS Code, every test case appears in the Testing panel; run or debug one with a click.

Each doctest case is its own CTest test running in its own process, so cases cannot leak state into
each other. A unit case that takes longer than 1 s, or an integration case longer than 10 s, fails.

## Writing a test

Tests mirror the source tree: `tests/<module>/<topic>_test.cpp` tests `src/ez/<module>/`.

```cpp
// tests/log/ring_test.cpp
#include "ez/log/detail/ring.hpp"   // a module's tests may use its detail/

#include <doctest/doctest.h>

namespace {

TEST_CASE("log: a full ring drops the line and counts it") {   // "module: behaviour", a sentence
    // CHECK keeps going and reports every failure; REQUIRE stops the case.
    CHECK(...);
}

TEST_SUITE("integration") {          // the suite name becomes the CTest label
TEST_CASE("log: cvars set the level at startup") { ... }
}

}  // namespace
```

```cmake
# tests/log/CMakeLists.txt: one executable per module, linking that module
ez_test(log SOURCES ring_test.cpp sink_test.cpp)
```

Rules (T-6): no sleeps and no wall clock, no network except loopback, no dependence on the order of
cases. Names are sentences that start with the module.

## Helpers in `tests/support/`

| Helper | Use |
|---|---|
| `ez::test::run_subprocess("name")` with `EZ_TEST_SUBPROCESS(name) { ... }` | Code that must end the process: asserts, fatal paths, fail-fast startup. Returns exit code or signal and the combined output. |
| `ez::test::run_process(path, args, env, timeout_ms)` | Any program, for example the real `ezego` binary |
| `ez::test::TempDir dir{"name"}` | A fresh directory under the build tree; removed when the case passes, kept when it fails |

```cpp
EZ_TEST_SUBPROCESS(assert_fails) {
    EZ_ASSERT_MSG(argc == 100, "argc is never 100");
    return 0;
}

TEST_CASE("base: a failed assert ends the process with a message") {
    const auto r = ez::test::run_subprocess("assert_fails");
    CHECK_FALSE(r.exited_normally());
    CHECK(r.output.find("assertion failed") != std::string::npos);
}
```

## Benchmarks

```bash
./ez bench                     # builds release, runs every benchmark, compares with your last run
./ez bench --filter=log        # only names containing "log"
```

Benchmarks live in `tests/bench/<module>_bench.cpp`; results append to `build/bench/history.jsonl`
with the commit, and the table shows the change against this machine's previous run. There is no
gate: until a reference machine exists the numbers inform, they do not fail anything.

```cpp
EZ_BENCH("log: filtered line") {
    while (state.next()) {          // only the loop is timed
        EZ_LOG_INFO(cvars, "x %d", 1);
    }
}
```

## How discovery works

`ez_test()` in `cmake/testing.cmake` registers a script that runs when `ctest` starts. It asks the
executable for its cases once (`--list-test-cases --reporters=xml`), caches the list next to it,
and lists again only when the executable was rebuilt. Each case runs as
`<exe> --test-case=<exact name>`. A case with `,` in its name is escaped; avoid `*` and `?`, which
doctest treats as wildcards.

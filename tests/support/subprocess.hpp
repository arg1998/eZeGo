// The subprocess runner (specs/testing.md §4): runs a process, captures its combined output and
// exit status, with a timeout. Used for what doctest cannot do in-process: asserts, fatal paths,
// fail-fast startup, smoke tests of the real binaries.
//
//   EZ_TEST_SUBPROCESS(assert_traps) { EZ_ASSERT(false); return 0; }
//
//   TEST_CASE("base: a failed assert ends the process") {
//       const auto r = ez::test::run_subprocess("assert_traps");
//       CHECK_FALSE(r.exited_normally());
//   }
#pragma once

#include "ez/base/macros.hpp"
#include "ez/base/types.hpp"

#include <initializer_list>
#include <string>
#include <vector>

namespace ez::test {

struct ProcessResult {
    i32 exit_code = -1;      // valid when exited
    bool exited = false;     // ended through exit or return from main
    i32 signal = 0;          // POSIX: the signal that ended it; Windows: 0
    bool timed_out = false;  // killed by the runner
    std::string output;      // stdout and stderr, interleaved as written

    [[nodiscard]] bool exited_normally() const noexcept { return exited && !timed_out; }
    [[nodiscard]] bool succeeded() const noexcept { return exited_normally() && exit_code == 0; }
};

// Runs `program` with `args` (not including argv[0]). `env` entries are "NAME=value" pairs added
// to the inherited environment.
ProcessResult run_process(const std::string& program, const std::vector<std::string>& args,
                          const std::vector<std::string>& env = {}, u32 timeout_ms = 10000);

// Runs this test executable again, entering the function registered with EZ_TEST_SUBPROCESS(name)
// instead of the tests. Extra args reach that function's argv after its name.
ProcessResult run_subprocess(const char* name, std::initializer_list<std::string> args = {},
                             const std::vector<std::string>& env = {}, u32 timeout_ms = 10000);

// The path of the running test executable.
const std::string& self_path();

using SubprocessFn = int (*)(int argc, char** argv);

struct SubprocessRegistrar {
    SubprocessRegistrar(const char* name, SubprocessFn fn) noexcept;
};

}  // namespace ez::test

namespace ez::test::detail {
// Called from the support main(): runs a registered subprocess if argv asks for one.
bool dispatch_subprocess(int argc, char** argv, int& exit_code);
void set_self_path(const char* argv0);
}  // namespace ez::test::detail

#define EZ_TEST_SUBPROCESS(name)                                                              \
    static int EZ_CONCAT(ez_subprocess_, name)(int argc, char** argv);                        \
    static const ::ez::test::SubprocessRegistrar EZ_CONCAT(g_ez_subprocess_registrar_, name){ \
        #name, &EZ_CONCAT(ez_subprocess_, name)};                                             \
    static int EZ_CONCAT(ez_subprocess_, name)([[maybe_unused]] int argc, [[maybe_unused]] char** argv)

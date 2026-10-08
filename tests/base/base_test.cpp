#include "ez/base/assert.hpp"
#include "ez/base/build.hpp"
#include "ez/base/detect.h"
#include "ez/base/fixed_string.hpp"
#include "ez/base/hash.hpp"
#include "ez/base/modules.hpp"
#include "ez/base/types.hpp"
#include "support/subprocess.hpp"

#include <doctest/doctest.h>

#include <cstring>

namespace {

using namespace std::string_view_literals;

TEST_CASE("base: exactly one OS, architecture and compiler is detected") {
    CHECK(EZ_OS_LINUX + EZ_OS_MACOS + EZ_OS_WINDOWS == 1);
    CHECK(EZ_ARCH_X64 + EZ_ARCH_ARM64 == 1);
    CHECK(EZ_COMPILER_CLANG + EZ_COMPILER_MSVC + EZ_COMPILER_GCC == 1);
    CHECK(EZ_OS_POSIX == (EZ_OS_LINUX || EZ_OS_MACOS));
    CHECK((EZ_CACHE_LINE == 64 || EZ_CACHE_LINE == 128));
}

TEST_CASE("base: build information comes from the preset") {
    CHECK(std::strlen(ez::build_version) > 0);
    const std::string_view mode = ez::build_mode;
    CHECK((mode == "debug" || mode == "profile" || mode == "release"));
    CHECK(std::strlen(ez::build_compiler) > 4);
    const std::string_view os = ez::build_os;
    CHECK((os == "Linux" || os == "macOS" || os == "Windows"));
    const std::string_view arch = ez::build_arch;
    CHECK((arch == "x86-64" || arch == "arm64"));
}

TEST_CASE("base: count_of counts array elements") {
    const int a[7] = {};
    static_assert(ez::count_of(a) == 7);
    CHECK(ez::count_of(a) == 7);
}

TEST_CASE("base: FixedString truncates, terminates and never allocates") {
    ez::FixedString<8> s;
    CHECK(s.empty());
    CHECK(s.c_str()[0] == '\0');

    CHECK(s.assign("abc"));
    CHECK(s == "abc"sv);
    CHECK(s.size() == 3);

    CHECK_FALSE(s.assign("0123456789"));  // false: truncated
    CHECK(s.view() == "01234567"sv);
    CHECK(s.c_str()[8] == '\0');

    s.clear();
    CHECK(s.empty());

    constexpr ez::FixedString<4> c{"xyz"};
    static_assert(c.size() == 3);
}

TEST_CASE("base: hash64 is FNV-1a and stable across platforms") {
    // Reference values of 64-bit FNV-1a.
    static_assert(ez::hash64("") == 0xcbf29ce484222325ull);
    CHECK(ez::hash64("a") == 0xaf63dc4c8601ec8cull);
    CHECK(ez::hash64("foobar") == 0x85944171f73967e8ull);
    CHECK(ez::hash64("log.drain_ms") != ez::hash64("log.drain_mS"));
}

TEST_CASE("base: the module table is available as an enum") {
    CHECK(ez::module_count >= 1);
    CHECK(ez::module_name(ez::Module::base) == "base"sv);
    CHECK(ez::module_layer(ez::Module::base) == 0);
}

#if EZ_ASSERTS

ez::AssertInfo g_last_assert{};
int g_assert_count = 0;

void recording_handler(const ez::AssertInfo& info) {
    g_last_assert = info;
    ++g_assert_count;
}

EZ_TEST_SUBPROCESS(assert_fails) {
    EZ_ASSERT_MSG(argc == 100, "argc is never 100");
    return 0;
}

TEST_CASE("base: a failed assert reports through the handler") {
    const ez::AssertHandler previous = ez::set_assert_handler(&recording_handler);
    // Only the handler is exercised here: calling the detail function avoids the debug break.
    ez::detail::assert_failed("x > 0", "must be positive", "file.cpp", 12);
    ez::set_assert_handler(previous);
    CHECK(g_assert_count == 1);
    CHECK(g_last_assert.expression == "x > 0"sv);
    CHECK(g_last_assert.message == "must be positive"sv);
    CHECK(g_last_assert.line == 12);
}

TEST_CASE("base: a failed assert ends the process with a message") {
    const ez::test::ProcessResult r = ez::test::run_subprocess("assert_fails");
    CHECK_FALSE(r.exited_normally());
    CHECK(r.output.find("assertion failed: argc == 100 (argc is never 100)") != std::string::npos);
}

#else

TEST_CASE("base: a compiled-out assert does not evaluate its expression") {
    int evaluations = 0;
    EZ_ASSERT(++evaluations > 100);
    CHECK(evaluations == 0);
}

#endif

}  // namespace

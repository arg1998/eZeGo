// The one main() for every test executable (specs/testing.md §4, §5): doctest's implementation is
// compiled once here, and a subprocess request (--ez-subprocess=<name>) is served before doctest.
// DOCTEST_CONFIG_IMPLEMENT is set for this file by tests/support/CMakeLists.txt.
#include "support/subprocess.hpp"

#include <doctest/doctest.h>

int main(int argc, char** argv) {
    ez::test::detail::set_self_path(argv[0]);
    int exit_code = 0;
    if (ez::test::detail::dispatch_subprocess(argc, argv, exit_code)) {
        return exit_code;
    }
    doctest::Context context(argc, argv);
    return context.run();
}

#include "support/subprocess.hpp"

#include <cstring>
#include <filesystem>

namespace ez::test {

namespace {

struct Entry {
    const char* name;
    SubprocessFn fn;
};

// Function-local so registration from other translation units never sees it uninitialised.
std::vector<Entry>& registry() {
    static std::vector<Entry> g_entries;
    return g_entries;
}

std::string& self() {
    static std::string g_self;
    return g_self;
}

constexpr const char* subprocess_flag = "--ez-subprocess=";

}  // namespace

SubprocessRegistrar::SubprocessRegistrar(const char* name, SubprocessFn fn) noexcept {
    registry().push_back({name, fn});
}

const std::string& self_path() {
    return self();
}

ProcessResult run_subprocess(const char* name, std::initializer_list<std::string> args,
                             const std::vector<std::string>& env, u32 timeout_ms) {
    std::vector<std::string> argv;
    argv.push_back(std::string(subprocess_flag) + name);
    argv.insert(argv.end(), args.begin(), args.end());
    return run_process(self_path(), argv, env, timeout_ms);
}

void detail::set_self_path(const char* argv0) {
    std::error_code ec;
    const auto p = std::filesystem::absolute(argv0, ec);
    self() = ec ? std::string(argv0) : p.string();
}

bool detail::dispatch_subprocess(int argc, char** argv, int& exit_code) {
    const usize flag_len = std::strlen(subprocess_flag);
    if (argc < 2 || std::strncmp(argv[1], subprocess_flag, flag_len) != 0) {
        return false;
    }
    const char* name = argv[1] + flag_len;
    for (const Entry& e : registry()) {
        if (std::strcmp(e.name, name) == 0) {
            exit_code = e.fn(argc - 1, argv + 1);  // argv[0] of the function is its name flag
            return true;
        }
    }
    std::fprintf(stderr, "no subprocess named '%s' is registered\n", name);
    exit_code = 127;
    return true;
}

}  // namespace ez::test

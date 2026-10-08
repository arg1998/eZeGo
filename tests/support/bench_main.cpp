// The runner of every bench executable. Arguments:
//   --filter=<text>     only benchmarks whose name contains it
//   --history=<file>    append results as JSON lines; print the delta against this machine's last run
//   --commit=<sha>      recorded with the results
//   --min-ms=<n>        minimum sampling time per benchmark (default 300)
#include "ez/base/build.hpp"
#include "support/bench.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#if EZ_OS_WINDOWS
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace ez::test {

namespace {

struct Entry {
    const char* name;
    BenchFn fn;
};

std::vector<Entry>& benches() {
    static std::vector<Entry> g_benches;
    return g_benches;
}

u64 now_ns() {
    return u64(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
                   .count());
}

std::string machine_name() {
#if EZ_OS_WINDOWS
    const char* n = std::getenv("COMPUTERNAME");
    return n != nullptr ? n : "unknown";
#else
    char buf[256] = {};
    return gethostname(buf, sizeof(buf) - 1) == 0 ? buf : "unknown";
#endif
}

std::string json_escape(const std::string& s) {
    std::string out;
    for (const char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
        }
        out += c;
    }
    return out;
}

// The last recorded median of each benchmark on this machine, read from our own JSON lines.
std::map<std::string, double> previous_medians(const std::string& path, const std::string& machine) {
    std::map<std::string, double> last;
    std::ifstream in(path);
    std::string line;
    const std::string machine_field = "\"machine\":\"" + json_escape(machine) + "\"";
    while (std::getline(in, line)) {
        if (line.find(machine_field) == std::string::npos) {
            continue;
        }
        const usize n = line.find("\"name\":\"");
        const usize m = line.find("\"median_ns\":");
        if (n == std::string::npos || m == std::string::npos) {
            continue;
        }
        const usize name_end = line.find("\",", n + 8);
        last[line.substr(n + 8, name_end - n - 8)] = std::atof(line.c_str() + m + 12);
    }
    return last;
}

std::string arg_value(int argc, char** argv, const char* key, const char* fallback) {
    const usize len = std::strlen(key);
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], key, len) == 0) {
            return argv[i] + len;
        }
    }
    return fallback;
}

}  // namespace

bool BenchState::next() noexcept {
    if (!started_) {
        started_ = true;
        remaining_ = batch_;
        start_ns_ = now_ns();
    }
    if (remaining_ == 0) {
        elapsed_ns_ = now_ns() - start_ns_;
        return false;
    }
    --remaining_;
    return true;
}

BenchRegistrar::BenchRegistrar(const char* name, BenchFn fn) noexcept {
    benches().push_back({name, fn});
}

struct BenchRunner {
    // One sample: per-operation time in ns for a batch of `batch` iterations.
    static double sample(BenchFn fn, u64 batch) {
        BenchState s;
        s.batch_ = batch;
        fn(s);
        return double(s.elapsed_ns_) / double(batch);
    }

    static void run(const Entry& e, u64 min_ns, std::vector<double>& per_op) {
        // Calibrate: grow the batch until one sample takes at least 100 us.
        u64 batch = 1;
        for (;;) {
            BenchState s;
            s.batch_ = batch;
            e.fn(s);
            if (s.elapsed_ns_ >= 100000 || batch >= (u64(1) << 30)) {
                break;
            }
            batch *= 2;
        }
        const u64 start = now_ns();
        while ((now_ns() - start < min_ns || per_op.size() < 10) && per_op.size() < 100000) {
            per_op.push_back(sample(e.fn, batch));
        }
        std::sort(per_op.begin(), per_op.end());
    }
};

}  // namespace ez::test

int main(int argc, char** argv) {
    using namespace ez;
    using namespace ez::test;
    const std::string filter = arg_value(argc, argv, "--filter=", "");
    const std::string history = arg_value(argc, argv, "--history=", "");
    const std::string commit = arg_value(argc, argv, "--commit=", "unknown");
    const u64 min_ns = u64(std::atoll(arg_value(argc, argv, "--min-ms=", "300").c_str())) * 1000000;
    const std::string machine = machine_name();
    const auto previous = history.empty() ? std::map<std::string, double>{} : previous_medians(history, machine);

    std::ofstream out;
    if (!history.empty()) {
        out.open(history, std::ios::app);
    }
    const std::time_t now = std::time(nullptr);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%S", std::localtime(&now));

    std::printf("%-58s %10s %10s %10s %9s\n", "benchmark", "min", "median", "p99", "vs last");
    int ran = 0;
    for (const auto& e : benches()) {
        if (!filter.empty() && std::string(e.name).find(filter) == std::string::npos) {
            continue;
        }
        std::vector<double> per_op;
        BenchRunner::run(e, min_ns, per_op);
        const double min = per_op.front();
        const double median = per_op[per_op.size() / 2];
        const double p99 = per_op[std::min(per_op.size() - 1, per_op.size() * 99 / 100)];
        char delta[32] = "";
        if (const auto it = previous.find(e.name); it != previous.end() && it->second > 0) {
            std::snprintf(delta, sizeof(delta), "%+.1f%%", (median - it->second) / it->second * 100.0);
        }
        std::printf("%-58s %8.2fns %8.2fns %8.2fns %9s\n", e.name, min, median, p99, delta);
        if (out) {
            out << "{\"time\":\"" << stamp << "\",\"commit\":\"" << json_escape(commit) << "\",\"mode\":\""
                << build_mode << "\",\"machine\":\"" << json_escape(machine) << "\",\"name\":\"" << json_escape(e.name)
                << "\",\"min_ns\":" << min << ",\"median_ns\":" << median << ",\"p99_ns\":" << p99
                << ",\"samples\":" << per_op.size() << "}\n";
        }
        ++ran;
    }
    if (std::string(build_mode) != "release") {
        std::printf("\nnote: built in '%s'; numbers are only meaningful from the release tree (ez bench)\n",
                    build_mode);
    }
    return ran > 0 ? 0 : 1;
}

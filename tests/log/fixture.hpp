// Starting the logger in a test, and a sink that captures lines.
#pragma once

#include "ez/cvars/registry.hpp"
#include "ez/log/log.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ez::test {

struct CapturedLine {
    log::Level level;
    log::Category category;
    std::string text;
    std::string thread;
    std::string rendered;
    u32 frame;
    u64 ticks;
};

class CapturedLog {
public:
    CapturedLog() { log::add_sink(&CapturedLog::sink, this); }
    ~CapturedLog() { log::remove_sink(&CapturedLog::sink, this); }
    CapturedLog(const CapturedLog&) = delete;
    CapturedLog& operator=(const CapturedLog&) = delete;

    // Flushes the logger, then returns every line captured so far.
    std::vector<CapturedLine> lines() {
        log::flush();
        const std::lock_guard<std::mutex> lock(mutex_);
        return lines_;
    }
    bool contains(const std::string& text) {
        for (const CapturedLine& l : lines()) {
            if (l.text.find(text) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

private:
    static void sink(void* ctx, const log::Line& line) {
        auto* self = static_cast<CapturedLog*>(ctx);
        const std::lock_guard<std::mutex> lock(self->mutex_);
        self->lines_.push_back({line.level, line.category, std::string(line.text, line.length), line.thread,
                                std::string(line.rendered, line.rendered_length), line.frame, line.ticks});
    }
    std::mutex mutex_;
    std::vector<CapturedLine> lines_;
};

// A registry with the log cvars, initialised with `args` (argv[0] added), then log::init().
// No settings file; the environment is empty unless given.
struct LogSession {
    std::unique_ptr<cvars::Registry> registry = std::make_unique<cvars::Registry>();
    cvars::StartupResult start;

    explicit LogSession(std::vector<std::string> args = {}, std::vector<const char*> env = {}) {
        log::register_cvars(*registry);
        args.insert(args.begin(), "test");
        std::vector<char*> argv;
        for (std::string& a : args) {
            argv.push_back(a.data());
        }
        argv.push_back(nullptr);
        env.push_back(nullptr);
        start =
            registry->init({.argc = int(args.size()), .argv = argv.data(), .envp = env.data(), .settings_path = ""});
        if (start.ok) {
            log::init(*registry);
        }
    }
    ~LogSession() { log::shutdown(); }
    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;
};

}  // namespace ez::test

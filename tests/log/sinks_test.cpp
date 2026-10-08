#include "support/temp_dir.hpp"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "log/fixture.hpp"

namespace {

using namespace ez;
using namespace ez::test;

std::string read_file(const std::filesystem::path& p) {
    std::ifstream in(p);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

std::vector<std::filesystem::path> log_files(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    for (const auto& e : std::filesystem::directory_iterator(dir)) {
        if (e.path().extension() == ".log") {
            files.push_back(e.path());
        }
    }
    return files;
}

TEST_CASE("log: the file sink is off by default") {
    TempDir dir("file_off");
    LogSession session({"--log.file.dir=" + dir.path().string()});
    EZ_LOG_WARN(log, "not in a file");
    log::flush();
    CHECK(log_files(dir.path()).empty());
}

TEST_CASE("log: with log.file on, each session writes one file with a header") {
    TempDir dir("file_on");
    LogSession session({"--log.file", "--log.file.dir=" + dir.path().string()});
    EZ_LOG_WARN(log, "into the file %d", 1);
    log::flush();
    const auto files = log_files(dir.path());
    REQUIRE(files.size() == 1);
    CHECK(files[0].filename().string().rfind("ezego-", 0) == 0);
    const std::string text = read_file(files[0]);  // Warn flushes immediately
    CHECK(text.find("# eZeGo ") == 0);
    CHECK(text.find("into the file 1") != std::string::npos);
}

TEST_CASE("log: log.file can be switched on at runtime") {
    TempDir dir("file_runtime");
    LogSession session({"--log.file.dir=" + dir.path().string()});
    session.registry->set("log.file", "on");
    session.registry->apply_pending();
    log::flush();
    EZ_LOG_ERROR(log, "after switching on");
    log::flush();
    const auto files = log_files(dir.path());
    REQUIRE(files.size() == 1);
    CHECK(read_file(files[0]).find("after switching on") != std::string::npos);
}

TEST_CASE("log: only the newest log.file.keep session files are kept") {
    TempDir dir("file_keep");
    for (int i = 0; i < 5; ++i) {
        dir.write("ezego-2020010" + std::to_string(i) + "-000000.log", "old\n");
    }
    LogSession session({"--log.file", "--log.file.keep=3", "--log.file.dir=" + dir.path().string()});
    log::flush();
    const auto files = log_files(dir.path());
    CHECK(files.size() == 3);
}

TEST_CASE("log: the history ring keeps recent lines, oldest first") {
    LogSession session({"--log.history_kib=1"});
    for (int i = 0; i < 100; ++i) {
        EZ_LOG_INFO(log, "history line %03d", i);
    }
    log::flush();
    std::vector<std::string> lines;
    log::read_history([](void* ctx, const char* text,
                         usize n) { static_cast<std::vector<std::string>*>(ctx)->emplace_back(text, n); },
                      &lines);
    REQUIRE(!lines.empty());
    CHECK(lines.size() < 100);  // 1 KiB holds only the most recent lines
    CHECK(lines.back().find("history line 099") != std::string::npos);
    for (const auto& l : lines) {
        CHECK(l.back() == '\n');  // whole lines only, even after wrapping
    }
}

}  // namespace

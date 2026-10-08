#include "support/temp_dir.hpp"

#include "support/subprocess.hpp"

#include <doctest/doctest.h>

#include <exception>
#include <fstream>
#include <sstream>

namespace ez::test {

namespace {

std::string sanitized(std::string_view name) {
    std::string s(name);
    for (char& c : s) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) {
            c = '_';
        }
    }
    return s;
}

// Tracks whether the running test case has a failed assertion, through doctest's public listener
// interface, so a TempDir can keep its files when its test fails.
thread_local bool t_current_test_failed = false;

struct FailureListener : doctest::IReporter {
    explicit FailureListener(const doctest::ContextOptions&) {}
    void report_query(const doctest::QueryData&) override {}
    void test_run_start() override {}
    void test_run_end(const doctest::TestRunStats&) override {}
    void test_case_start(const doctest::TestCaseData&) override { t_current_test_failed = false; }
    void test_case_reenter(const doctest::TestCaseData&) override {}
    void test_case_end(const doctest::CurrentTestCaseStats&) override {}
    void test_case_exception(const doctest::TestCaseException&) override { t_current_test_failed = true; }
    void subcase_start(const doctest::SubcaseSignature&) override {}
    void subcase_end() override {}
    void log_assert(const doctest::AssertData& data) override {
        if (data.m_failed) {
            t_current_test_failed = true;
        }
    }
    void log_message(const doctest::MessageData&) override {}
    void test_case_skipped(const doctest::TestCaseData&) override {}
};

}  // namespace

DOCTEST_REGISTER_LISTENER("ez_failure_tracker", 1, FailureListener);

TempDir::TempDir(std::string_view name) {
    // <build tree>/tests/tmp/<executable>/<name>: next to the test binaries, never in the user's dirs.
    const std::filesystem::path exe = self_path();
    path_ = exe.parent_path() / "tmp" / exe.stem() / sanitized(name);
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
    std::filesystem::create_directories(path_);
}

TempDir::~TempDir() {
    if (t_current_test_failed || std::uncaught_exceptions() > 0) {
        return;  // keep the files of a failing test for inspection
    }
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
}

std::filesystem::path TempDir::write(std::string_view name, std::string_view content) const {
    const std::filesystem::path p = path_ / name;
    std::filesystem::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return p;
}

std::string TempDir::read(std::string_view name) const {
    std::ifstream in(path_ / name, std::ios::binary);
    if (!in) {
        return "";
    }
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

}  // namespace ez::test

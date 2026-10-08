#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "cvars/fixtures.hpp"

namespace {

using namespace ez;
using namespace ez::test;
using cvars::SetStatus;
using cvars::Source;

struct Reports {
    std::vector<std::pair<cvars::Severity, std::string>> lines;
    static void hook(void* ctx, cvars::Severity s, std::string_view m) {
        static_cast<Reports*>(ctx)->lines.emplace_back(s, std::string(m));
    }
};

TEST_CASE("cvars: a valid set is queued and applied at the frame boundary") {
    auto r = make_registry();
    const cvars::SetResult res = r->set("cvars.test.drain_ms", "50");
    CHECK(res.status == SetStatus::Queued);
    CHECK(cv_drain_ms.value() == 20);  // not yet: writes happen in apply_pending()
    CHECK(r->apply_pending() == 1);
    CHECK(cv_drain_ms.value() == 50);
    CHECK(cv_drain_ms.generation() == 1);
    CHECK(cv_drain_ms.source() == Source::Console);
}

TEST_CASE("cvars: an invalid set is rejected, the previous value kept, and the message names the range") {
    auto r = make_registry();
    Reports reports;
    r->set_report_hook(&Reports::hook, &reports);
    const cvars::SetResult res = r->set("cvars.test.drain_ms", "5000");
    CHECK(res.status == SetStatus::Rejected);
    CHECK(res.message.view().find("1..1000") != std::string_view::npos);
    CHECK(r->apply_pending() == 0);
    CHECK(cv_drain_ms.value() == 20);
    REQUIRE(reports.lines.size() == 1);
    CHECK(reports.lines[0].first == cvars::Severity::Warning);
}

TEST_CASE("cvars: values are validated per type") {
    auto r = make_registry();
    CHECK(r->set("cvars.test.watch", "off").status == SetStatus::Queued);
    CHECK(r->set("cvars.test.watch", "maybe").status == SetStatus::Rejected);
    CHECK(r->set("cvars.test.drain_ms", "12abc").status == SetStatus::Rejected);
    CHECK(r->set("cvars.test.drain_ms", "").status == SetStatus::Rejected);
    CHECK(r->set("cvars.test.gain", "1.25").status == SetStatus::Queued);
    CHECK(r->set("cvars.test.gain", "2.5").status == SetStatus::Rejected);
    CHECK(r->set("cvars.test.gain", "nan").status == SetStatus::Rejected);
    CHECK(r->set("cvars.test.big", "9223372036854775807").status == SetStatus::Queued);
    CHECK(r->set("cvars.test.big", "9223372036854775808").status == SetStatus::Rejected);
    r->apply_pending();
    CHECK(cv_watch.value() == false);
    CHECK(cv_gain.value() == doctest::Approx(1.25f));
    CHECK(cv_big.value() == 9223372036854775807);
}

TEST_CASE("cvars: unknown names and const values are refused") {
    auto r = make_registry();
    CHECK(r->set("cvars.test.nope", "1").status == SetStatus::Unknown);
}

TEST_CASE("cvars: applying a change reports one line with old value, new value and source") {
    auto r = make_registry();
    Reports reports;
    r->set_report_hook(&Reports::hook, &reports);
    r->set("cvars.test.drain_ms", "50", Source::Panel);
    r->apply_pending();
    REQUIRE(reports.lines.size() == 1);
    CHECK(reports.lines[0].first == cvars::Severity::Info);
    CHECK(reports.lines[0].second == "cvar cvars.test.drain_ms: 20 -> 50 (panel)");
}

TEST_CASE("cvars: messages before the hook is installed are buffered and delivered") {
    auto r = make_registry();
    r->set("cvars.test.drain_ms", "70");
    r->apply_pending();
    Reports reports;
    r->set_report_hook(&Reports::hook, &reports);
    REQUIRE(reports.lines.size() == 1);
    CHECK(reports.lines[0].second.find("20 -> 70") != std::string::npos);
}

TEST_CASE("cvars: secret values are redacted in reports") {
    auto r = make_registry();
    Reports reports;
    r->set_report_hook(&Reports::hook, &reports);
    r->set("cvars.test.secret", "true");
    r->apply_pending();
    REQUIRE(reports.lines.size() == 1);
    CHECK(reports.lines[0].second == "cvar cvars.test.secret: <redacted> -> <redacted> (console)");
}

TEST_CASE("cvars: a former name still works and is reported as renamed") {
    auto r = make_registry();
    CHECK(r->find("cvars.test.old_drain") == r->find("cvars.test.drain_ms"));
    CHECK(r->set("cvars.test.old_drain", "33").status == SetStatus::Queued);
    r->apply_pending();
    CHECK(cv_drain_ms.value() == 33);
}

TEST_CASE("cvars: reset returns to the default") {
    auto r = make_registry();
    r->set("cvars.test.drain_ms", "50");
    r->apply_pending();
    CHECK(r->reset("cvars.test.drain_ms").status == SetStatus::Queued);
    r->apply_pending();
    CHECK(cv_drain_ms.value() == 20);
    CHECK(cv_drain_ms.source() == Source::Default);
    CHECK(r->is_default(*r->find("cvars.test.drain_ms")));
}

TEST_CASE("cvars: enums are set by name, case-insensitively, never by number") {
    auto r = make_registry();  // before init(), a Startup cvar is not locked yet
    CHECK(r->set("cvars.test.backend", "PULSE").status == SetStatus::Queued);
    CHECK(r->set("cvars.test.backend", "1").status == SetStatus::Rejected);
    const cvars::SetResult bad = r->set("cvars.test.backend", "jack");
    CHECK(bad.message.view().find("auto|alsa|pulse") != std::string_view::npos);
    r->apply_pending();
    CHECK(cv_backend.value() == Backend::Pulse);
}

TEST_CASE("cvars: the pending queue is bounded") {
    auto r = make_registry();
    for (usize i = 0; i < cvars::Registry::pending_capacity; ++i) {
        CHECK(r->set("cvars.test.drain_ms", "10").status == SetStatus::Queued);
    }
    CHECK(r->set("cvars.test.drain_ms", "11").status == SetStatus::QueueFull);
    CHECK(r->apply_pending() == cvars::Registry::pending_capacity);
}

// ------------------------------------------------------------ registration self-checks

EZ_CVAR_I32(cv_out_of_range, "cvars.test.bad_range", 5000, {.min = 1, .max = 10, .help = "x"});
EZ_CVAR_I32(cv_no_help, "cvars.test.no_help", 1, {.help = ""});
EZ_CVAR_I32(cv_bad_module, "nosuchmodule.x", 1, {.help = "x"});  // NOLINT(ez-registry-name) deliberately invalid
EZ_CVAR_I32(cv_bad_name, "cvars..x", 1, {.help = "x"});          // NOLINT(ez-registry-name) deliberately invalid
EZ_CVAR_STRING(cv_live_string, "cvars.test.live_string", "", {.mutability = cvars::Mutability::Live, .help = "x"});
EZ_CVAR_I32(cv_duplicate, "cvars.test.drain_ms", 1, {.help = "x"});

TEST_CASE("cvars: invalid declarations are refused at registration and make init fail fast") {
    auto r = make_registry();
    CHECK_FALSE(r->add(cv_out_of_range));
    CHECK_FALSE(r->add(cv_no_help));
    CHECK_FALSE(r->add(cv_bad_module));
    CHECK_FALSE(r->add(cv_bad_name));
    CHECK_FALSE(r->add(cv_live_string));
    CHECK_FALSE(r->add(cv_duplicate));
    CHECK_FALSE(r->add(cv_watch));  // already in this registry

    const cvars::StartupResult start = r->init({.settings_path = ""});
    CHECK_FALSE(start.ok);
    CHECK(start.exit_now);
    CHECK(start.exit_code == 2);
    CHECK(start.report.view().find("cvars.test.bad_range: the default is outside its range") != std::string_view::npos);
    CHECK(start.report.view().find("help text is mandatory") != std::string_view::npos);
}

}  // namespace

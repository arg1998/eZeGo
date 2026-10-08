#include "support/temp_dir.hpp"

#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "cvars/fixtures.hpp"

namespace {

using namespace ez;
using namespace ez::test;
using cvars::SetStatus;
using cvars::Source;

// argv as init() expects it; argv[0] is the program name.
struct Args {
    std::vector<std::string> storage;
    std::vector<char*> argv;
    explicit Args(std::initializer_list<std::string> list) : storage(list) {
        storage.insert(storage.begin(), "ezego");
        for (std::string& s : storage) {
            argv.push_back(s.data());
        }
        argv.push_back(nullptr);
    }
    [[nodiscard]] int argc() const { return int(argv.size()) - 1; }
    char** data() { return argv.data(); }
};

cvars::StartupResult start(cvars::Registry& r, Args args, const std::string& settings,
                           std::vector<const char*> env = {}) {
    env.push_back(nullptr);
    return r.init({.argc = args.argc(), .argv = args.data(), .envp = env.data(), .settings_path = settings.c_str()});
}

TEST_CASE("cvars: precedence is defaults < settings file < environment < command line") {
    TempDir dir("precedence");
    const std::string file = dir.write("settings.cfg",
                                       "cvars.test.drain_ms = 30\ncvars.test.gain = 1.5\n"
                                       "cvars.test.watch = false\n")
                                 .string();
    auto r = make_registry();
    const cvars::StartupResult res =
        start(*r, Args{"--cvars.test.drain_ms=50"}, file, {"EZ_CVARS_TEST_DRAIN_MS=40", "EZ_CVARS_TEST_GAIN=0.25"});
    REQUIRE(res.ok);
    CHECK_FALSE(res.exit_now);
    CHECK(cv_drain_ms.value() == 50);  // command line wins
    CHECK(cv_drain_ms.source() == Source::CommandLine);
    CHECK(cv_gain.value() == doctest::Approx(0.25f));  // environment beats the file
    CHECK(cv_gain.source() == Source::Environment);
    CHECK(cv_watch.value() == false);  // file beats the default
    CHECK(cv_watch.source() == Source::File);
    CHECK(cv_ring_kib.value() == 128);  // nothing set it
}

TEST_CASE("cvars: a bad value anywhere at startup fails fast and lists every error") {
    TempDir dir("fail_fast");
    const std::string file = dir.write("settings.cfg", "cvars.test.drain_ms = 9999\nnot a setting line\n").string();
    auto r = make_registry();
    const cvars::StartupResult res = start(*r, Args{"--cvars.test.gain=loud", "--no.such.option=1"}, file);
    CHECK_FALSE(res.ok);
    CHECK(res.exit_now);
    CHECK(res.exit_code == 2);
    CHECK(res.error_count == 4);
    const std::string_view report = res.report.view();
    CHECK(report.find("eZeGo cannot start: 4 invalid settings.") != std::string_view::npos);
    CHECK(report.find("settings.cfg:1: cvars.test.drain_ms: '9999' is not accepted; expected 1..1000") !=
          std::string_view::npos);
    CHECK(report.find("settings.cfg:2: malformed line") != std::string_view::npos);
    CHECK(report.find("command line: cvars.test.gain: 'loud' is not accepted") != std::string_view::npos);
    CHECK(report.find("unknown option '--no.such.option=1'") != std::string_view::npos);
    CHECK(report.find("--reset-settings") != std::string_view::npos);
}

TEST_CASE("cvars: unknown names in the settings file are kept, not errors") {
    TempDir dir("unknown_kept");
    const std::string file = dir.write("settings.cfg", "future.feature = 3\ncvars.test.drain_ms = 30\n").string();
    auto r = make_registry();
    const cvars::StartupResult res = start(*r, Args{}, file);
    REQUIRE(res.ok);
    CHECK(cv_drain_ms.value() == 30);
    r->set("cvars.test.watch", "false");
    r->apply_pending();
    REQUIRE(r->save());
    const std::string saved = dir.read("settings.cfg");
    CHECK(saved.find("future.feature = 3") != std::string::npos);
    CHECK(saved.find("cvars.test.drain_ms = 30") != std::string::npos);
    CHECK(saved.find("cvars.test.watch = false") != std::string::npos);
}

TEST_CASE("cvars: only explicit overrides of Persist cvars are saved, sorted, and they load back") {
    TempDir dir("save");
    const std::string file = (dir / "settings.cfg").string();
    {
        auto r = make_registry();
        REQUIRE(start(*r, Args{"--cvars.test.gain=1.5"}, file).ok);  // command line: not persisted
        r->set("cvars.test.watch", "false");                         // console + Persist: saved
        r->set("cvars.test.drain_ms", "70");
        r->apply_pending();
        CHECK(r->dirty());
        REQUIRE(r->save());
        CHECK_FALSE(r->dirty());
    }
    const std::string saved = dir.read("settings.cfg");
    CHECK(saved.find("cvars.test.gain") == std::string::npos);
    CHECK(saved.find("cvars.test.drain_ms = 70\ncvars.test.watch = false\n") != std::string::npos);
}

TEST_CASE("cvars: Startup cvars lock after init; Persist ones save the change for the next start") {
    TempDir dir("startup_lock");
    const std::string file = (dir / "settings.cfg").string();
    auto r = make_registry();
    REQUIRE(start(*r, Args{}, file).ok);
    const cvars::SetResult a = r->set("cvars.test.backend", "alsa");
    CHECK(a.status == SetStatus::AfterRestart);
    CHECK(cv_backend.value() == Backend::Auto);  // this run keeps its value
    const cvars::SetResult b = r->set("cvars.test.ring_kib", "256");
    CHECK(b.status == SetStatus::NotSettable);
    REQUIRE(r->save());
    CHECK(dir.read("settings.cfg").find("cvars.test.backend = alsa") != std::string::npos);
}

TEST_CASE("cvars: a bool option without a value means true; others need a value") {
    auto r = make_registry();
    const cvars::StartupResult res = start(*r, Args{"--cvars.test.secret", "--cvars.test.drain_ms"}, "");
    CHECK_FALSE(res.ok);
    CHECK(res.error_count == 1);
    CHECK(res.report.view().find("--cvars.test.drain_ms needs a value") != std::string_view::npos);
}

TEST_CASE("cvars: positional arguments and everything after -- belong to the application") {
    auto r = make_registry();
    const cvars::StartupResult res = start(*r, Args{"show.ezp", "--", "--anything"}, "");
    CHECK(res.ok);
}

TEST_CASE("cvars: --help writes generated help and asks to exit with 0") {
    auto r = make_registry();
    std::string help;
    Args args{"--help"};
    const cvars::StartupResult res =
        r->init({.argc = args.argc(), .argv = args.data(), .settings_path = "", .help_out = string_writer(help)});
    CHECK(res.exit_now);
    CHECK(res.exit_code == 0);
    CHECK(help.find("--cvars.test.drain_ms=<1..1000>  default 20") != std::string::npos);
    CHECK(help.find("Drain period in milliseconds.") != std::string::npos);
    CHECK(help.find("--cvars.test.backend=<auto|alsa|pulse>  default auto, startup") != std::string::npos);
}

TEST_CASE("cvars: --version prints the given line and asks to exit with 0") {
    auto r = make_registry();
    std::string out;
    Args args{"--version"};
    const cvars::StartupResult res = r->init({.argc = args.argc(),
                                              .argv = args.data(),
                                              .settings_path = "",
                                              .help_out = string_writer(out),
                                              .version_text = "eZeGo 1.2.3"});
    CHECK(res.exit_now);
    CHECK(res.exit_code == 0);
    CHECK(out == "eZeGo 1.2.3\n");
}

TEST_CASE("cvars: without a version text, --version is an unknown option") {
    auto r = make_registry();
    const cvars::StartupResult res = start(*r, Args{"--version"}, "");
    CHECK_FALSE(res.ok);
}

TEST_CASE("cvars: --reset-settings sets a bad file aside and starts with defaults") {
    TempDir dir("reset_settings");
    const std::string file = dir.write("settings.cfg", "cvars.test.drain_ms = 9999\n").string();
    auto r = make_registry();
    const cvars::StartupResult res = start(*r, Args{"--reset-settings"}, file);
    CHECK(res.ok);
    CHECK(cv_drain_ms.value() == 20);
    CHECK(dir.read("settings.cfg").empty());
    usize backups = 0;
    for (const auto& e : std::filesystem::directory_iterator(dir.path())) {
        backups += e.path().filename().string().rfind("settings.cfg.bad-", 0) == 0 ? 1 : 0;
    }
    CHECK(backups == 1);
}

bool expand_test_shorthand(cvars::Registry& r, std::string_view value, Source source, cvars::Writer errors) {
    // "fast" sets two cvars at once.
    if (value == "fast") {
        return r.stage("cvars.test.drain_ms", "5", source, errors) && r.stage("cvars.test.gain", "2", source, errors);
    }
    errors("expected 'fast'");
    return false;
}

TEST_CASE("cvars: shorthands expand into ordinary sets from the command line and the environment") {
    auto r = make_registry();
    r->add_shorthand("cvars-test", &expand_test_shorthand);
    REQUIRE(start(*r, Args{"--cvars-test=fast"}, "").ok);
    CHECK(cv_drain_ms.value() == 5);
    CHECK(cv_gain.value() == doctest::Approx(2.0f));
}

TEST_CASE("cvars: a shorthand's errors fail startup like any other bad value") {
    auto r = make_registry();  // a cvar belongs to one registry, so this is its own case
    r->add_shorthand("cvars-test", &expand_test_shorthand);
    const cvars::StartupResult bad = start(*r, Args{}, "", {"EZ_CVARS-TEST=slow"});
    CHECK_FALSE(bad.ok);
    CHECK(bad.error_count == 1);
    CHECK(bad.report.view().find("environment: expected 'fast'") != std::string_view::npos);
}

}  // namespace

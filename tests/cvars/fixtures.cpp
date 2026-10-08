#include "cvars/fixtures.hpp"

namespace ez::test {

using cvars::Flags;
using cvars::Mutability;
using cvars::Tier;

constexpr const char* backend_names[] = {"auto", "alsa", "pulse"};

EZ_CVAR_BOOL(cv_watch, "cvars.test.watch", true,
             {.mutability = Mutability::Live, .flags = Flags::Persist, .help = "Watch for devices."});
EZ_CVAR_I32(cv_drain_ms, "cvars.test.drain_ms", 20,
            {.min = 1,
             .max = 1000,
             .mutability = Mutability::Live,
             .tier = Tier::Advanced,
             .flags = Flags::Persist,
             .help = "Drain period in milliseconds.",
             .aliases = "cvars.test.old_drain"});
EZ_CVAR_F32(cv_gain, "cvars.test.gain", 0.5f, {.min = 0, .max = 2, .help = "Output gain."});
EZ_CVAR_I64(cv_big, "cvars.test.big", 5000000000, {.help = "A 64-bit number."});
EZ_CVAR_ENUM(cv_backend, "cvars.test.backend", Backend, Backend::Auto, backend_names,
             {.mutability = Mutability::Startup, .flags = Flags::Persist, .help = "Audio backend."});
EZ_CVAR_STRING(cv_dir, "cvars.test.dir", "", {.mutability = Mutability::Startup, .help = "A directory."});
EZ_CVAR_I32(cv_ring_kib, "cvars.test.ring_kib", 128,
            {.min = 4, .max = 4096, .mutability = Mutability::Startup, .help = "Ring size, read at init."});
EZ_CVAR_BOOL(cv_secret, "cvars.test.secret", false, {.flags = Flags::Secret | Flags::Persist, .help = "A secret."});

std::unique_ptr<cvars::Registry> make_registry() {
    auto r = std::make_unique<cvars::Registry>();
    r->add(cv_watch);
    r->add(cv_drain_ms);
    r->add(cv_gain);
    r->add(cv_big);
    r->add(cv_backend);
    r->add(cv_dir);
    r->add(cv_ring_kib);
    r->add(cv_secret);
    return r;
}

cvars::Writer string_writer(std::string& out) {
    return cvars::Writer{&out, [](void* ctx, std::string_view s) { static_cast<std::string*>(ctx)->append(s); }};
}

}  // namespace ez::test

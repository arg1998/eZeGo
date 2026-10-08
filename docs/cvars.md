# Runtime variables (cvars)

The design is [`specs/cvars.md`](../specs/cvars.md). This page is how to use them.

## Declare, register, read

```cpp
// src/ez/net/net_cvars.cpp: a module's cvars live in its own .cpp, inside its namespace
#include "ez/cvars/registry.hpp"

namespace ez::net {

using cvars::Flags;
using cvars::Mutability;
using cvars::Tier;

EZ_CVAR_BOOL(cv_net_watch, "net.watch", true,
             {.flags = Flags::Persist, .help = "Announce devices as soon as they appear on the network."});
EZ_CVAR_I32(cv_net_port, "net.port", 6454,
            {.min = 1, .max = 65535, .mutability = Mutability::Startup, .tier = Tier::Advanced,
             .flags = Flags::Persist, .help = "UDP port to listen on. Takes effect after restart."});

void register_cvars(cvars::Registry& r) {   // called from the one list in main()
    r.add(cv_net_watch);
    r.add(cv_net_port);
}

}  // namespace ez::net
```

| Rule | Why |
|---|---|
| Object name starts with `cv_`, registry name with the module | `ez lint` checks both (naming.md N-6, N-9) |
| `.help` is mandatory, the default must be in range | Otherwise `init()` refuses to start and says which declaration is wrong |
| Strings are `Startup` | They cannot be replaced atomically |
| Read with `.value()` | One load, any thread; copy to a local before a hot loop |
| React to a change by comparing `.generation()` once per frame or poll | The registry never calls back into modules |

## The application side

```cpp
auto& r = ez::cvars::registry();
ez::net::register_cvars(r);                 // one visible list, in module order
const auto start = r.init({.argc = argc, .argv = argv});
if (start.exit_now) {                       // --help, or invalid settings (fail fast)
    std::fputs(start.report.c_str(), stderr);
    return start.exit_code;
}
// every frame, main thread:   r.apply_pending();
// on shutdown:                r.save();
```

## Setting values

| From | Form | When |
|---|---|---|
| Settings file | `net.port = 7000` in the per-user `settings.cfg` | Startup |
| Environment | `EZ_NET_PORT=7000` | Startup |
| Command line | `--net.port=7000`, `--net.watch` (bool means true) | Startup |
| Console | `net.watch off`, `find port`, `reset net.watch`, `dump` | Runtime, applied at the next frame |

Later sources win at startup: defaults, then file, then environment, then command line. A bad value
at startup stops the program with every error listed; `--reset-settings` sets the file aside.
At runtime a bad value is rejected with a message and the previous value stays.

Only values you change are saved; when a default changes in a new version, you get the new default.

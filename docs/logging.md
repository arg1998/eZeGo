# Logging

The design is [`specs/logging.md`](../specs/logging.md). This page is how to use it.

## Logging a line

```cpp
#include "ez/log/log.hpp"

EZ_LOG_INFO(net, "listening on %u, universes %u..%u", port, first, last);   // category = module name
EZ_LOG_WARN_EVERY(100, net, "frame late by %.2f ms", late_ms);              // flood guard
EZ_LOG_RT(audio, Warn, "dropout, frames missed", missed);                   // audio thread: no formatting
EZ_LOG_FATAL(app, "project '%s' failed to load", path);                     // never returns
```

| Level | For | Rule |
|---|---|---|
| Fatal | the process cannot continue | once |
| Error | an operation failed and the user would want to know | per event |
| Warn | degraded but working | per event, flood-guarded |
| Info | lifecycle milestones | a few per second at most |
| Debug | event detail for one module | never per frame |
| Trace | flow detail | the only per-frame level; exists in debug builds only |

Logs are for events; anything that happens at a rate is a metric. Arguments are what printf accepts:
pass `.c_str()` for strings and cast scoped enums.

## Controlling it

```bash
./ezego --log=all:debug,net:trace       # or EZ_LOG=... in the environment
./ezego --log.file                      # also write a session file (off by default)
```

```text
log.level.net trace          # in the console, live
log.file on
log.drain_ms 50
```

| Cvar | Default | |
|---|---|---|
| `log.level.all`, `log.level.<module>` | trace in debug, info in profile and release; modules inherit | live |
| `log.file`, `log.file.dir`, `log.file.keep`, `log.file.flush_ms` | off, per-user state dir, 10, 1000 ms | |
| `log.max_line` | 512 bytes, up to 4096 | live |
| `log.drain_ms` | 20 ms | live |
| `log.stderr`, `log.color`, `log.sync` | on, auto, auto | `sync` writes on the calling thread; auto = when a debugger is attached |
| `log.ring_kib`, `log.threads_max`, `log.history_kib` | 128, 16, 256 | startup |

Lines below the build's floor do not exist in the binary: debug keeps everything, profile and
release keep Info and up.

## Setting it up in an executable

```cpp
auto& r = ez::cvars::registry();
ez::log::register_cvars(r);
// ... other modules' register_cvars
if (const auto start = r.init({.argc = argc, .argv = argv}); start.exit_now) { /* print, exit */ }
ez::log::init(r);                 // after cvars; the calling thread is named "main"
ez::log::register_thread("audio"); // in each thread you create, to name it
ez::log::set_frame(frame);         // once per frame
ez::log::shutdown();               // writes what is queued
```

## Seams for later modules

| Seam | Used by |
|---|---|
| `set_profiler_hook` | the profiler backend: every line at the call site, for Tracy messages |
| `set_fatal_hook` | the crash reporter |
| `add_sink` | the in-app console, tests (`tests/log/fixture.hpp` has a capturing sink) |
| `read_history` | the console panel and crash reports |

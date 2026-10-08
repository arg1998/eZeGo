// Logging (specs/logging.md). The whole caller-facing API:
//
//   EZ_LOG_TRACE(category, "format literal", args...)   one macro per level; category is a module name
//   EZ_LOG_DEBUG / EZ_LOG_INFO / EZ_LOG_WARN / EZ_LOG_ERROR
//   EZ_LOG_FATAL(category, ...)                         synchronous, never returns (LG-11)
//   EZ_LOG_<LEVEL>_ONCE(category, ...)                  flood guards: first hit at this call site,
//   EZ_LOG_<LEVEL>_EVERY(n, category, ...)              or every n-th hit
//   EZ_LOG_RT(category, Level, "literal", value)        realtime threads: no formatting, wait-free
//
//   EZ_LOG_INFO(net, "listening on %u, universes %u..%u", port, first, last);
//
// Caller rules (LG-2), enforced at compile time so the implementation can change underneath:
//   1. The format string is a string literal.
//   2. Arguments are what printf accepts: integers, floats, pointers, C strings. Objects such as
//      std::string do not compile; pass .c_str(). Scoped enums are cast by the caller.
//   3. Arguments are evaluated once, and only when the line is enabled.
//   4. A line longer than log.max_line is truncated and ends with "...".
//
// Below the build floor (EZ_LOG_LEVEL) a macro compiles to nothing. Above it, a filtered line costs
// one relaxed load and a compare; an enabled line formats into this thread's ring (no allocation,
// no lock, no I/O) and the log thread writes it out within log.drain_ms.
#pragma once

#include "ez/base/build.hpp"
#include "ez/base/detect.h"
#include "ez/base/macros.hpp"
#include "ez/base/modules.hpp"
#include "ez/base/types.hpp"

#include <atomic>
#include <cstdio>
#include <type_traits>

namespace ez::cvars {  // NOLINT(ez-namespace) forward declaration of a dependency's type
class Registry;
}  // namespace ez::cvars

namespace ez::log {

// Ascending severity; the numbers match EZ_LOG_LEVEL (LG-3).
enum class Level : u8 { Trace, Debug, Info, Warn, Error, Fatal };

// One category per module, generated from modules.cmake (LG-4); 64..127 are reserved for plugins.
enum class Category : u8 {
#define EZ_DETAIL_LOG_CATEGORY(name, text, layer) name,
    EZ_MODULES(EZ_DETAIL_LOG_CATEGORY)
#undef EZ_DETAIL_LOG_CATEGORY
};
inline constexpr usize category_capacity = 128;
inline constexpr u8 plugin_category_first = 64;
static_assert(module_count < plugin_category_first);

const char* category_name(Category c) noexcept;
const char* level_name(Level l) noexcept;

// ------------------------------------------------------------------ lifecycle
// Adds the log.* cvars and the --log= / EZ_LOG shorthand. Before cvars::Registry::init().
void register_cvars(cvars::Registry& registry) noexcept;
// After cvars init: allocates every ring, the batch and history buffers once, installs itself as
// the cvars report receiver, and starts the log thread. Lines logged before init are written to
// stderr synchronously.
bool init(cvars::Registry& registry) noexcept;
// Drains every ring, closes the file and stops the log thread. Lines after it go to stderr directly.
void shutdown() noexcept;
// Blocks until every line committed before the call has reached the sinks. Not for hot paths.
void flush() noexcept;

// Names this thread in log lines and gives it a ring now rather than on its first line.
void register_thread(const char* name) noexcept;
// The engine frame counter recorded with every line; the loop calls it once per frame.
void set_frame(u32 frame) noexcept;

// ------------------------------------------------------------------ extension points
struct Line {
    Level level;
    Category category;
    u64 ticks;  // reference clock, nanoseconds
    u32 frame;
    const char* thread;  // thread name
    const char* file;
    u32 line;
    const char* text;  // the message, not terminated
    usize length;
    const char* rendered;  // the full console line without colour, newline included
    usize rendered_length;
};

// Extra sinks, called on the log thread for every line in timestamp order. At most 4.
using SinkFn = void (*)(void* context, const Line& line);
bool add_sink(SinkFn fn, void* context) noexcept;
void remove_sink(SinkFn fn, void* context) noexcept;

// The profiler seam (LG-4 §4.4): called at the call site, on the calling thread, with the formatted
// text, so the profiler stamps it with the right time and zone. The profiler backend installs it.
using ProfilerHook = void (*)(const char* text, usize length, Level level);
void set_profiler_hook(ProfilerHook hook) noexcept;

// The fatal seam (LG-11): called with the fatal line after it was written to stderr and the rings
// were drained. The crash reporter installs itself here; the default ends the process.
using FatalHook = void (*)(const char* text, usize length);
void set_fatal_hook(FatalHook hook) noexcept;

// The history ring: the last log.history_kib of rendered lines, oldest first.
void read_history(void (*fn)(void* context, const char* text, usize length), void* context) noexcept;

struct Stats {
    u64 lines;    // written to the sinks
    u64 dropped;  // lost to a full ring or an exhausted pool
};
Stats stats() noexcept;

}  // namespace ez::log

// ------------------------------------------------------------------ the hot path
namespace ez::log::detail {
// One byte per category; the log thread recomputes it from the level cvars.
alignas(EZ_CACHE_LINE) inline std::atomic<u8> g_levels[category_capacity] = {};
}  // namespace ez::log::detail

namespace ez::log {

[[nodiscard]] EZ_FORCE_INLINE bool enabled(Category c, Level l) noexcept {
    return u8(l) >= detail::g_levels[u8(c)].load(std::memory_order_relaxed);
}

void set_level(Category c, Level l) noexcept;  // for tests and tools; cvars normally own levels

}  // namespace ez::log

namespace ez::log::detail {

template <class T>
inline constexpr bool loggable = std::is_arithmetic_v<T> || std::is_pointer_v<T> || std::is_null_pointer_v<T> ||
                                 (std::is_enum_v<T> && std::is_convertible_v<T, int>);

// Never called: lets -Wformat check every log line's arguments against its format string.
EZ_PRINTF_FORMAT(1, 2) inline void check_format(const char*, ...) noexcept {}

struct Reservation {
    char* text;      // where the formatted text goes
    usize capacity;  // bytes available, terminator included
    void* record;    // opaque to callers
};
bool begin(Reservation& r, Level level, Category c, const char* file, u32 line) noexcept;
void commit(Reservation& r, int formatted_length) noexcept;
void emit_rt(Category c, Level level, const char* literal, u64 value) noexcept;
[[noreturn]] void fatal(Category c, const char* file, u32 line, const char* text) noexcept;

#if EZ_COMPILER_CLANG || EZ_COMPILER_GCC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif
template <Level L, class... Args>
void emit(Category c, const char* file, u32 line, const char* fmt, const Args&... args) noexcept {
    static_assert((loggable<std::decay_t<Args>> && ...),
                  "log arguments are what printf accepts: integers, floats, pointers, C strings (use .c_str())");
    Reservation r;
    if (begin(r, L, c, file, line)) {
        commit(r, std::snprintf(r.text, r.capacity, fmt, args...));
    }
}

template <class... Args>
[[noreturn]] EZ_NO_INLINE void emit_fatal(Category c, const char* file, u32 line, const char* fmt,
                                          const Args&... args) noexcept {
    char text[4096];
    std::snprintf(text, sizeof(text), fmt, args...);
    fatal(c, file, line, text);
}
#if EZ_COMPILER_CLANG || EZ_COMPILER_GCC
    #pragma GCC diagnostic pop
#endif

}  // namespace ez::log::detail

// ------------------------------------------------------------------ macros
#define EZ_DETAIL_LOG(floor, lvl, cat, fmt, ...)                                                             \
    do {                                                                                                     \
        if constexpr (EZ_LOG_LEVEL <= (floor)) {                                                             \
            if (::ez::log::enabled(::ez::log::Category::cat, ::ez::log::Level::lvl)) {                       \
                ::ez::log::detail::emit<::ez::log::Level::lvl>(::ez::log::Category::cat, __FILE__, __LINE__, \
                                                               "" fmt "" __VA_OPT__(, ) __VA_ARGS__);        \
            }                                                                                                \
            if (false) {                                                                                     \
                ::ez::log::detail::check_format("" fmt "" __VA_OPT__(, ) __VA_ARGS__);                       \
            }                                                                                                \
        }                                                                                                    \
    } while (0)

#define EZ_LOG_TRACE(cat, fmt, ...) EZ_DETAIL_LOG(0, Trace, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_DEBUG(cat, fmt, ...) EZ_DETAIL_LOG(1, Debug, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_INFO(cat, fmt, ...) EZ_DETAIL_LOG(2, Info, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_WARN(cat, fmt, ...) EZ_DETAIL_LOG(3, Warn, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_ERROR(cat, fmt, ...) EZ_DETAIL_LOG(4, Error, cat, fmt __VA_OPT__(, ) __VA_ARGS__)

// Fatal is never compiled out and never returns.
#define EZ_LOG_FATAL(cat, fmt, ...)                                                 \
    do {                                                                            \
        if (false) {                                                                \
            ::ez::log::detail::check_format("" fmt "" __VA_OPT__(, ) __VA_ARGS__);  \
        }                                                                           \
        ::ez::log::detail::emit_fatal(::ez::log::Category::cat, __FILE__, __LINE__, \
                                      "" fmt "" __VA_OPT__(, ) __VA_ARGS__);        \
    } while (0)

// Flood guards: a per-call-site counter, about a nanosecond per suppressed hit.
#define EZ_DETAIL_LOG_ONCE(macro, cat, fmt, ...)                        \
    do {                                                                \
        static std::atomic<bool> g_ez_log_once{false};                  \
        if (!g_ez_log_once.load(std::memory_order_relaxed) &&           \
            !g_ez_log_once.exchange(true, std::memory_order_relaxed)) { \
            macro(cat, fmt __VA_OPT__(, ) __VA_ARGS__);                 \
        }                                                               \
    } while (0)
#define EZ_DETAIL_LOG_EVERY(macro, n, cat, fmt, ...)                             \
    do {                                                                         \
        static std::atomic<::ez::u32> g_ez_log_every{0};                         \
        if (g_ez_log_every.fetch_add(1, std::memory_order_relaxed) % (n) == 0) { \
            macro(cat, fmt __VA_OPT__(, ) __VA_ARGS__);                          \
        }                                                                        \
    } while (0)

#define EZ_LOG_TRACE_ONCE(cat, fmt, ...) EZ_DETAIL_LOG_ONCE(EZ_LOG_TRACE, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_DEBUG_ONCE(cat, fmt, ...) EZ_DETAIL_LOG_ONCE(EZ_LOG_DEBUG, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_INFO_ONCE(cat, fmt, ...) EZ_DETAIL_LOG_ONCE(EZ_LOG_INFO, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_WARN_ONCE(cat, fmt, ...) EZ_DETAIL_LOG_ONCE(EZ_LOG_WARN, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_ERROR_ONCE(cat, fmt, ...) EZ_DETAIL_LOG_ONCE(EZ_LOG_ERROR, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_TRACE_EVERY(n, cat, fmt, ...) EZ_DETAIL_LOG_EVERY(EZ_LOG_TRACE, n, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_DEBUG_EVERY(n, cat, fmt, ...) EZ_DETAIL_LOG_EVERY(EZ_LOG_DEBUG, n, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_INFO_EVERY(n, cat, fmt, ...) EZ_DETAIL_LOG_EVERY(EZ_LOG_INFO, n, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_WARN_EVERY(n, cat, fmt, ...) EZ_DETAIL_LOG_EVERY(EZ_LOG_WARN, n, cat, fmt __VA_OPT__(, ) __VA_ARGS__)
#define EZ_LOG_ERROR_EVERY(n, cat, fmt, ...) EZ_DETAIL_LOG_EVERY(EZ_LOG_ERROR, n, cat, fmt __VA_OPT__(, ) __VA_ARGS__)

// Realtime variant (§4.6): a 48-byte record with the literal's address and one integer; the log
// thread renders it as "literal = value". No formatting on the caller, wait-free.
#define EZ_LOG_RT(cat, lvl, literal, value)                                                                \
    do {                                                                                                   \
        if constexpr (int(EZ_LOG_LEVEL) <= int(::ez::log::Level::lvl)) {                                   \
            if (::ez::log::enabled(::ez::log::Category::cat, ::ez::log::Level::lvl)) {                     \
                ::ez::log::detail::emit_rt(::ez::log::Category::cat, ::ez::log::Level::lvl, "" literal "", \
                                           ::ez::u64(value));                                              \
            }                                                                                              \
        }                                                                                                  \
    } while (0)

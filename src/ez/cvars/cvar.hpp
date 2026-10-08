// Declaring and reading cvars (specs/cvars.md CV-2, CV-3, CV-4).
//
//   // in the module's own .cpp, inside its namespace
//   EZ_CVAR_I32(cv_log_drain_ms, "log.drain_ms", 20,
//       {.min = 1, .max = 1000, .mutability = Mutability::Live, .tier = Tier::Advanced,
//        .flags = Flags::Persist, .help = "How often the log thread writes queued lines, in ms."});
//
//   // anywhere, any thread: one relaxed load from a fixed address
//   if (cv_hw_watch.value()) { ... }
//
// A cvar object is constant-initialised (no constructor runs) and must be registered once with
// Registry::add() from the module's register function (CV-11). Reading never touches metadata.
#pragma once

#include "ez/base/fixed_string.hpp"
#include "ez/base/macros.hpp"
#include "ez/base/types.hpp"

#include <atomic>
#include <limits>
#include <string_view>
#include <type_traits>

namespace ez::cvars {

enum class Type : u8 { Bool, I32, I64, F32, F64, Enum, String };

// Who may set a cvar, and when the change takes effect (CV-4).
enum class Mutability : u8 {
    Const,    // baked at build time; never settable
    Startup,  // settings file, environment, command line; locked once init completes
    Live,     // also console and panel; applied at the next frame boundary
};

// Which interfaces show a cvar (CV-10). Visibility only: the command line and the file always work.
enum class Tier : u8 { User, Advanced, Developer, Hidden };

enum class Flags : u32 {
    None = 0,
    Persist = 1u << 0,       // explicit overrides are saved to the settings file
    Secret = 1u << 1,        // redacted wherever the value would be printed
    ShowCritical = 1u << 2,  // reserved: refuse changes during a running show (CV deferred)
};
constexpr Flags operator|(Flags a, Flags b) noexcept {
    return Flags(u32(a) | u32(b));
}
constexpr bool has(Flags set, Flags f) noexcept {
    return (u32(set) & u32(f)) != 0;
}

// Where the current value came from (spec §6.3).
enum class Source : u8 { Default, File, Environment, CommandLine, Console, Panel, Plugin };

// The designated-initializer part of a declaration.
struct Options {
    f64 min = -std::numeric_limits<f64>::infinity();  // numbers: inclusive range
    f64 max = std::numeric_limits<f64>::infinity();
    f64 step = 0;  // UI hint for sliders; 0 = none
    Mutability mutability = Mutability::Live;
    Tier tier = Tier::User;
    Flags flags = Flags::None;
    const char* help = "";     // one sentence, mandatory (checked at registration)
    const char* aliases = "";  // former names, comma-separated; accepted with a deprecation warning
};

// Everything the cold paths need, in read-only memory.
struct Meta {
    const char* name = "";
    Type type = Type::Bool;
    Options options{};
    i64 default_int = 0;    // Bool, I32, I64, Enum (index into enum_names)
    f64 default_float = 0;  // F32, F64
    const char* default_text = "";
    const char* const* enum_names = nullptr;
    u32 enum_count = 0;
};

// Per-cvar state that is not the value. Written by the registry on the main thread only.
struct State {
    std::atomic<u32> generation{0};  // incremented on every change; owners compare it (CV-5)
    std::atomic<u8> source{u8(Source::Default)};
    const Meta* meta = nullptr;
    bool locked = false;  // Startup cvars after init
    bool registered = false;
};

// Scalar cvar: bool, i32, i64, f32, f64.
template <class T>
class CVar {
    static_assert(std::is_same_v<T, bool> || std::is_same_v<T, i32> || std::is_same_v<T, i64> ||
                  std::is_same_v<T, f32> || std::is_same_v<T, f64>);

public:
    constexpr CVar(T initial, const Meta* meta) noexcept : value_(initial) { state_.meta = meta; }
    CVar(const CVar&) = delete;
    CVar& operator=(const CVar&) = delete;

    // The hot path: one relaxed load, safe on any thread including the audio thread.
    [[nodiscard]] EZ_FORCE_INLINE T value() const noexcept { return value_.load(std::memory_order_relaxed); }
    [[nodiscard]] EZ_FORCE_INLINE u32 generation() const noexcept {
        return state_.generation.load(std::memory_order_acquire);
    }
    [[nodiscard]] Source source() const noexcept { return Source(state_.source.load(std::memory_order_relaxed)); }
    [[nodiscard]] const Meta& meta() const noexcept { return *state_.meta; }

private:
    friend struct Access;
    std::atomic<T> value_;  // offset 0
    State state_;
};

// Enum cvar: stored as the enumerator's index, read as the enum type. The enum's values must be
// 0, 1, 2 ... in the same order as its name list.
template <class E>
class CVarEnum {
    static_assert(std::is_enum_v<E>);

public:
    constexpr CVarEnum(E initial, const Meta* meta) noexcept : storage_(i32(initial), meta) {}
    [[nodiscard]] EZ_FORCE_INLINE E value() const noexcept { return E(storage_.value()); }
    [[nodiscard]] EZ_FORCE_INLINE u32 generation() const noexcept { return storage_.generation(); }
    [[nodiscard]] Source source() const noexcept { return storage_.source(); }
    [[nodiscard]] const Meta& meta() const noexcept { return storage_.meta(); }

private:
    friend struct Access;
    CVar<i32> storage_;
};

// String cvar: fixed capacity, no allocation. Strings cannot be replaced atomically, so a string
// cvar is either Startup or read only on the main thread (CV-6; checked at registration).
class CVarString {
public:
    static constexpr usize capacity = 255;

    constexpr CVarString(std::string_view initial, const Meta* meta) noexcept : value_(initial) { state_.meta = meta; }
    CVarString(const CVarString&) = delete;
    CVarString& operator=(const CVarString&) = delete;

    [[nodiscard]] std::string_view value() const noexcept { return value_.view(); }
    [[nodiscard]] const char* c_str() const noexcept { return value_.c_str(); }
    [[nodiscard]] u32 generation() const noexcept { return state_.generation.load(std::memory_order_acquire); }
    [[nodiscard]] Source source() const noexcept { return Source(state_.source.load(std::memory_order_relaxed)); }
    [[nodiscard]] const Meta& meta() const noexcept { return *state_.meta; }

private:
    friend struct Access;
    FixedString<capacity> value_;
    State state_;
};

// The registry's door into cvar internals. Nothing else uses it.
struct Access {
    template <class T>
    static std::atomic<T>& value(CVar<T>& c) noexcept {
        return c.value_;
    }
    template <class T>
    static State& state(CVar<T>& c) noexcept {
        return c.state_;
    }
    template <class E>
    static CVar<i32>& storage(CVarEnum<E>& c) noexcept {
        return c.storage_;
    }
    static FixedString<CVarString::capacity>& text(CVarString& c) noexcept { return c.value_; }
    static State& state(CVarString& c) noexcept { return c.state_; }
};

// Builds metadata at compile time. Used by the EZ_CVAR_* macros.
template <class T>
constexpr Meta make_meta(const char* name, T initial, Options options) noexcept {
    Meta m{};
    m.name = name;
    m.options = options;
    if constexpr (std::is_same_v<T, bool>) {
        m.type = Type::Bool;
        m.default_int = initial ? 1 : 0;
    } else if constexpr (std::is_same_v<T, i32>) {
        m.type = Type::I32;
        m.default_int = initial;
    } else if constexpr (std::is_same_v<T, i64>) {
        m.type = Type::I64;
        m.default_int = initial;
    } else if constexpr (std::is_same_v<T, f32>) {
        m.type = Type::F32;
        m.default_float = initial;
    } else {
        static_assert(std::is_same_v<T, f64>);
        m.type = Type::F64;
        m.default_float = initial;
    }
    return m;
}

template <class E, usize N>
constexpr Meta make_meta_enum(const char* name, E initial, const char* const (&names)[N], Options options) noexcept {
    Meta m{};
    m.name = name;
    m.type = Type::Enum;
    m.options = options;
    m.default_int = i64(initial);
    m.enum_names = names;
    m.enum_count = u32(N);
    return m;
}

constexpr Meta make_meta_string(const char* name, const char* initial, Options options) noexcept {
    Meta m{};
    m.name = name;
    m.type = Type::String;
    m.options = options;
    m.default_text = initial;
    return m;
}

}  // namespace ez::cvars

// Declarations. `var` is the object's identifier and starts with cv_ (naming.md N-6); `name` is the
// dotted registry name and starts with the owning module (N-9). The last argument is an Options
// initializer list: {.help = "...", ...}.
#define EZ_DETAIL_CVAR(cvar_type, meta_expr, var, initial)         \
    constexpr ::ez::cvars::Meta EZ_CONCAT(var, _meta) = meta_expr; \
    constinit cvar_type var {                                      \
        initial, &EZ_CONCAT(var, _meta)                            \
    }

#define EZ_CVAR_BOOL(var, name, initial, ...) \
    EZ_DETAIL_CVAR(::ez::cvars::CVar<bool>,   \
                   ::ez::cvars::make_meta<bool>(name, initial, ::ez::cvars::Options __VA_ARGS__), var, initial)
#define EZ_CVAR_I32(var, name, initial, ...)                                                                           \
    EZ_DETAIL_CVAR(::ez::cvars::CVar<::ez::i32>,                                                                       \
                   ::ez::cvars::make_meta<::ez::i32>(name, ::ez::i32(initial), ::ez::cvars::Options __VA_ARGS__), var, \
                   ::ez::i32(initial))
#define EZ_CVAR_I64(var, name, initial, ...)                                                                           \
    EZ_DETAIL_CVAR(::ez::cvars::CVar<::ez::i64>,                                                                       \
                   ::ez::cvars::make_meta<::ez::i64>(name, ::ez::i64(initial), ::ez::cvars::Options __VA_ARGS__), var, \
                   ::ez::i64(initial))
#define EZ_CVAR_F32(var, name, initial, ...)                                                                           \
    EZ_DETAIL_CVAR(::ez::cvars::CVar<::ez::f32>,                                                                       \
                   ::ez::cvars::make_meta<::ez::f32>(name, ::ez::f32(initial), ::ez::cvars::Options __VA_ARGS__), var, \
                   ::ez::f32(initial))
#define EZ_CVAR_F64(var, name, initial, ...)                                                                           \
    EZ_DETAIL_CVAR(::ez::cvars::CVar<::ez::f64>,                                                                       \
                   ::ez::cvars::make_meta<::ez::f64>(name, ::ez::f64(initial), ::ez::cvars::Options __VA_ARGS__), var, \
                   ::ez::f64(initial))
// `names` is a constexpr array of the enumerators' names in value order: {"auto", "alsa", ...}.
#define EZ_CVAR_ENUM(var, name, EnumType, initial, names, ...)                                                         \
    EZ_DETAIL_CVAR(::ez::cvars::CVarEnum<EnumType>,                                                                    \
                   ::ez::cvars::make_meta_enum<EnumType>(name, initial, names, ::ez::cvars::Options __VA_ARGS__), var, \
                   initial)
#define EZ_CVAR_STRING(var, name, initial, ...) \
    EZ_DETAIL_CVAR(::ez::cvars::CVarString,     \
                   ::ez::cvars::make_meta_string(name, initial, ::ez::cvars::Options __VA_ARGS__), var, initial)

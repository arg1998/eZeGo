// The cvar registry (specs/cvars.md): lookup, validation, startup sources, the pending-write queue,
// persistence, and the text interface used by the console and the settings panel.
//
// Startup, in main():
//
//   auto& r = ez::cvars::registry();
//   ez::log::register_cvars(r);            // one visible list, in module order (CV-11)
//   const auto start = r.init({.argc = argc, .argv = argv});
//   if (start.exit_now) { std::fputs(start.report.c_str(), start.exit_code == 0 ? stdout : stderr); return
//   start.exit_code; }
//
// Every frame, on the main thread: r.apply_pending();
// On shutdown: r.save();
//
// Everything here is the cold path and runs on the main thread. Reading a cvar never touches it.
#pragma once

#include "ez/base/fixed_string.hpp"
#include "ez/base/types.hpp"
#include "ez/cvars/cvar.hpp"

#include <string_view>

namespace ez::cvars {

// Where text output goes: the console, a test buffer, stdout.
struct Writer {
    void* context = nullptr;
    void (*write)(void* context, std::string_view text) = nullptr;
    void operator()(std::string_view text) const {
        if (write != nullptr) {
            write(context, text);
        }
    }
};

enum class Severity : u8 { Info, Warning, Error };

// Receives the registry's messages: changes (Info), unknown or deprecated names (Warning). The
// logger installs itself here; until then messages are buffered (spec §5, §13).
using ReportFn = void (*)(void* context, Severity severity, std::string_view message);

struct InitOptions {
    int argc = 0;
    char** argv = nullptr;
    // "NAME=value" entries, null-terminated; nullptr means the process environment.
    const char* const* envp = nullptr;
    // Settings file; nullptr means the per-user default location, "" means no file.
    const char* settings_path = nullptr;
    // Where --help and --version write; the default is stdout.
    Writer help_out{};
    // The line --version prints; nullptr means --version is not an option.
    const char* version_text = nullptr;
};

struct StartupResult {
    bool ok = true;         // no invalid value anywhere
    bool exit_now = false;  // --help or --version was handled, or an error: print `report`, exit with `exit_code`
    int exit_code = 0;
    u32 error_count = 0;
    FixedString<8191> report;  // every error, one per line
};

enum class SetStatus : u8 {
    Queued,        // valid; applied at the next frame boundary
    AfterRestart,  // a locked Startup cvar: the persisted value was updated
    Rejected,      // invalid value; the previous value is kept
    Unknown,       // no cvar has this name
    NotSettable,   // Const, or a Startup cvar without Persist after init
    QueueFull,     // too many writes this frame
};

struct SetResult {
    SetStatus status = SetStatus::Unknown;
    FixedString<255> message;  // shown to the user next to the input
    [[nodiscard]] bool accepted() const noexcept {
        return status == SetStatus::Queued || status == SetStatus::AfterRestart;
    }
};

// One registered cvar, type-erased. The console and the panel iterate these.
struct Entry {
    const Meta* meta = nullptr;
    void* object = nullptr;  // CVar<T>, the storage of a CVarEnum, or CVarString
    State* state = nullptr;
    FixedString<CVarString::capacity> persisted;  // the override saved to the settings file
    bool has_persisted = false;
};

// A command-line and environment shorthand (spec §6.2), such as --log=all:debug,net:trace. The
// expander turns the value into ordinary sets through Registry::stage().
using ShorthandFn = bool (*)(class Registry& registry, std::string_view value, Source source, Writer errors);

class Registry {
public:
    static constexpr usize capacity = 1024;
    static constexpr usize pending_capacity = 64;
    static constexpr usize shorthand_capacity = 16;

    Registry() noexcept;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    // ------------------------------------------------------------ registration (CV-11)
    // Returns false and reports why when the declaration is invalid: duplicate or malformed name,
    // missing help, default outside the range, a Live string, a full table.
    bool add(CVar<bool>& cvar) noexcept;
    bool add(CVar<i32>& cvar) noexcept;
    bool add(CVar<i64>& cvar) noexcept;
    bool add(CVar<f32>& cvar) noexcept;
    bool add(CVar<f64>& cvar) noexcept;
    bool add(CVarString& cvar) noexcept;
    template <class E>
    bool add(CVarEnum<E>& cvar) noexcept;

    // `name` is used as --name=... and EZ_NAME (uppercased).
    bool add_shorthand(const char* name, ShorthandFn expand) noexcept;

    // ------------------------------------------------------------ startup (CV-7, CV-8)
    // Defaults < settings file < environment < command line. Collects every error; on any error
    // the result asks to exit (fail fast). Locks Startup cvars at the end. Handles --help and
    // --reset-settings.
    StartupResult init(const InitOptions& options) noexcept;

    // During init and from shorthand expanders: validate and assign immediately with `source`.
    // Returns false and writes the reason to `errors`.
    bool stage(std::string_view name, std::string_view value, Source source, Writer errors) noexcept;

    // ------------------------------------------------------------ runtime (CV-5, CV-7)
    // Validates now; a valid value is queued and applied by apply_pending(). An invalid one is
    // rejected and the previous value kept; the result's message says why.
    SetResult set(std::string_view name, std::string_view value, Source source = Source::Console) noexcept;
    // Main thread, at the start of a frame. Returns how many writes were applied.
    usize apply_pending() noexcept;
    // Back to the default and no persisted override. Queued like set().
    SetResult reset(std::string_view name, Source source = Source::Console) noexcept;
    void reset_all(Source source = Source::Console) noexcept;

    // ------------------------------------------------------------ lookup and text
    [[nodiscard]] const Entry* find(std::string_view name) const noexcept;  // names and aliases
    [[nodiscard]] usize size() const noexcept { return count_; }
    [[nodiscard]] const Entry& entry(usize index) const noexcept { return entries_[index]; }
    // The current value as the settings file and the console write it.
    void format_value(const Entry& entry, FixedString<CVarString::capacity>& out) const noexcept;
    [[nodiscard]] bool is_default(const Entry& entry) const noexcept;

    // ------------------------------------------------------------ persistence (CV-9)
    // Writes the overrides of Persist cvars and the preserved unknown lines. Atomic: a temporary
    // file, then rename. Returns false when the file cannot be written.
    bool save() noexcept;
    [[nodiscard]] bool dirty() const noexcept { return dirty_; }
    [[nodiscard]] const char* settings_path() const noexcept { return settings_path_.c_str(); }

    // ------------------------------------------------------------ console and help (spec §9)
    // name | name value | name ? | find text | reset name | reset all | dump | help
    void execute(std::string_view line, Writer out) noexcept;
    void write_help(Writer out) const noexcept;
    void dump(Writer out) const noexcept;

    // ------------------------------------------------------------ messages
    // Installs the receiver and hands it every buffered message.
    void set_report_hook(ReportFn fn, void* context) noexcept;

private:
    struct Pending {
        u16 index;
        Source source;
        bool reset;
        union {
            i64 i;
            f64 f;
        } number;
        FixedString<CVarString::capacity> text;
    };

    bool add_entry(const Meta* meta, void* object, State* state) noexcept;
    bool insert_name(std::string_view name, u16 index) noexcept;
    [[nodiscard]] i32 lookup(std::string_view name) const noexcept;
    void report(Severity severity, std::string_view message) noexcept;

    Entry entries_[capacity];
    usize count_ = 0;
    // Open-addressing table of name hashes to entry index + 1 (0 = empty); names and aliases.
    static constexpr usize table_size = capacity * 4;
    u64 table_hash_[table_size] = {};
    u16 table_index_[table_size] = {};

    struct Shorthand {
        const char* name;
        ShorthandFn expand;
    };
    Shorthand shorthands_[shorthand_capacity] = {};
    usize shorthand_count_ = 0;

    Pending pending_[pending_capacity];
    usize pending_count_ = 0;

    FixedString<1023> settings_path_;
    FixedString<8191> unknown_lines_;  // lines of the settings file this build does not know
    FixedString<8191> buffered_reports_;
    ReportFn report_fn_ = nullptr;
    void* report_context_ = nullptr;
    bool initialized_ = false;
    bool dirty_ = false;
    u32 registration_errors_ = 0;
    FixedString<2047> registration_report_;

    friend struct Access;
};

// The process-wide registry the application uses. Tests make their own.
Registry& registry() noexcept;

template <class E>
bool Registry::add(CVarEnum<E>& cvar) noexcept {
    CVar<i32>& storage = Access::storage(cvar);
    return add_entry(&cvar.meta(), &storage, &Access::state(storage));
}

}  // namespace ez::cvars

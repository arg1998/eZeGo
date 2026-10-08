#include "ez/cvars/registry.hpp"

#include "ez/base/hash.hpp"
#include "ez/base/modules.hpp"
#include "ez/cvars/detail/paths.hpp"
#include "ez/cvars/detail/text.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <thread>

namespace ez::cvars {

namespace {

using detail::Text;

constexpr std::string_view source_name(Source s) noexcept {
    switch (s) {
        case Source::Default:
            return "default";
        case Source::File:
            return "settings file";
        case Source::Environment:
            return "environment";
        case Source::CommandLine:
            return "command line";
        case Source::Console:
            return "console";
        case Source::Panel:
            return "panel";
        case Source::Plugin:
            return "plugin";
    }
    return "?";
}

constexpr std::string_view type_name(Type t) noexcept {
    switch (t) {
        case Type::Bool:
            return "bool";
        case Type::I32:
            return "i32";
        case Type::I64:
            return "i64";
        case Type::F32:
            return "f32";
        case Type::F64:
            return "f64";
        case Type::Enum:
            return "enum";
        case Type::String:
            return "string";
    }
    return "?";
}

constexpr std::string_view mutability_name(Mutability m) noexcept {
    switch (m) {
        case Mutability::Const:
            return "const";
        case Mutability::Startup:
            return "startup";
        case Mutability::Live:
            return "live";
    }
    return "?";
}

constexpr std::string_view tier_name(Tier t) noexcept {
    switch (t) {
        case Tier::User:
            return "user";
        case Tier::Advanced:
            return "advanced";
        case Tier::Developer:
            return "developer";
        case Tier::Hidden:
            return "hidden";
    }
    return "?";
}

bool valid_name(std::string_view name) noexcept {
    if (name.empty() || name.size() > 63 || name.front() == '.' || name.back() == '.') {
        return false;
    }
    char prev = '.';
    for (const char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.';
        if (!ok || (c == '.' && prev == '.')) {
            return false;
        }
        prev = c;
    }
    return true;
}

bool known_module(std::string_view name) noexcept {
    const std::string_view first = name.substr(0, name.find('.'));
    if (first == "plugin") {
        return true;
    }
    for (usize i = 0; i < module_count; ++i) {
        if (module_name(Module(i)) == first) {
            return true;
        }
    }
    return false;
}

// A parsed, validated value ready to store.
struct Value {
    i64 i = 0;
    f64 f = 0;
    FixedString<CVarString::capacity> text;
};

bool parse_bool(std::string_view s, bool& out) noexcept {
    if (detail::iequals(s, "true") || detail::iequals(s, "on") || detail::iequals(s, "yes") || s == "1") {
        out = true;
        return true;
    }
    if (detail::iequals(s, "false") || detail::iequals(s, "off") || detail::iequals(s, "no") || s == "0") {
        out = false;
        return true;
    }
    return false;
}

bool parse_int(std::string_view s, i64& out) noexcept {
    if (!s.empty() && s.front() == '+') {
        s.remove_prefix(1);
    }
    const auto r = std::from_chars(s.data(), s.data() + s.size(), out);
    return r.ec == std::errc{} && r.ptr == s.data() + s.size() && !s.empty();
}

bool parse_float(std::string_view s, f64& out) noexcept {
    if (s.empty() || s.size() > 63) {
        return false;
    }
    char buf[64];
    std::memcpy(buf, s.data(), s.size());
    buf[s.size()] = '\0';
    char* end = nullptr;
    out = std::strtod(buf, &end);  // "C" locale: eZeGo never calls setlocale
    return end == buf + s.size() && std::isfinite(out);
}

// Shortest text that reads back as the same value.
void format_float(f64 v, bool single, Text& out) noexcept {
    char buf[64];
    if (v == std::trunc(v) && std::fabs(v) < 1e15) {
        std::snprintf(buf, sizeof(buf), "%.0f", v);  // whole numbers as integers: 1000, not 1e+03
        out.append(buf);
        return;
    }
    for (int precision = 1; precision <= 17; ++precision) {
        std::snprintf(buf, sizeof(buf), "%.*g", precision, v);
        const f64 back = std::strtod(buf, nullptr);
        if (single ? f32(back) == f32(v) : back == v) {
            break;
        }
    }
    out.append(buf);
}

bool in_range(const Meta& m, f64 v) noexcept {
    return v >= m.options.min && v <= m.options.max;
}

void range_text(const Meta& m, Text& out) noexcept {
    const bool lo = std::isfinite(m.options.min);
    const bool hi = std::isfinite(m.options.max);
    const bool single = m.type == Type::F32;
    if (lo && hi) {
        format_float(m.options.min, single, out);
        out.append("..");
        format_float(m.options.max, single, out);
    } else if (lo) {
        out.append(">= ");
        format_float(m.options.min, single, out);
    } else if (hi) {
        out.append("<= ");
        format_float(m.options.max, single, out);
    }
}

// What the cvar accepts, for messages and --help: "1..1000", "true|false", "auto|alsa|pulse".
void accepted_text(const Meta& m, Text& out) noexcept {
    switch (m.type) {
        case Type::Bool:
            out.append("true|false");
            break;
        case Type::Enum:
            for (u32 i = 0; i < m.enum_count; ++i) {
                if (i > 0) {
                    out.append("|");
                }
                out.append(m.enum_names[i]);
            }
            break;
        case Type::String:
            out.printf("text, at most %zu characters", CVarString::capacity);
            break;
        default: {
            const usize before = out.size();
            range_text(m, out);
            if (out.size() == before) {
                out.append(type_name(m.type));
            }
            break;
        }
    }
}

// Validates `text` for `m`. On failure writes a message that names the accepted values.
bool parse_value(const Meta& m, std::string_view text, Value& out, Text& error) noexcept {
    bool ok = false;
    switch (m.type) {
        case Type::Bool: {
            bool b = false;
            ok = parse_bool(text, b);
            out.i = b ? 1 : 0;
            break;
        }
        case Type::I32:
        case Type::I64: {
            ok = parse_int(text, out.i) && in_range(m, f64(out.i));
            if (ok && m.type == Type::I32) {
                ok = out.i >= std::numeric_limits<i32>::min() && out.i <= std::numeric_limits<i32>::max();
            }
            break;
        }
        case Type::F32:
        case Type::F64: {
            ok = parse_float(text, out.f) && in_range(m, out.f);
            if (ok && m.type == Type::F32) {
                ok = std::isfinite(f32(out.f));
            }
            break;
        }
        case Type::Enum: {
            for (u32 i = 0; i < m.enum_count; ++i) {
                if (detail::iequals(text, m.enum_names[i])) {
                    out.i = i;
                    ok = true;
                    break;
                }
            }
            break;
        }
        case Type::String:
            ok = out.text.assign(text);
            break;
    }
    if (!ok) {
        error.printf("%s: '%.*s' is not accepted; expected ", m.name, int(text.size()), text.data());
        accepted_text(m, error);
    }
    return ok;
}

void default_value(const Meta& m, Value& out) noexcept {
    out.i = m.default_int;
    out.f = m.default_float;
    out.text.assign(m.default_text);
}

// ---------------------------------------------------------------- typed access to an entry

void store(Entry& e, const Value& v, Source source) noexcept {
    switch (e.meta->type) {
        case Type::Bool:
            Access::value(*static_cast<CVar<bool>*>(e.object)).store(v.i != 0, std::memory_order_relaxed);
            break;
        case Type::I32:
        case Type::Enum:
            Access::value(*static_cast<CVar<i32>*>(e.object)).store(i32(v.i), std::memory_order_relaxed);
            break;
        case Type::I64:
            Access::value(*static_cast<CVar<i64>*>(e.object)).store(v.i, std::memory_order_relaxed);
            break;
        case Type::F32:
            Access::value(*static_cast<CVar<f32>*>(e.object)).store(f32(v.f), std::memory_order_relaxed);
            break;
        case Type::F64:
            Access::value(*static_cast<CVar<f64>*>(e.object)).store(v.f, std::memory_order_relaxed);
            break;
        case Type::String:
            Access::text(*static_cast<CVarString*>(e.object)).assign(v.text.view());
            break;
    }
    e.state->source.store(u8(source), std::memory_order_relaxed);
    e.state->generation.fetch_add(1, std::memory_order_release);
}

void load(const Entry& e, Value& out) noexcept {
    switch (e.meta->type) {
        case Type::Bool:
            out.i = static_cast<const CVar<bool>*>(e.object)->value() ? 1 : 0;
            break;
        case Type::I32:
        case Type::Enum:
            out.i = static_cast<const CVar<i32>*>(e.object)->value();
            break;
        case Type::I64:
            out.i = static_cast<const CVar<i64>*>(e.object)->value();
            break;
        case Type::F32:
            out.f = static_cast<const CVar<f32>*>(e.object)->value();
            break;
        case Type::F64:
            out.f = static_cast<const CVar<f64>*>(e.object)->value();
            break;
        case Type::String:
            out.text.assign(static_cast<const CVarString*>(e.object)->value());
            break;
    }
}

void format(const Meta& m, const Value& v, Text& out) noexcept {
    switch (m.type) {
        case Type::Bool:
            out.append(v.i != 0 ? "true" : "false");
            break;
        case Type::I32:
        case Type::I64:
            out.printf("%lld", static_cast<long long>(v.i));
            break;
        case Type::F32:
            format_float(v.f, true, out);
            break;
        case Type::F64:
            format_float(v.f, false, out);
            break;
        case Type::Enum:
            out.append(v.i >= 0 && u64(v.i) < m.enum_count ? m.enum_names[v.i] : "?");
            break;
        case Type::String:
            out.append(v.text.view());
            break;
    }
}

bool equal(const Meta& m, const Value& a, const Value& b) noexcept {
    switch (m.type) {
        case Type::F32:
        case Type::F64:
            return a.f == b.f;
        case Type::String:
            return a.text.view() == b.text.view();
        default:
            return a.i == b.i;
    }
}

// Values as the user sees them: redacted for Secret cvars.
void shown(const Meta& m, const Value& v, Text& out) noexcept {
    if (has(m.options.flags, Flags::Secret)) {
        out.append("<redacted>");
    } else {
        format(m, v, out);
    }
}

const char* env_lookup(const char* const* envp, const char* name) noexcept {
    if (envp == nullptr) {
        return std::getenv(name);
    }
    const usize n = std::strlen(name);
    for (const char* const* e = envp; *e != nullptr; ++e) {
        if (std::strncmp(*e, name, n) == 0 && (*e)[n] == '=') {
            return *e + n + 1;
        }
    }
    return nullptr;
}

// log.drain_ms -> EZ_LOG_DRAIN_MS
void env_name(std::string_view name, char (&out)[80]) noexcept {
    usize n = 0;
    for (const char c : std::string_view("EZ_")) {
        out[n++] = c;
    }
    for (const char c : name) {
        if (n + 1 >= sizeof(out)) {
            break;
        }
        out[n++] = c == '.' ? '_' : (c >= 'a' && c <= 'z' ? char(c - 'a' + 'A') : c);
    }
    out[n] = '\0';
}

Writer stdout_writer() noexcept {
    return Writer{nullptr, [](void*, std::string_view s) { std::fwrite(s.data(), 1, s.size(), stdout); }};
}

}  // namespace

// ==================================================================== registration

Registry::Registry() noexcept = default;

bool Registry::add(CVar<bool>& c) noexcept {
    return add_entry(&c.meta(), &c, &Access::state(c));
}
bool Registry::add(CVar<i32>& c) noexcept {
    return add_entry(&c.meta(), &c, &Access::state(c));
}
bool Registry::add(CVar<i64>& c) noexcept {
    return add_entry(&c.meta(), &c, &Access::state(c));
}
bool Registry::add(CVar<f32>& c) noexcept {
    return add_entry(&c.meta(), &c, &Access::state(c));
}
bool Registry::add(CVar<f64>& c) noexcept {
    return add_entry(&c.meta(), &c, &Access::state(c));
}
bool Registry::add(CVarString& c) noexcept {
    return add_entry(&c.meta(), &c, &Access::state(c));
}

bool Registry::add_entry(const Meta* meta, void* object, State* state) noexcept {
    const Meta& m = *meta;
    Text why;
    if (count_ >= capacity) {
        why.printf("%s: the registry is full (%zu cvars)", m.name, capacity);
    } else if (!valid_name(m.name)) {
        why.printf("'%s': names are dotted lower_snake_case segments, at most 63 characters", m.name);
    } else if (!known_module(m.name)) {
        why.printf("%s: the first segment must be a module from modules.cmake", m.name);
    } else if (m.options.help == nullptr || m.options.help[0] == '\0') {
        why.printf("%s: help text is mandatory", m.name);
    } else if (lookup(m.name) >= 0) {
        why.printf("%s: registered twice, or the name is another cvar's alias", m.name);
    } else if (m.type == Type::String && m.options.mutability == Mutability::Live) {
        why.printf("%s: a string cvar must be Startup or Const; strings cannot be replaced atomically", m.name);
    } else if (m.type == Type::String && std::strlen(m.default_text) > CVarString::capacity) {
        why.printf("%s: the default is longer than %zu characters", m.name, CVarString::capacity);
    } else if (m.type == Type::Enum && (m.enum_count == 0 || m.default_int < 0 || u64(m.default_int) >= m.enum_count)) {
        why.printf("%s: the default is not one of the enum's names", m.name);
    } else if ((m.type == Type::I32 || m.type == Type::I64) && !in_range(m, f64(m.default_int))) {
        why.printf("%s: the default is outside its range", m.name);
    } else if ((m.type == Type::F32 || m.type == Type::F64) && !in_range(m, m.default_float)) {
        why.printf("%s: the default is outside its range", m.name);
    } else if (state->registered) {
        why.printf("%s: already registered with another registry", m.name);
    }
    if (!why.empty()) {
        ++registration_errors_;
        // Reported now, and init() refuses to start while any declaration is invalid (fail fast).
        report(Severity::Error, why.view());
        Text line;
        line.printf("  declaration: %s\n", why.c_str());
        if (registration_report_.size() + line.size() <= decltype(registration_report_)::capacity()) {
            Text all;
            all.append(registration_report_.view());
            all.append(line.view());
            registration_report_.assign(all.view());
        }
        return false;
    }

    const u16 index = u16(count_);
    Entry& e = entries_[count_++];
    e.meta = meta;
    e.object = object;
    e.state = state;
    state->registered = true;
    insert_name(m.name, index);

    // Aliases: former names, accepted with a deprecation warning.
    std::string_view aliases = m.options.aliases;
    while (!aliases.empty()) {
        const usize comma = aliases.find(',');
        const std::string_view alias = detail::trim(aliases.substr(0, comma));
        if (!alias.empty() && lookup(alias) < 0) {
            insert_name(alias, index);
        }
        aliases = comma == std::string_view::npos ? std::string_view{} : aliases.substr(comma + 1);
    }
    return true;
}

bool Registry::insert_name(std::string_view name, u16 index) noexcept {
    const u64 h = hash64(name);
    for (usize probe = 0; probe < table_size; ++probe) {
        const usize slot = (h + probe) & (table_size - 1);
        if (table_index_[slot] == 0) {
            table_hash_[slot] = h;
            table_index_[slot] = u16(index + 1);
            return true;
        }
    }
    return false;
}

i32 Registry::lookup(std::string_view name) const noexcept {
    const u64 h = hash64(name);
    for (usize probe = 0; probe < table_size; ++probe) {
        const usize slot = (h + probe) & (table_size - 1);
        if (table_index_[slot] == 0) {
            return -1;
        }
        if (table_hash_[slot] != h) {
            continue;
        }
        const Entry& e = entries_[table_index_[slot] - 1];
        if (name == e.meta->name) {
            return table_index_[slot] - 1;
        }
        // An alias: compare against the alias list.
        std::string_view aliases = e.meta->options.aliases;
        while (!aliases.empty()) {
            const usize comma = aliases.find(',');
            if (detail::trim(aliases.substr(0, comma)) == name) {
                return table_index_[slot] - 1;
            }
            aliases = comma == std::string_view::npos ? std::string_view{} : aliases.substr(comma + 1);
        }
    }
    return -1;
}

const Entry* Registry::find(std::string_view name) const noexcept {
    const i32 i = lookup(name);
    return i < 0 ? nullptr : &entries_[i];
}

bool Registry::add_shorthand(const char* name, ShorthandFn expand) noexcept {
    if (shorthand_count_ >= shorthand_capacity || name == nullptr || expand == nullptr) {
        return false;
    }
    shorthands_[shorthand_count_++] = Shorthand{name, expand};
    return true;
}

// ==================================================================== messages

void Registry::report(Severity severity, std::string_view message) noexcept {
    if (report_fn_ != nullptr) {
        report_fn_(report_context_, severity, message);
        return;
    }
    // Buffered until a receiver exists: one line per message, severity as the first character.
    const char tag = severity == Severity::Info ? 'I' : severity == Severity::Warning ? 'W' : 'E';
    Text line;
    line.printf("%c%.*s\n", tag, int(message.size()), message.data());
    if (buffered_reports_.size() + line.size() <= decltype(buffered_reports_)::capacity()) {
        Text all;
        all.append(buffered_reports_.view());
        all.append(line.view());
        buffered_reports_.assign(all.view());
    }
}

void Registry::set_report_hook(ReportFn fn, void* context) noexcept {
    report_fn_ = fn;
    report_context_ = context;
    if (fn == nullptr) {
        return;
    }
    std::string_view rest = buffered_reports_.view();
    while (!rest.empty()) {
        const usize nl = rest.find('\n');
        const std::string_view line = rest.substr(0, nl);
        if (!line.empty()) {
            const Severity s = line[0] == 'I' ? Severity::Info : line[0] == 'W' ? Severity::Warning : Severity::Error;
            fn(context, s, line.substr(1));
        }
        rest = nl == std::string_view::npos ? std::string_view{} : rest.substr(nl + 1);
    }
    buffered_reports_.clear();
}

// ==================================================================== values as text

void Registry::format_value(const Entry& entry, FixedString<CVarString::capacity>& out) const noexcept {
    Value v;
    load(entry, v);
    Text t;
    format(*entry.meta, v, t);
    out.assign(t.view());
}

bool Registry::is_default(const Entry& entry) const noexcept {
    Value v;
    Value d;
    load(entry, v);
    default_value(*entry.meta, d);
    return equal(*entry.meta, v, d);
}

// ==================================================================== startup

bool Registry::stage(std::string_view name, std::string_view value, Source source, Writer errors) noexcept {
    const i32 index = lookup(name);
    if (index < 0) {
        Text t;
        t.printf("unknown setting '%.*s'", int(name.size()), name.data());
        errors(t.view());
        return false;
    }
    Entry& e = entries_[index];
    const Meta& m = *e.meta;
    if (name != m.name) {
        Text t;
        t.printf("'%.*s' was renamed to '%s'; the old name still works for now", int(name.size()), name.data(), m.name);
        report(Severity::Warning, t.view());
        dirty_ = dirty_ || source == Source::File;  // the next save writes the new name
    }
    if (m.options.mutability == Mutability::Const) {
        Text t;
        t.printf("%s is fixed at build time and cannot be set", m.name);
        errors(t.view());
        return false;
    }
    if (e.state->locked) {
        Text t;
        t.printf("%s can only be set at startup", m.name);
        errors(t.view());
        return false;
    }
    Value v;
    Text why;
    if (!parse_value(m, value, v, why)) {
        errors(why.view());
        return false;
    }
    store(e, v, source);
    if (source == Source::File && has(m.options.flags, Flags::Persist)) {
        Text t;
        format(m, v, t);
        e.persisted.assign(t.view());
        e.has_persisted = true;
    }
    return true;
}

StartupResult Registry::init(const InitOptions& options) noexcept {
    StartupResult result;
    Text errors_text;
    u32 errors = 0;
    struct ErrorSink {
        Text* text;
        u32* count;
        const char* prefix;
    };

    // Each error is one line; the prefix says where it came from.
    auto make_sink = [](ErrorSink& sink) {
        return Writer{&sink, [](void* ctx, std::string_view s) {
                          auto* k = static_cast<ErrorSink*>(ctx);
                          ++*k->count;
                          k->text->printf("  %s%.*s\n", k->prefix, int(s.size()), s.data());
                      }};
    };

    // --help and --reset-settings first: they change what happens to everything else.
    bool help = false;
    bool reset_settings = false;
    for (int i = 1; i < options.argc; ++i) {
        const std::string_view a = options.argv[i];
        if (a == "--") {
            break;
        }
        help = help || a == "--help" || a == "-h";
        reset_settings = reset_settings || a == "--reset-settings";
    }
    if (help) {
        write_help(options.help_out.write != nullptr ? options.help_out : stdout_writer());
        result.exit_now = true;
        result.exit_code = 0;
        return result;
    }

    // ---------------------------------------------------------------- settings file
    if (options.settings_path == nullptr) {
        if (!detail::default_settings_path(settings_path_)) {
            settings_path_.clear();
        }
    } else {
        settings_path_.assign(options.settings_path);
    }
    if (!settings_path_.empty() && reset_settings) {
        std::error_code ec;
        if (std::filesystem::exists(settings_path_.c_str(), ec)) {
            char stamp[32];
            const std::time_t now = std::time(nullptr);
            std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
            Text backup;
            backup.printf("%s.bad-%s", settings_path_.c_str(), stamp);
            std::filesystem::rename(settings_path_.c_str(), backup.c_str(), ec);
            Text t;
            t.printf("settings reset: the previous file was moved to %s", backup.c_str());
            report(Severity::Warning, t.view());
        }
    }
    if (!settings_path_.empty()) {
        if (std::FILE* f = std::fopen(settings_path_.c_str(), "rb")) {
            char raw[2048];
            u32 line_no = 0;
            while (std::fgets(raw, sizeof(raw), f) != nullptr) {
                ++line_no;
                const std::string_view line = detail::trim(raw);
                if (line.empty() || line.front() == '#') {
                    continue;
                }
                Text prefix;
                prefix.printf("%s:%u: ", settings_path_.c_str(), line_no);
                ErrorSink sink{&errors_text, &errors, prefix.c_str()};
                const usize eq = line.find('=');
                const std::string_view name = detail::trim(line.substr(0, eq));
                if (eq == std::string_view::npos || name.empty()) {
                    make_sink(sink)("malformed line; expected 'name = value'");
                    continue;
                }
                const std::string_view value = detail::unquote(detail::trim(line.substr(eq + 1)));
                if (lookup(name) < 0) {
                    // Unknown to this build (a newer version, an absent plugin): kept, not an error.
                    Text keep;
                    keep.printf("%.*s\n", int(line.size()), line.data());
                    if (unknown_lines_.size() + keep.size() <= decltype(unknown_lines_)::capacity()) {
                        Text all;
                        all.append(unknown_lines_.view());
                        all.append(keep.view());
                        unknown_lines_.assign(all.view());
                    }
                    Text t;
                    t.printf("%s:%u: unknown setting '%.*s' is kept in the file but not used", settings_path_.c_str(),
                             line_no, int(name.size()), name.data());
                    report(Severity::Warning, t.view());
                    continue;
                }
                stage(name, value, Source::File, make_sink(sink));
            }
            std::fclose(f);
        }
    }

    // ---------------------------------------------------------------- environment
    {
        ErrorSink sink{&errors_text, &errors, "environment: "};
        for (usize s = 0; s < shorthand_count_; ++s) {
            char var[80];
            env_name(shorthands_[s].name, var);
            if (const char* v = env_lookup(options.envp, var)) {
                shorthands_[s].expand(*this, v, Source::Environment, make_sink(sink));
            }
        }
        for (usize i = 0; i < count_; ++i) {
            char var[80];
            env_name(entries_[i].meta->name, var);
            if (const char* v = env_lookup(options.envp, var)) {
                Text t;
                ErrorSink named{&errors_text, &errors, ""};
                t.printf("environment %s: ", var);
                named.prefix = t.c_str();
                stage(entries_[i].meta->name, v, Source::Environment, make_sink(named));
            }
        }
    }

    // ---------------------------------------------------------------- command line
    {
        ErrorSink sink{&errors_text, &errors, "command line: "};
        for (int i = 1; i < options.argc; ++i) {
            const std::string_view a = options.argv[i];
            if (a == "--") {
                break;
            }
            if (a.size() < 3 || a.substr(0, 2) != "--" || a == "--reset-settings") {
                continue;  // positional arguments belong to the application
            }
            const std::string_view body = a.substr(2);
            const usize eq = body.find('=');
            const std::string_view name = body.substr(0, eq);
            bool handled = false;
            for (usize s = 0; s < shorthand_count_ && !handled; ++s) {
                if (name == shorthands_[s].name) {
                    handled = true;
                    if (eq == std::string_view::npos) {
                        Text t;
                        t.printf("--%.*s needs a value", int(name.size()), name.data());
                        make_sink(sink)(t.view());
                    } else {
                        shorthands_[s].expand(*this, body.substr(eq + 1), Source::CommandLine, make_sink(sink));
                    }
                }
            }
            if (handled) {
                continue;
            }
            const i32 index = lookup(name);
            if (index < 0) {
                Text t;
                t.printf("unknown option '%.*s' (see --help)", int(a.size()), a.data());
                make_sink(sink)(t.view());
                continue;
            }
            if (eq == std::string_view::npos) {
                if (entries_[index].meta->type == Type::Bool) {
                    stage(name, "true", Source::CommandLine, make_sink(sink));  // --flag means true
                } else {
                    Text t;
                    t.printf("--%.*s needs a value: --%.*s=<value>", int(name.size()), name.data(), int(name.size()),
                             name.data());
                    make_sink(sink)(t.view());
                }
                continue;
            }
            stage(name, body.substr(eq + 1), Source::CommandLine, make_sink(sink));
        }
    }

    // ---------------------------------------------------------------- fail fast (CV-7)
    errors += registration_errors_;
    result.error_count = errors;
    if (errors > 0) {
        result.ok = false;
        result.exit_now = true;
        result.exit_code = 2;
        Text head;
        head.printf("eZeGo cannot start: %u invalid setting%s.\n", errors, errors == 1 ? "" : "s");
        Text tail;
        tail.printf("Fix %s, or start once with --reset-settings to set the settings file aside and use defaults.\n",
                    errors == 1 ? "it" : "them");
        Text all;
        all.append(head.view());
        all.append(registration_report_.view());
        all.append(errors_text.view());
        all.append(tail.view());
        result.report.assign(all.view());
        return result;
    }

    for (usize i = 0; i < count_; ++i) {
        entries_[i].state->locked = entries_[i].meta->options.mutability == Mutability::Startup;
    }
    initialized_ = true;
    return result;
}

// ==================================================================== runtime

SetResult Registry::set(std::string_view name, std::string_view value, Source source) noexcept {
    SetResult r;
    Text msg;
    const i32 index = lookup(name);
    if (index < 0) {
        r.status = SetStatus::Unknown;
        msg.printf("unknown setting '%.*s'", int(name.size()), name.data());
        r.message.assign(msg.view());
        return r;
    }
    Entry& e = entries_[index];
    const Meta& m = *e.meta;
    if (m.options.mutability == Mutability::Const) {
        r.status = SetStatus::NotSettable;
        msg.printf("%s is fixed at build time", m.name);
        r.message.assign(msg.view());
        return r;
    }
    Value v;
    if (!parse_value(m, value, v, msg)) {
        r.status = SetStatus::Rejected;
        r.message.assign(msg.view());
        report(Severity::Warning, msg.view());
        return r;
    }
    if (e.state->locked) {
        if (!has(m.options.flags, Flags::Persist)) {
            r.status = SetStatus::NotSettable;
            msg.printf("%s can only be set at startup: command line, environment or settings file", m.name);
            r.message.assign(msg.view());
            return r;
        }
        Text t;
        format(m, v, t);
        e.persisted.assign(t.view());
        e.has_persisted = true;
        dirty_ = true;
        r.status = SetStatus::AfterRestart;
        msg.printf("%s saved; takes effect after restart", m.name);
        r.message.assign(msg.view());
        return r;
    }
    if (pending_count_ >= pending_capacity) {
        r.status = SetStatus::QueueFull;
        msg.printf("too many changes this frame; %s was not changed", m.name);
        r.message.assign(msg.view());
        return r;
    }
    Pending& p = pending_[pending_count_++];
    p.index = u16(index);
    p.source = source;
    p.reset = false;
    if (m.type == Type::F32 || m.type == Type::F64) {
        p.number.f = v.f;
    } else {
        p.number.i = v.i;
    }
    p.text.assign(v.text.view());
    r.status = SetStatus::Queued;
    return r;
}

SetResult Registry::reset(std::string_view name, Source source) noexcept {
    SetResult r;
    const i32 index = lookup(name);
    if (index < 0) {
        r.status = SetStatus::Unknown;
        Text msg;
        msg.printf("unknown setting '%.*s'", int(name.size()), name.data());
        r.message.assign(msg.view());
        return r;
    }
    Entry& e = entries_[index];
    if (e.state->locked || e.meta->options.mutability == Mutability::Const) {
        // Startup: forget the override so the next start uses the default.
        if (e.has_persisted) {
            e.has_persisted = false;
            dirty_ = true;
        }
        r.status = e.meta->options.mutability == Mutability::Const ? SetStatus::NotSettable : SetStatus::AfterRestart;
        return r;
    }
    if (pending_count_ >= pending_capacity) {
        r.status = SetStatus::QueueFull;
        return r;
    }
    Pending& p = pending_[pending_count_++];
    p.index = u16(index);
    p.source = source;
    p.reset = true;
    r.status = SetStatus::Queued;
    return r;
}

void Registry::reset_all(Source source) noexcept {
    for (usize i = 0; i < count_; ++i) {
        if (!is_default(entries_[i]) || entries_[i].has_persisted) {
            reset(entries_[i].meta->name, source);
        }
    }
}

usize Registry::apply_pending() noexcept {
    const usize n = pending_count_;
    for (usize k = 0; k < n; ++k) {
        const Pending& p = pending_[k];
        Entry& e = entries_[p.index];
        const Meta& m = *e.meta;
        Value before;
        load(e, before);
        Value after;
        if (p.reset) {
            default_value(m, after);
        } else {
            if (m.type == Type::F32 || m.type == Type::F64) {
                after.f = p.number.f;
            } else {
                after.i = p.number.i;
            }
            after.text.assign(p.text.view());
        }
        const Source source = p.reset ? Source::Default : p.source;
        store(e, after, source);

        if (p.reset) {
            if (e.has_persisted) {
                e.has_persisted = false;
                dirty_ = true;
            }
        } else if (has(m.options.flags, Flags::Persist) &&
                   (p.source == Source::Console || p.source == Source::Panel || p.source == Source::Plugin)) {
            Text t;
            format(m, after, t);
            e.persisted.assign(t.view());
            e.has_persisted = true;
            dirty_ = true;
        }

        Text line;
        line.printf("cvar %s: ", m.name);
        shown(m, before, line);
        line.append(" -> ");
        shown(m, after, line);
        const std::string_view src = source_name(p.source);
        line.printf(" (%.*s%s)", int(src.size()), src.data(), p.reset ? ", reset" : "");
        report(Severity::Info, line.view());
    }
    pending_count_ = 0;
    return n;
}

// ==================================================================== persistence

bool Registry::save() noexcept {
    if (settings_path_.empty()) {
        return false;
    }
    u16 order[capacity];
    usize n = 0;
    for (usize i = 0; i < count_; ++i) {
        if (entries_[i].has_persisted && has(entries_[i].meta->options.flags, Flags::Persist)) {
            order[n++] = u16(i);
        }
    }
    std::sort(order, order + n,
              [this](u16 a, u16 b) { return std::strcmp(entries_[a].meta->name, entries_[b].meta->name) < 0; });

    std::error_code ec;
    const std::filesystem::path path(settings_path_.c_str());
    std::filesystem::create_directories(path.parent_path(), ec);
    const std::filesystem::path tmp = path.string() + ".tmp";
    std::FILE* f = std::fopen(tmp.string().c_str(), "wb");
    if (f == nullptr) {
        return false;
    }
    std::fputs("# eZeGo settings. Only values you changed are stored here. Edit while eZeGo is closed.\n", f);
    std::fputs("# schema 1\n", f);
    for (usize k = 0; k < n; ++k) {
        const Entry& e = entries_[order[k]];
        const std::string_view v = e.persisted.view();
        const bool quote =
            e.meta->type == Type::String && (v.empty() || v.front() == ' ' || v.back() == ' ' || v.front() == '"');
        std::fprintf(f, quote ? "%s = \"%.*s\"\n" : "%s = %.*s\n", e.meta->name, int(v.size()), v.data());
    }
    if (!unknown_lines_.empty()) {
        std::fputs("# Not known to this version; kept as they were:\n", f);
        std::fputs(unknown_lines_.c_str(), f);
    }
    const bool written = std::fflush(f) == 0;
    std::fclose(f);
    if (!written) {
        return false;
    }
    std::filesystem::rename(tmp, path, ec);  // atomic replace on POSIX; MoveFileEx(REPLACE) on Windows
    if (ec) {
        return false;
    }
    dirty_ = false;
    return true;
}

// ==================================================================== console and help (spec §9)

void Registry::dump(Writer out) const noexcept {
    for (usize i = 0; i < count_; ++i) {
        const Entry& e = entries_[i];
        if (is_default(e)) {
            continue;
        }
        Value v;
        load(e, v);
        Text line;
        line.printf("%s = ", e.meta->name);
        shown(*e.meta, v, line);
        const std::string_view src = source_name(e.state ? Source(e.state->source.load()) : Source::Default);
        line.printf("    # %.*s\n", int(src.size()), src.data());
        out(line.view());
    }
}

void Registry::write_help(Writer out) const noexcept {
    Text head;
    head.printf(
        "Usage: ezego [options] [project]\n\n"
        "  --help                 this text\n"
        "  --reset-settings       set the settings file aside and start with defaults\n");
    for (usize s = 0; s < shorthand_count_; ++s) {
        head.printf("  --%s=...\n", shorthands_[s].name);
    }
    head.printf(
        "\nEvery setting below is --name=value on the command line, EZ_NAME=value in the environment\n"
        "(dots become underscores), or 'name = value' in %s.\n",
        settings_path_.empty() ? "the settings file" : settings_path_.c_str());
    out(head.view());

    u16 order[capacity];
    usize n = 0;
    for (usize i = 0; i < count_; ++i) {
        if (entries_[i].meta->options.tier != Tier::Hidden) {
            order[n++] = u16(i);
        }
    }
    std::sort(order, order + n,
              [this](u16 a, u16 b) { return std::strcmp(entries_[a].meta->name, entries_[b].meta->name) < 0; });
    std::string_view group;
    for (usize k = 0; k < n; ++k) {
        const Entry& e = entries_[order[k]];
        const Meta& m = *e.meta;
        const std::string_view name = m.name;
        const std::string_view module = name.substr(0, name.find('.'));
        Text t;
        if (module != group) {
            group = module;
            t.printf("\n%.*s:\n", int(module.size()), module.data());
        }
        Value d;
        default_value(m, d);
        t.printf("  --%s=<", m.name);
        accepted_text(m, t);
        t.append(">  default ");
        shown(m, d, t);
        if (m.options.mutability != Mutability::Live) {
            const std::string_view mu = mutability_name(m.options.mutability);
            t.printf(", %.*s", int(mu.size()), mu.data());
        }
        t.printf("\n      %s\n", m.options.help);
        out(t.view());
    }
}

void Registry::execute(std::string_view line, Writer out) noexcept {
    line = detail::trim(line);
    if (line.empty()) {
        return;
    }
    const usize space = line.find(' ');
    const std::string_view head = line.substr(0, space);
    const std::string_view rest =
        space == std::string_view::npos ? std::string_view{} : detail::trim(line.substr(space));

    if (head == "help") {
        out("name            show value, default, source and help\n"
            "name value      set (validated; applied at the next frame)\n"
            "name ?          full help\n"
            "find text       search names and help\n"
            "reset name      back to the default\n"
            "reset all       every setting back to its default\n"
            "dump            every setting that differs from its default\n");
        return;
    }
    if (head == "dump") {
        dump(out);
        return;
    }
    if (head == "find") {
        usize hits = 0;
        for (usize i = 0; i < count_; ++i) {
            const Meta& m = *entries_[i].meta;
            if (detail::icontains(m.name, rest) || detail::icontains(m.options.help, rest)) {
                Value v;
                load(entries_[i], v);
                Text t;
                t.printf("%s = ", m.name);
                shown(m, v, t);
                t.printf("    %s\n", m.options.help);
                out(t.view());
                ++hits;
            }
        }
        if (hits == 0) {
            out("no setting matches\n");
        }
        return;
    }
    if (head == "reset") {
        if (rest == "all") {
            reset_all(Source::Console);
            out("every setting will be reset at the next frame\n");
            return;
        }
        const SetResult r = reset(rest, Source::Console);
        Text t;
        if (r.status == SetStatus::Queued) {
            t.printf("%.*s will be reset at the next frame\n", int(rest.size()), rest.data());
        } else {
            t.printf("%s\n", r.message.empty() ? "done; takes effect after restart" : r.message.c_str());
        }
        out(t.view());
        return;
    }

    const Entry* e = find(head);
    if (e == nullptr) {
        Text t;
        t.printf("unknown setting '%.*s' (try: find %.*s)\n", int(head.size()), head.data(), int(head.size()),
                 head.data());
        out(t.view());
        return;
    }
    const Meta& m = *e->meta;
    if (rest.empty() || rest == "?") {
        Value v;
        Value d;
        load(*e, v);
        default_value(m, d);
        Text t;
        t.printf("%s = ", m.name);
        shown(m, v, t);
        const std::string_view src = source_name(Source(e->state->source.load()));
        t.printf("    (%.*s)\n  default ", int(src.size()), src.data());
        shown(m, d, t);
        const std::string_view ty = type_name(m.type);
        const std::string_view mu = mutability_name(m.options.mutability);
        const std::string_view ti = tier_name(m.options.tier);
        t.printf("; %.*s; %.*s; %.*s; accepts ", int(ty.size()), ty.data(), int(mu.size()), mu.data(), int(ti.size()),
                 ti.data());
        accepted_text(m, t);
        t.printf("\n  %s\n", m.options.help);
        if (rest == "?" && m.options.aliases[0] != '\0') {
            t.printf("  former names: %s\n", m.options.aliases);
        }
        out(t.view());
        return;
    }
    const SetResult r = set(head, rest, Source::Console);
    if (r.status == SetStatus::Queued) {
        Text t;
        t.printf("%s will be %.*s at the next frame\n", m.name, int(rest.size()), rest.data());
        out(t.view());
    } else {
        Text t;
        t.printf("%s\n", r.message.c_str());
        out(t.view());
    }
}

Registry& registry() noexcept {
    static Registry g_registry;
    return g_registry;
}

}  // namespace ez::cvars

// The logger (specs/logging.md §4, §5, §8): the call-site path, the per-thread ring pool, the log
// thread, rendering and the sinks.
#include "ez/log/log.hpp"

#include "ez/base/assert.hpp"
#include "ez/base/fixed_string.hpp"
#include "ez/cvars/registry.hpp"
#include "ez/log/detail/os.hpp"
#include "ez/log/detail/ring.hpp"
#include "ez/log/detail/settings.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

namespace ez::log {

namespace {

using detail::RecordHeader;
using detail::RecordKind;
using detail::Ring;

constexpr usize slot_limit = 256;
constexpr usize sink_limit = 4;
constexpr usize render_capacity = detail::max_line_cap + 256;

// ---------------------------------------------------------------- time

u64 now_ticks() noexcept {
    return u64(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
                   .count());
}

i64 now_unix_ns() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ---------------------------------------------------------------- the ring pool

enum class SlotState : u8 { Free, Claimed, Retiring };

struct Slot {
    Ring ring;
    std::atomic<u8> state{u8(SlotState::Free)};
    FixedString<31> name;
    u64 reported_drops = 0;  // log thread only
};

struct ThreadSlot {
    Slot* slot = nullptr;
    u32 epoch = 0;
    ~ThreadSlot() {
        if (slot != nullptr) {
            slot->state.store(u8(SlotState::Retiring), std::memory_order_release);  // freed once drained
        }
    }
};

struct Sink {
    SinkFn fn;
    void* context;
};

// One record copied out of a ring, waiting to be sorted.
struct BatchEntry {
    u64 ticks;
    u32 offset;  // into the batch buffer
    u16 slot;
};

struct State {
    std::atomic<bool> running{false};
    std::atomic<u32> epoch{0};
    std::atomic<u32> frame{0};
    std::atomic<ProfilerHook> profiler{nullptr};
    std::atomic<FatalHook> fatal{nullptr};
    std::atomic<u64> lines{0};
    std::atomic<u64> pool_drops{0};
    std::atomic<bool> sync{false};

    Slot slots[slot_limit];
    usize slot_count = 0;
    u8* ring_memory = nullptr;
    usize ring_bytes = 0;

    // Log thread.
    std::thread thread;
    std::mutex mutex;  // stop, flush requests
    std::condition_variable wake;
    std::condition_variable flushed;
    bool stop = false;
    u64 flush_requested = 0;
    u64 flush_done = 0;
    std::mutex drain_mutex;  // held while draining: log thread, fatal

    // Batch.
    u8* batch = nullptr;
    usize batch_capacity = 0;
    usize batch_used = 0;
    BatchEntry* entries = nullptr;
    usize entry_capacity = 0;
    usize entry_count = 0;

    // Sinks.
    bool color = false;
    Sink sinks[sink_limit] = {};
    std::mutex sinks_mutex;
    char* history = nullptr;
    usize history_capacity = 0;
    u64 history_written = 0;
    std::mutex history_mutex;
    std::FILE* file = nullptr;
    bool file_failed = false;
    u32 file_generation = ~0u;
    u64 file_last_flush = 0;
    FixedString<1023> file_dir;
    i32 file_keep = 10;
    char stderr_buffer[64 * 1024];
    usize stderr_used = 0;

    // Wall clock = wall_epoch + (ticks - steady_epoch).
    i64 wall_epoch_ns = 0;
    u64 steady_epoch = 0;
    u32 levels_generation = ~0u;

    cvars::Registry* registry = nullptr;

    // A process that exits without shutdown() still writes what is queued and stops the thread
    // cleanly (a joinable std::thread at exit would terminate the process). The registry may
    // already be gone at this point, so it is not touched.
    ~State();
};

State g_state;
thread_local ThreadSlot t_slot;
thread_local bool t_in_fatal = false;

// The calling thread's ring, claimed from the pool on first use. nullptr when the logger is not
// running or the pool is exhausted.
Slot* current_slot() noexcept {
    const u32 epoch = g_state.epoch.load(std::memory_order_acquire);
    if (t_slot.slot != nullptr && t_slot.epoch == epoch) {
        return t_slot.slot;
    }
    if (!g_state.running.load(std::memory_order_acquire)) {
        return nullptr;
    }
    for (usize i = 0; i < g_state.slot_count; ++i) {
        u8 expected = u8(SlotState::Free);
        if (g_state.slots[i].state.compare_exchange_strong(expected, u8(SlotState::Claimed),
                                                           std::memory_order_acq_rel)) {
            Slot& s = g_state.slots[i];
            char name[32];
            std::snprintf(name, sizeof(name), "thread-%zu", i);
            s.name.assign(name);
            t_slot.slot = &s;
            t_slot.epoch = epoch;
            return &s;
        }
    }
    g_state.pool_drops.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}

// ---------------------------------------------------------------- rendering

constexpr char level_letter(Level l) noexcept {
    constexpr char letters[] = {'T', 'D', 'I', 'W', 'E', 'F'};
    return u8(l) < sizeof(letters) ? letters[u8(l)] : '?';
}

const char* basename(const char* path) noexcept {
    const char* b = path;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            b = p + 1;
        }
    }
    return b;
}

// hh:mm:ss.mmm  L  category    thread      message    <file:line>
usize render(char* out, usize cap, const Line& line, bool color) noexcept {
    const i64 wall_ns =
        g_state.wall_epoch_ns != 0 ? g_state.wall_epoch_ns + i64(line.ticks - g_state.steady_epoch) : now_unix_ns();
    const i64 seconds = wall_ns / 1000000000;
    const i64 millis = (wall_ns / 1000000) % 1000;
    const detail::LocalTime t = detail::local_time(seconds);
    const char* on = "";
    const char* off = "";
    if (color) {
        off = "\x1b[0m";
        switch (line.level) {
            case Level::Trace:
            case Level::Debug:
                on = "\x1b[2m";
                break;
            case Level::Warn:
                on = "\x1b[33m";
                break;
            case Level::Error:
                on = "\x1b[31m";
                break;
            case Level::Fatal:
                on = "\x1b[1;31m";
                break;
            default:
                off = "";
                break;
        }
    }
    const int n =
        std::snprintf(out, cap, "%02d:%02d:%02d.%03d  %s%c  %-12s %-10s %.*s%s    <%s:%u>\n", t.hour, t.minute,
                      t.second, int(millis), on, level_letter(line.level), category_name(line.category), line.thread,
                      int(line.length), line.text, off, basename(line.file), line.line);
    return n < 0 ? 0 : (usize(n) < cap ? usize(n) : cap - 1);
}

// ---------------------------------------------------------------- sinks (log thread)

void stderr_flush() noexcept {
    if (g_state.stderr_used > 0) {
        detail::write_stderr(g_state.stderr_buffer, g_state.stderr_used);
        g_state.stderr_used = 0;
    }
}

void stderr_append(const char* text, usize length) noexcept {
    if (g_state.stderr_used + length > sizeof(g_state.stderr_buffer)) {
        stderr_flush();
    }
    if (length > sizeof(g_state.stderr_buffer)) {
        detail::write_stderr(text, length);
        return;
    }
    std::memcpy(g_state.stderr_buffer + g_state.stderr_used, text, length);
    g_state.stderr_used += length;
}

void history_append(const char* text, usize length) noexcept {
    if (g_state.history_capacity == 0) {
        return;
    }
    const std::lock_guard<std::mutex> lock(g_state.history_mutex);
    for (usize i = 0; i < length; ++i) {
        g_state.history[(g_state.history_written + i) % g_state.history_capacity] = text[i];
    }
    g_state.history_written += length;
}

void file_close() noexcept {
    if (g_state.file != nullptr) {
        std::fclose(g_state.file);
        g_state.file = nullptr;
    }
}

// Opens the session file in the log directory and deletes the oldest beyond log.file.keep.
void file_open() noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path dir(g_state.file_dir.c_str());
    fs::create_directories(dir, ec);
    const detail::LocalTime t = detail::local_time(g_state.wall_epoch_ns / 1000000000);
    char name[64];
    std::snprintf(name, sizeof(name), "ezego-%04d%02d%02d-%02d%02d%02d.log", t.year, t.month, t.day, t.hour, t.minute,
                  t.second);
    fs::path path = dir / name;
    for (int n = 2; fs::exists(path, ec) && n < 100; ++n) {
        char again[72];
        std::snprintf(again, sizeof(again), "ezego-%04d%02d%02d-%02d%02d%02d-%d.log", t.year, t.month, t.day, t.hour,
                      t.minute, t.second, n);
        path = dir / again;
    }
    g_state.file = std::fopen(path.string().c_str(), "wb");
    if (g_state.file == nullptr) {
        g_state.file_failed = true;
        char msg[1200];
        const int len =
            std::snprintf(msg, sizeof(msg), "log: cannot open %s; file logging stays off until log.file changes\n",
                          path.string().c_str());
        detail::write_stderr(msg, usize(len > 0 ? len : 0));
        return;
    }
    std::setvbuf(g_state.file, nullptr, _IOFBF, 64 * 1024);
    std::fprintf(g_state.file, "# eZeGo %s (%s) log, started %04d-%02d-%02d %02d:%02d:%02d\n", build_version,
                 build_mode, t.year, t.month, t.day, t.hour, t.minute, t.second);

    std::vector<fs::path> old;  // the log thread may allocate; callers never do
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        const std::string f = e.path().filename().string();
        if (f.rfind("ezego-", 0) == 0 && e.path().extension() == ".log") {
            old.push_back(e.path());
        }
    }
    std::sort(old.begin(), old.end());
    const usize keep = usize(g_state.file_keep);
    for (usize i = 0; old.size() > keep && i < old.size() - keep; ++i) {
        fs::remove(old[i], ec);
    }
}

// Follows log.file: opens or closes the file when the cvar changes.
void file_follow_cvar() noexcept {
    const u32 gen = detail::cv_log_file.generation();
    if (gen != g_state.file_generation) {
        g_state.file_generation = gen;
        g_state.file_failed = false;
        if (!detail::cv_log_file.value()) {
            file_close();
        }
    }
    if (detail::cv_log_file.value() && g_state.file == nullptr && !g_state.file_failed) {
        file_open();
    }
}

void deliver(const Line& line, bool urgent_out) noexcept {
    char rendered[render_capacity];
    Line full = line;
    const usize plain_len = render(rendered, sizeof(rendered), line, false);
    full.rendered = rendered;
    full.rendered_length = plain_len;

    const bool sync = g_state.sync.load(std::memory_order_relaxed);
    if (detail::cv_log_stderr.value() && (!sync || urgent_out)) {  // sync mode: the caller already wrote Text lines
        if (g_state.color) {
            char colored[render_capacity];
            stderr_append(colored, render(colored, sizeof(colored), line, true));
        } else {
            stderr_append(rendered, plain_len);
        }
    }
    detail::debug_output(rendered);
    history_append(rendered, plain_len);
    if (g_state.file != nullptr) {
        std::fwrite(rendered, 1, plain_len, g_state.file);
    }
    {
        const std::lock_guard<std::mutex> lock(g_state.sinks_mutex);
        for (const Sink& s : g_state.sinks) {
            if (s.fn != nullptr) {
                s.fn(s.context, full);
            }
        }
    }
    g_state.lines.fetch_add(1, std::memory_order_relaxed);
}

// Renders and delivers the sorted batch, then empties it. Returns whether a Warn or worse was in it.
bool process_batch() noexcept {
    std::stable_sort(g_state.entries, g_state.entries + g_state.entry_count,
                     [](const BatchEntry& a, const BatchEntry& b) { return a.ticks < b.ticks; });
    bool urgent = false;
    for (usize i = 0; i < g_state.entry_count; ++i) {
        const BatchEntry& e = g_state.entries[i];
        const auto* h = reinterpret_cast<const RecordHeader*>(g_state.batch + e.offset);
        const char* payload = reinterpret_cast<const char*>(h + 1);
        Line line{};
        line.level = Level(h->level);
        line.category = Category(h->category);
        line.ticks = h->ticks;
        line.frame = h->frame;
        line.thread = e.slot == u16(~0) ? "log" : g_state.slots[e.slot].name.c_str();
        line.file = h->file;
        line.line = h->line;
        char rt_text[512];
        bool rt = false;
        if (h->kind == RecordKind::Rt) {
            detail::RtPayload p{};
            std::memcpy(&p, payload, sizeof(p));
            const int n = std::snprintf(rt_text, sizeof(rt_text), "%s = %llu", p.literal,
                                        static_cast<unsigned long long>(p.value));
            line.text = rt_text;
            line.length = n < 0 ? 0 : usize(n);
            rt = true;
        } else {
            line.text = payload;
            line.length = h->length;
        }
        urgent = urgent || line.level >= Level::Warn;
        deliver(line, rt);  // RT lines are never written synchronously, so stderr gets them here
    }
    g_state.entry_count = 0;
    g_state.batch_used = 0;
    return urgent;
}

// Copies one record into the batch; processes the batch first when it is full.
void batch_add(const RecordHeader& h, u16 slot) noexcept {
    if (g_state.batch_used + h.size > g_state.batch_capacity || g_state.entry_count == g_state.entry_capacity) {
        process_batch();
    }
    std::memcpy(g_state.batch + g_state.batch_used, &h, h.size);
    g_state.entries[g_state.entry_count++] = BatchEntry{h.ticks, u32(g_state.batch_used), slot};
    g_state.batch_used += h.size;
}

// A line produced by the logger itself (drops, its own warnings), added to the batch.
void batch_internal(Level level, const char* text) noexcept {
    alignas(8) u8 buf[sizeof(RecordHeader) + 256];
    auto* h = reinterpret_cast<RecordHeader*>(buf);
    const usize len = std::min<usize>(std::strlen(text), 255);
    *h = RecordHeader{};
    h->kind = RecordKind::Text;
    h->level = u8(level);
    h->category = u8(Category::log);
    h->ticks = now_ticks();
    h->frame = g_state.frame.load(std::memory_order_relaxed);
    h->file = __FILE__;
    h->line = __LINE__;
    h->length = u16(len);
    h->size = u16(detail::align_up(sizeof(RecordHeader) + len));
    std::memcpy(h + 1, text, len);
    batch_add(*h, u16(~0));
}

// Drains every ring into the batch and delivers it. Caller holds drain_mutex.
void drain_all() noexcept {
    for (usize i = 0; i < g_state.slot_count; ++i) {
        Slot& s = g_state.slots[i];
        const auto state = SlotState(s.state.load(std::memory_order_acquire));
        if (state == SlotState::Free) {
            continue;
        }
        s.ring.drain([&](const RecordHeader& h) { batch_add(h, u16(i)); });
        const u64 dropped = s.ring.dropped();
        if (dropped != s.reported_drops) {
            char msg[160];
            std::snprintf(msg, sizeof(msg), "%llu line(s) dropped on thread %s: its log ring was full",
                          static_cast<unsigned long long>(dropped - s.reported_drops), s.name.c_str());
            s.reported_drops = dropped;
            batch_internal(Level::Warn, msg);
        }
        if (state == SlotState::Retiring && s.ring.empty()) {
            s.reported_drops = s.ring.dropped();
            s.state.store(u8(SlotState::Free), std::memory_order_release);
        }
    }
    const bool urgent = process_batch();
    stderr_flush();
    if (g_state.file != nullptr) {
        const u64 now = now_ticks();
        const u64 period = u64(detail::cv_log_file_flush_ms.value()) * 1000000;
        if (urgent || now - g_state.file_last_flush >= period) {
            std::fflush(g_state.file);
            g_state.file_last_flush = now;
        }
    }
}

void follow_levels() noexcept {
    const u32 gen = detail::level_generation();
    if (gen == g_state.levels_generation) {
        return;
    }
    g_state.levels_generation = gen;
    u8 levels[category_capacity];
    detail::compute_levels(levels);
    for (usize c = 0; c < category_capacity; ++c) {
        detail::g_levels[c].store(levels[c], std::memory_order_relaxed);
    }
}

void log_thread_main() noexcept {
    detail::set_thread_name("ez-log");
    std::unique_lock<std::mutex> lock(g_state.mutex);
    for (;;) {
        const auto period = std::chrono::milliseconds(detail::cv_log_drain_ms.value());
        g_state.wake.wait_for(lock, period,
                              [] { return g_state.stop || g_state.flush_requested != g_state.flush_done; });
        const bool stopping = g_state.stop;
        const u64 request = g_state.flush_requested;
        lock.unlock();
        {
            const std::lock_guard<std::mutex> drain(g_state.drain_mutex);
            follow_levels();
            file_follow_cvar();
            drain_all();
        }
        lock.lock();
        g_state.flush_done = request;
        g_state.flushed.notify_all();
        if (stopping) {
            return;
        }
    }
}

void cvars_report(void*, cvars::Severity severity, std::string_view message) {
    switch (severity) {
        case cvars::Severity::Info:
            EZ_LOG_INFO(cvars, "%.*s", int(message.size()), message.data());
            break;
        case cvars::Severity::Warning:
            EZ_LOG_WARN(cvars, "%.*s", int(message.size()), message.data());
            break;
        case cvars::Severity::Error:
            EZ_LOG_ERROR(cvars, "%.*s", int(message.size()), message.data());
            break;
    }
}

usize round_up_pow2(usize n) noexcept {
    usize p = 1024;
    while (p < n) {
        p <<= 1;
    }
    return p;
}

// Before init and after shutdown: render on the caller and write to stderr directly.
void write_direct(Level level, Category c, const char* file, u32 line_no, const char* text, usize length) noexcept {
    Line line{};
    line.level = level;
    line.category = c;
    line.ticks = now_ticks();
    line.frame = g_state.frame.load(std::memory_order_relaxed);
    line.thread = "-";
    line.file = file;
    line.line = line_no;
    line.text = text;
    line.length = length;
    char out[render_capacity];
    detail::write_stderr(out, render(out, sizeof(out), line, false));
}

// The not-running path: begin() and commit() run on the same thread, so thread-locals carry the line.
thread_local char t_fallback[detail::max_line_cap + 1];
thread_local RecordHeader t_direct;

}  // namespace

State::~State() {
    if (!thread.joinable()) {
        return;
    }
    {
        const std::lock_guard<std::mutex> lock(mutex);
        stop = true;
    }
    wake.notify_all();
    thread.join();
    running.store(false, std::memory_order_release);
    file_close();
}

// ==================================================================== names

const char* category_name(Category c) noexcept {
    const usize i = usize(c);
    if (i < module_count) {
        return module_name(Module(i)).data();  // a view of a string literal: terminated
    }
    return i >= plugin_category_first ? "plugin" : "?";
}

const char* level_name(Level l) noexcept {
    constexpr const char* names[] = {"trace", "debug", "info", "warn", "error", "fatal"};
    return u8(l) < count_of(names) ? names[u8(l)] : "?";
}

void set_level(Category c, Level l) noexcept {
    detail::g_levels[u8(c)].store(u8(l), std::memory_order_relaxed);
}

// ==================================================================== the call site

bool detail::begin(Reservation& r, Level level, Category c, const char* file, u32 line) noexcept {
    Slot* slot = current_slot();
    if (slot == nullptr) {
        if (g_state.running.load(std::memory_order_acquire)) {
            return false;  // pool exhausted: counted in current_slot()
        }
        // Not running: format into a thread-local buffer; commit() writes it to stderr directly.
        t_direct = RecordHeader{};
        t_direct.level = u8(level);
        t_direct.category = u8(c);
        t_direct.file = file;
        t_direct.line = line;
        r.text = t_fallback;
        r.capacity = max_line_cap + 1;
        r.record = nullptr;
        return true;
    }
    const usize max_line = usize(cv_log_max_line.value());
    RecordHeader* h = slot->ring.reserve(sizeof(RecordHeader) + max_line + 1);
    if (h == nullptr) {
        slot->ring.count_drop();
        return false;
    }
    h->level = u8(level);
    h->category = u8(c);
    h->kind = RecordKind::Text;
    h->truncated = 0;
    h->ticks = now_ticks();
    h->frame = g_state.frame.load(std::memory_order_relaxed);
    h->file = file;
    h->line = line;
    r.text = reinterpret_cast<char*>(h + 1);
    r.capacity = max_line + 1;
    r.record = h;
    return true;
}

void detail::commit(Reservation& r, int formatted_length) noexcept {
    usize length = formatted_length < 0 ? 0 : usize(formatted_length);
    const bool truncated = length >= r.capacity;
    if (truncated) {
        length = r.capacity - 1;
        if (length >= 3) {
            std::memcpy(r.text + length - 3, "...", 3);
        }
    }
    if (r.record == nullptr) {  // not running: straight to stderr
        write_direct(Level(t_direct.level), Category(t_direct.category), t_direct.file, t_direct.line, r.text, length);
        return;
    }
    auto* h = static_cast<RecordHeader*>(r.record);
    h->length = u16(length);
    h->truncated = truncated ? 1 : 0;
    h->size = u16(align_up(sizeof(RecordHeader) + length));
    t_slot.slot->ring.commit(h);

    if (const ProfilerHook hook = g_state.profiler.load(std::memory_order_relaxed)) {
        hook(r.text, length, Level(h->level));  // at the call site: exact time, current zone
    }
    if (g_state.sync.load(std::memory_order_relaxed)) {
        Line line{};
        line.level = Level(h->level);
        line.category = Category(h->category);
        line.ticks = h->ticks;
        line.frame = h->frame;
        line.thread = t_slot.slot->name.c_str();
        line.file = h->file;
        line.line = h->line;
        line.text = r.text;
        line.length = length;
        char out[render_capacity];
        write_stderr(out, render(out, sizeof(out), line, g_state.color));
    }
}

void detail::emit_rt(Category c, Level level, const char* literal, u64 value) noexcept {
    Slot* slot = current_slot();
    if (slot == nullptr) {
        if (!g_state.running.load(std::memory_order_acquire)) {
            char text[256];
            const int n =
                std::snprintf(text, sizeof(text), "%s = %llu", literal, static_cast<unsigned long long>(value));
            write_direct(level, c, "", 0, text, usize(n > 0 ? n : 0));
        }
        return;
    }
    RecordHeader* h = slot->ring.reserve(sizeof(RecordHeader) + sizeof(RtPayload));
    if (h == nullptr) {
        slot->ring.count_drop();
        return;
    }
    h->level = u8(level);
    h->category = u8(c);
    h->kind = RecordKind::Rt;
    h->truncated = 0;
    h->ticks = now_ticks();
    h->frame = g_state.frame.load(std::memory_order_relaxed);
    h->file = "";
    h->line = 0;
    h->length = u16(sizeof(RtPayload));
    h->size = u16(align_up(sizeof(RecordHeader) + sizeof(RtPayload)));
    const RtPayload p{literal, value};
    std::memcpy(h + 1, &p, sizeof(p));
    slot->ring.commit(h);
}

namespace {

// Writes a line now, on the calling thread, after everything already queued: the profiler hook,
// a drain of every ring, then the line to every sink, then flushes. For fatal lines and failed
// assertions, where the process may end next.
void write_urgent(Level level, Category c, const char* file, u32 line, const char* text, usize length) noexcept {
    if (const ProfilerHook hook = g_state.profiler.load(std::memory_order_relaxed)) {
        hook(text, length, level);
    }
    if (!g_state.running.load(std::memory_order_acquire)) {
        write_direct(level, c, file, line, text, length);
        return;
    }
    const std::lock_guard<std::mutex> drain(g_state.drain_mutex);
    drain_all();
    Line l{};
    l.level = level;
    l.category = c;
    l.ticks = now_ticks();
    l.frame = g_state.frame.load(std::memory_order_relaxed);
    l.thread = t_slot.slot != nullptr ? t_slot.slot->name.c_str() : "-";
    l.file = file;
    l.line = line;
    l.text = text;
    l.length = length;
    deliver(l, true);
    stderr_flush();
    if (g_state.file != nullptr) {
        std::fflush(g_state.file);
    }
}

// Installed as the base assert handler while the logger runs (specs/base.md BA-4): a failed
// assertion is written after every pending line, so what led up to it is not lost in the rings.
ez::AssertHandler g_previous_assert_handler = nullptr;

void assert_handler(const ez::AssertInfo& info) {
    char text[1024];
    const int n = info.message[0] != '\0'
                      ? std::snprintf(text, sizeof(text), "assertion failed: %s (%s)", info.expression, info.message)
                      : std::snprintf(text, sizeof(text), "assertion failed: %s", info.expression);
    write_urgent(Level::Fatal, Category::base, info.file, info.line, text,
                 n < 0 ? 0 : std::min(usize(n), sizeof(text) - 1));
}

}  // namespace

void detail::fatal(Category c, const char* file, u32 line, const char* text) noexcept {
    const usize length = std::strlen(text);
    if (t_in_fatal) {  // fatal while handling fatal: write and stop
        detail::write_stderr(text, length);
        std::abort();
    }
    t_in_fatal = true;
    write_urgent(Level::Fatal, c, file, line, text, length);
    if (const FatalHook hook = g_state.fatal.load(std::memory_order_acquire)) {
        hook(text, length);
    }
    std::abort();
}

// ==================================================================== lifecycle

bool init(cvars::Registry& registry) noexcept {
    if (g_state.running.load(std::memory_order_acquire)) {
        return true;
    }
    using namespace detail;  // NOLINT(google-build-using-namespace) the module's own detail

    g_state.wall_epoch_ns = now_unix_ns();
    g_state.steady_epoch = now_ticks();
    const bool terminal = prepare_stderr();
    const char* no_color = std::getenv("NO_COLOR");
    switch (cv_log_color.value()) {
        case Toggle::On:
            g_state.color = true;
            break;
        case Toggle::Off:
            g_state.color = false;
            break;
        case Toggle::Auto:
            g_state.color = terminal && (no_color == nullptr || no_color[0] == '\0');
            break;
    }
    switch (cv_log_sync.value()) {
        case Toggle::On:
            g_state.sync.store(true);
            break;
        case Toggle::Off:
            g_state.sync.store(false);
            break;
        case Toggle::Auto:
            g_state.sync.store(debugger_attached());
            break;
    }

    // Everything is allocated here, once (LG-5).
    g_state.slot_count = std::min<usize>(usize(cv_log_threads_max.value()), slot_limit);
    g_state.ring_bytes = round_up_pow2(usize(cv_log_ring_kib.value()) * 1024);
    g_state.ring_memory = static_cast<u8*>(
        ::operator new(g_state.slot_count * g_state.ring_bytes, std::align_val_t(EZ_CACHE_LINE), std::nothrow));
    g_state.batch_capacity = 256 * 1024;
    g_state.batch = static_cast<u8*>(::operator new(g_state.batch_capacity, std::align_val_t(8), std::nothrow));
    g_state.entry_capacity = g_state.batch_capacity / sizeof(RecordHeader);
    g_state.entries =
        static_cast<BatchEntry*>(::operator new(g_state.entry_capacity * sizeof(BatchEntry), std::nothrow));
    g_state.history_capacity = usize(cv_log_history_kib.value()) * 1024;
    g_state.history = g_state.history_capacity > 0
                          ? static_cast<char*>(::operator new(g_state.history_capacity, std::nothrow))
                          : nullptr;
    if (g_state.ring_memory == nullptr || g_state.batch == nullptr || g_state.entries == nullptr ||
        (g_state.history_capacity > 0 && g_state.history == nullptr)) {
        const char msg[] = "log: out of memory at init; logging to stderr only\n";
        write_stderr(msg, sizeof(msg) - 1);
        return false;
    }
    for (usize i = 0; i < g_state.slot_count; ++i) {
        g_state.slots[i].ring.attach(g_state.ring_memory + i * g_state.ring_bytes, g_state.ring_bytes);
        g_state.slots[i].state.store(u8(SlotState::Free));
        g_state.slots[i].reported_drops = 0;
    }
    g_state.history_written = 0;
    g_state.file_keep = cv_log_file_keep.value();
    if (!cv_log_file_dir.value().empty()) {
        g_state.file_dir.assign(cv_log_file_dir.value());
    } else if (!default_log_dir(g_state.file_dir)) {
        g_state.file_dir.assign(".");
    }
    g_state.levels_generation = ~0u;
    follow_levels();

    g_state.stop = false;
    g_state.flush_requested = 0;
    g_state.flush_done = 0;
    g_state.registry = &registry;
    g_state.epoch.fetch_add(1, std::memory_order_acq_rel);
    g_state.running.store(true, std::memory_order_release);
    g_state.thread = std::thread(&log_thread_main);

    registry.set_report_hook(&cvars_report, nullptr);  // delivers what cvars buffered at startup
    g_previous_assert_handler = set_assert_handler(&assert_handler);
    if (t_slot.slot == nullptr || t_slot.epoch != g_state.epoch.load()) {
        register_thread("main");
    }
    return true;
}

void shutdown() noexcept {
    if (!g_state.running.load(std::memory_order_acquire)) {
        return;
    }
    if (g_state.registry != nullptr) {
        g_state.registry->set_report_hook(nullptr, nullptr);
    }
    set_assert_handler(g_previous_assert_handler);
    {
        const std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.stop = true;
    }
    g_state.wake.notify_all();
    g_state.thread.join();  // the thread drains once more before it returns
    g_state.running.store(false, std::memory_order_release);
    g_state.epoch.fetch_add(1, std::memory_order_acq_rel);  // stale thread slots claim again next time
    file_close();
    ::operator delete(g_state.ring_memory, std::align_val_t(EZ_CACHE_LINE));
    ::operator delete(g_state.batch, std::align_val_t(8));
    ::operator delete(g_state.entries);
    {
        const std::lock_guard<std::mutex> lock(g_state.history_mutex);
        ::operator delete(g_state.history);
        g_state.history = nullptr;
        g_state.history_capacity = 0;
    }
    g_state.ring_memory = nullptr;
    g_state.batch = nullptr;
    g_state.entries = nullptr;
    for (usize i = 0; i < g_state.slot_count; ++i) {
        g_state.slots[i].state.store(u8(SlotState::Free));
    }
    t_slot.slot = nullptr;
}

void flush() noexcept {
    if (!g_state.running.load(std::memory_order_acquire)) {
        return;
    }
    std::unique_lock<std::mutex> lock(g_state.mutex);
    const u64 ticket = ++g_state.flush_requested;
    g_state.wake.notify_all();
    g_state.flushed.wait(lock, [ticket] { return g_state.flush_done >= ticket || g_state.stop; });
}

void register_thread(const char* name) noexcept {
    detail::set_thread_name(name);
    if (Slot* s = current_slot()) {
        s->name.assign(name);
    }
}

void set_frame(u32 frame) noexcept {
    g_state.frame.store(frame, std::memory_order_relaxed);
}

bool add_sink(SinkFn fn, void* context) noexcept {
    const std::lock_guard<std::mutex> lock(g_state.sinks_mutex);
    for (Sink& s : g_state.sinks) {
        if (s.fn == nullptr) {
            s = Sink{fn, context};
            return true;
        }
    }
    return false;
}

void remove_sink(SinkFn fn, void* context) noexcept {
    const std::lock_guard<std::mutex> lock(g_state.sinks_mutex);
    for (Sink& s : g_state.sinks) {
        if (s.fn == fn && s.context == context) {
            s = Sink{};
        }
    }
}

void set_profiler_hook(ProfilerHook hook) noexcept {
    g_state.profiler.store(hook, std::memory_order_relaxed);
}
void set_fatal_hook(FatalHook hook) noexcept {
    g_state.fatal.store(hook, std::memory_order_release);
}

void read_history(void (*fn)(void* context, const char* text, usize length), void* context) noexcept {
    const std::lock_guard<std::mutex> lock(g_state.history_mutex);
    const usize cap = g_state.history_capacity;
    if (cap == 0 || g_state.history_written == 0) {
        return;
    }
    // Oldest complete line first: when the buffer has wrapped, skip the partial line at its start.
    const u64 end = g_state.history_written;
    u64 start = end > cap ? end - cap : 0;
    if (end > cap) {
        while (start < end && g_state.history[start % cap] != '\n') {
            ++start;
        }
        ++start;
    }
    char line[render_capacity];
    usize n = 0;
    for (u64 i = start; i < end; ++i) {
        const char ch = g_state.history[i % cap];
        if (n < sizeof(line)) {
            line[n++] = ch;
        }
        if (ch == '\n') {
            fn(context, line, n);
            n = 0;
        }
    }
}

Stats stats() noexcept {
    Stats s{};
    s.lines = g_state.lines.load(std::memory_order_relaxed);
    s.dropped = g_state.pool_drops.load(std::memory_order_relaxed);
    for (usize i = 0; i < g_state.slot_count; ++i) {
        s.dropped += g_state.slots[i].ring.dropped();
    }
    return s;
}

}  // namespace ez::log

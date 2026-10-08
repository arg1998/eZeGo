// Ring 3 of memory observability (specs/observability.md §4): global operator new and delete are
// replaced so C++ allocations made by third-party code appear in Tracy. Profile builds only, and
// linked into executables as an object library (ez_metrics_new_hooks), never through the static
// library, so the linker always takes these definitions (linking.md L-6).
//
// Why a naive hook recurses, and why this one does not:
//   - the hook calls the real allocator (malloc) directly, never operator new
//   - a thread-local "inside" flag makes any allocation made while reporting bypass the hook
//   - nothing is reported unless the profiler is running (manual lifetime)
#include "ez/base/build.hpp"
#include "ez/metrics/profiler.hpp"

#include <cstddef>
#include <cstdlib>
#include <new>

#if EZ_MEM_TRACE

namespace {

constexpr const char* untracked_pool = "untracked/operator-new";
thread_local bool t_inside = false;

void* hooked_alloc(std::size_t size, std::size_t align) {
    if (size == 0) {
        size = 1;
    }
    void* p = nullptr;
    if (align <= alignof(std::max_align_t)) {
        p = std::malloc(size);
    } else {
    #if EZ_OS_WINDOWS
        p = _aligned_malloc(size, align);
    #else
        if (posix_memalign(&p, align, size) != 0) {
            p = nullptr;
        }
    #endif
    }
    if (p != nullptr && !t_inside) {
        t_inside = true;
        ez::metrics::profiler_alloc(p, size, untracked_pool);
        t_inside = false;
    }
    return p;
}

void hooked_free(void* p, [[maybe_unused]] bool aligned) {
    if (p == nullptr) {
        return;
    }
    if (!t_inside) {
        t_inside = true;
        ez::metrics::profiler_free(p, untracked_pool);
        t_inside = false;
    }
    #if EZ_OS_WINDOWS
    if (aligned) {
        _aligned_free(p);
        return;
    }
    #endif
    std::free(p);
}

}  // namespace

void* operator new(std::size_t n) {
    if (void* p = hooked_alloc(n, 0)) {
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    if (void* p = hooked_alloc(n, 0)) {
        return p;
    }
    throw std::bad_alloc();
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
    return hooked_alloc(n, 0);
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
    return hooked_alloc(n, 0);
}
void* operator new(std::size_t n, std::align_val_t a) {
    if (void* p = hooked_alloc(n, std::size_t(a))) {
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t n, std::align_val_t a) {
    if (void* p = hooked_alloc(n, std::size_t(a))) {
        return p;
    }
    throw std::bad_alloc();
}
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
    return hooked_alloc(n, std::size_t(a));
}
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
    return hooked_alloc(n, std::size_t(a));
}

void operator delete(void* p) noexcept {
    hooked_free(p, false);
}
void operator delete[](void* p) noexcept {
    hooked_free(p, false);
}
void operator delete(void* p, std::size_t) noexcept {
    hooked_free(p, false);
}
void operator delete[](void* p, std::size_t) noexcept {
    hooked_free(p, false);
}
void operator delete(void* p, const std::nothrow_t&) noexcept {
    hooked_free(p, false);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept {
    hooked_free(p, false);
}
void operator delete(void* p, std::align_val_t) noexcept {
    hooked_free(p, true);
}
void operator delete[](void* p, std::align_val_t) noexcept {
    hooked_free(p, true);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept {
    hooked_free(p, true);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept {
    hooked_free(p, true);
}

#endif  // EZ_MEM_TRACE

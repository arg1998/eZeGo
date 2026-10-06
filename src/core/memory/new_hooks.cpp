// Ring 3 memory observability (specs/observability.md §4): replace global operator new/delete so
// C++ allocations made by third-party code (and anything not going through ezAllocate) show up in
// Tracy. Compiled ONLY into profile builds, and directly into executables (not ez_core) so the
// linker always picks these definitions (linking.md L-6).
//
// Why a naive malloc hook recurses, and why this one does not:
//   - the hook calls the real allocator (malloc) directly, never operator new
//   - a thread_local "inside" flag makes any allocation performed while REPORTING bypass the hook
//   - nothing is reported unless the profiler is alive (manual lifetime, profilerIsAlive())

#include <cstdlib>
#include <new>

#include "core/profiler/profiler.hpp"

#if !EZ_MEM_TRACE
    #error "new_hooks.cpp must only be compiled when EZ_MEM_TRACE is on (profile preset)"
#endif

namespace {
constexpr const char* k_pool = "untracked/operator-new";
thread_local bool t_inside = false;

void* hooked_alloc(std::size_t size, std::size_t align) {
    if (size == 0) size = 1;
    void* p = nullptr;
    if (align <= alignof(std::max_align_t)) {
        p = std::malloc(size);
    } else {
#if defined(_WIN32)
        p = _aligned_malloc(size, align);
#else
        if (posix_memalign(&p, align, size) != 0) p = nullptr;
#endif
    }
    if (p && !t_inside) {
        t_inside = true;
        profilerAlloc(p, size, k_pool);
        t_inside = false;
    }
    return p;
}

void hooked_free(void* p, bool aligned) {
    if (!p) return;
    if (!t_inside) {
        t_inside = true;
        profilerFree(p, k_pool);
        t_inside = false;
    }
#if defined(_WIN32)
    if (aligned) {
        _aligned_free(p);
        return;
    }
#endif
    (void)aligned;
    std::free(p);
}
}  // namespace

void* operator new(std::size_t n) {
    if (void* p = hooked_alloc(n, 0)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    if (void* p = hooked_alloc(n, 0)) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return hooked_alloc(n, 0); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return hooked_alloc(n, 0); }
void* operator new(std::size_t n, std::align_val_t a) {
    if (void* p = hooked_alloc(n, std::size_t(a))) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n, std::align_val_t a) {
    if (void* p = hooked_alloc(n, std::size_t(a))) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
    return hooked_alloc(n, std::size_t(a));
}
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
    return hooked_alloc(n, std::size_t(a));
}

void operator delete(void* p) noexcept { hooked_free(p, false); }
void operator delete[](void* p) noexcept { hooked_free(p, false); }
void operator delete(void* p, std::size_t) noexcept { hooked_free(p, false); }
void operator delete[](void* p, std::size_t) noexcept { hooked_free(p, false); }
void operator delete(void* p, const std::nothrow_t&) noexcept { hooked_free(p, false); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { hooked_free(p, false); }
void operator delete(void* p, std::align_val_t) noexcept { hooked_free(p, true); }
void operator delete[](void* p, std::align_val_t) noexcept { hooked_free(p, true); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { hooked_free(p, true); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { hooked_free(p, true); }

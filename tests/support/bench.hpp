// The micro-benchmark helper (specs/testing.md §4, §9): warm-up, samples until a minimum duration,
// min / median / p99 per operation, and a local history with the delta against the previous run.
//
//   EZ_BENCH("log: a filtered line") {
//       while (state.next()) {     // only the loop is timed
//           EZ_LOG_DEBUG(log, "x %d", 1);
//       }
//   }
//
// Bench executables are built in every tree but run by `ez bench`, which uses the release tree.
#pragma once

#include "ez/base/macros.hpp"
#include "ez/base/types.hpp"

namespace ez::test {

class BenchState {
public:
    // True `batch` times per call of the benchmark; the time between the first call and the last
    // is the sample.
    bool next() noexcept;
    [[nodiscard]] u64 batch() const noexcept { return batch_; }

private:
    friend struct BenchRunner;
    u64 batch_ = 1;
    u64 remaining_ = 0;
    u64 start_ns_ = 0;
    u64 elapsed_ns_ = 0;
    bool started_ = false;
};

using BenchFn = void (*)(BenchState& state);

struct BenchRegistrar {
    BenchRegistrar(const char* name, BenchFn fn) noexcept;
};

// Keeps the compiler from deleting a computation whose result is otherwise unused.
template <class T>
EZ_FORCE_INLINE void do_not_optimize(const T& value) noexcept {
#if EZ_COMPILER_MSVC
    const volatile T* sink = &value;
    (void)sink;
#else
    asm volatile("" : : "r,m"(value) : "memory");
#endif
}

}  // namespace ez::test

#define EZ_DETAIL_BENCH(id, name)                                                                           \
    static void EZ_CONCAT(ez_bench_fn_, id)(::ez::test::BenchState & state);                                \
    static const ::ez::test::BenchRegistrar EZ_CONCAT(g_ez_bench_, id){name, &EZ_CONCAT(ez_bench_fn_, id)}; \
    static void EZ_CONCAT(ez_bench_fn_, id)(::ez::test::BenchState & state)
#define EZ_BENCH(name) EZ_DETAIL_BENCH(__COUNTER__, name)

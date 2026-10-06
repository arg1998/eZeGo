#pragma once
// Minimal test harness. The test framework is still an open question (build-system.md §6 #8);
// this keeps the experiment dependency-free. Each test file has its own main() that calls its
// test functions explicitly (no static-initializer registration, linking.md L-6).

#include <cmath>
#include <cstdio>

inline int g_failures = 0;
inline int g_checks = 0;

#define CHECK(cond)                                                              \
  do {                                                                           \
    ++g_checks;                                                                  \
    if (!(cond)) {                                                               \
      ++g_failures;                                                              \
      std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);              \
    }                                                                            \
  } while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::fabs(double(a) - double(b)) <= double(eps))

#define RUN(fn)                      \
  do {                               \
    std::printf("[ run ] %s\n", #fn); \
    fn();                            \
  } while (0)

inline int finish() {
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

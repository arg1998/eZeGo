# eZeGo — Base Layer

> **Status:** Accepted and implemented, 2026-10-08. Decision IDs (`BA-n`) are indexed in [`README.md`](./README.md).
> **Companions:** [`code-organization.md`](./code-organization.md) CO-8, CO-9 · [`naming.md`](./naming.md) §4.1, §7 · [`build-system.md`](./build-system.md) B-3 · [`observability.md`](./observability.md) §6
> **Scope:** `src/ez/base/`, namespace `ez`: what every other module may include for free. Assertion tiers beyond the debug assert, status codes and the crash reporter belong to the `crash` module and are not designed yet; containers arrive when a second module needs the same one.

---

## 1. Rules for this module (BA-1)

- **Cheap to include.** Every first-party file includes something from here. Public headers include only `<cstddef>`, `<cstdint>` and, where unavoidable, `<string_view>`; never `<string>`, `<vector>`, `<functional>`, `<iostream>`, `<format>`.
- **Depends on nothing.** Layer 0, first in the module table, no first-party dependency.
- **Namespace `ez`**, not `ez::base` (CO-9). Private helpers in `ez::detail`.
- **Grows only by pull.** Something moves into base when a second module needs it, not before.

## 2. Contents (BA-2)

| Header | Content |
|---|---|
| `ez/base/detect.h` | C-compatible. `EZ_OS_LINUX`, `EZ_OS_MACOS`, `EZ_OS_WINDOWS`, `EZ_OS_POSIX`; `EZ_ARCH_X64`, `EZ_ARCH_ARM64`; `EZ_COMPILER_CLANG`, `EZ_COMPILER_MSVC`, `EZ_COMPILER_GCC`. **Every macro is defined as 0 or 1**, so `#if` with `-Wundef` catches typos. `EZ_CACHE_LINE`: 128 on Apple Silicon, 64 elsewhere. `EZ_EXPORT` for the SDK. Unsupported targets are an `#error`: 64-bit only, Apple Silicon only on macOS. Compilers are detected, never gated: this header is shared with the SDK and plugins may use any compiler; `cmake/toolchain.cmake` decides what builds the host (B-11). |
| `ez/base/build.hpp` | Feature macros from the build (`EZ_ASSERTS`, `EZ_PROFILER`, `EZ_MEM_TRACE`, `EZ_METRICS`, `EZ_LOG_LEVEL`), checked to be defined, with defaults only when a file is compiled outside CMake. `ez::build_version`, `ez::build_mode`, `ez::build_compiler`, `ez::build_os`, `ez::build_arch` as `constexpr` strings, for `--version`, the about screen and crash reports. |
| `ez/base/types.hpp` | `u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 b8 usize isize` in `ez`, with size checks (N-4 §4.1). `ez::count_of(array)`. |
| `ez/base/macros.hpp` | `EZ_FORCE_INLINE`, `EZ_NO_INLINE`, `EZ_LIKELY(x)`, `EZ_UNLIKELY(x)`, `EZ_UNREACHABLE()`, `EZ_ASSUME(x)`, `EZ_DEBUG_BREAK()`, `EZ_PRINTF_FORMAT(fmt, first)`, `EZ_STRINGIFY`, `EZ_CONCAT`, `EZ_UNIQUE_NAME(prefix)` |
| `ez/base/assert.hpp` | `EZ_ASSERT(expr)` and `EZ_ASSERT_MSG(expr, "literal")`: compiled in when `EZ_ASSERTS`, otherwise the expression is not evaluated but must still compile. A failure calls the **assert handler**, then traps. `ez::set_assert_handler(fn)` is the seam the crash reporter fills later; the default handler writes one line to stderr. |
| `ez/base/fixed_string.hpp` | `ez::FixedString<N>`: inline character buffer, always terminated, truncating assignment, no allocation. Needed by string cvars. |
| `ez/base/hash.hpp` | `ez::hash64(std::string_view)`: FNV-1a, `constexpr`, identical on every platform and standard library, so hashes can be stored and compared across machines. |
| `ez/base/modules.hpp` | `enum class ez::Module : u8` and `ez::module_name(Module)` from the generated `ez/base/modules.gen.hpp` (CO-8); `ez::module_count`. |

## 3. Decisions

| ID | Decision |
|---|---|
| BA-1 | Base is cheap to include, depends on nothing, uses namespace `ez`, and grows only when a second module needs something |
| BA-2 | The contents of §2 |
| BA-3 | Detection macros are always defined as 0 or 1 and live in a C header, so the SDK and the host share one definition; detection never rejects a compiler, the toolchain does |
| BA-4 | The debug assertion has a replaceable handler; the crash module installs its reporter there when it exists |
| BA-5 | The module table is generated into the build tree and exposed as an enum; log categories, cvar prefixes and memory tags derive from it |

## 4. Deferred

| Topic | Revisit when |
|---|---|
| Status codes and result type | Errors and crash spec |
| Assertion tiers `ensure` and `fatal`, stack capture | Errors and crash spec |
| Handles, pools, fixed vectors, SPSC rings as shared containers | A second module needs one; the logger's ring stays private until then |
| Strong time types (`Ticks`, durations) | Platform clock spec |

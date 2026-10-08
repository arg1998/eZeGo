// Primitive types (specs/naming.md §4.1, specs/base.md §2). Spelled like built-in types because
// they are used like them; the only lowercase type names in the codebase.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ez {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;
using b8 = bool;
using usize = std::size_t;
using isize = std::ptrdiff_t;

static_assert(sizeof(u8) == 1 && sizeof(u16) == 2 && sizeof(u32) == 4 && sizeof(u64) == 8);
static_assert(sizeof(i8) == 1 && sizeof(i16) == 2 && sizeof(i32) == 4 && sizeof(i64) == 8);
static_assert(sizeof(f32) == 4 && sizeof(f64) == 8);
static_assert(sizeof(void*) == 8, "eZeGo is 64-bit only");
static_assert(sizeof(usize) == 8 && sizeof(isize) == 8);

// Number of elements in a C array, checked at compile time (a pointer does not compile).
template <class T, usize N>
constexpr usize count_of(const T (&)[N]) noexcept {
    return N;
}

}  // namespace ez

// The module table as C++ (specs/code-organization.md CO-8, specs/base.md BA-5).
// ez/base/modules.gen.hpp is generated from modules.cmake into the build tree; it defines
// EZ_MODULES(X) as X(name, "name", layer) for every module. Log categories, cvar prefixes and
// memory tags are built from it.
#pragma once

#include "ez/base/modules.gen.hpp"
#include "ez/base/types.hpp"

#include <string_view>

namespace ez {

enum class Module : u8 {
#define EZ_DETAIL_MODULE_ENUM(name, text, layer) name,
    EZ_MODULES(EZ_DETAIL_MODULE_ENUM)
#undef EZ_DETAIL_MODULE_ENUM
};

}  // namespace ez

namespace ez::detail {
inline constexpr const char* module_names[] = {
#define EZ_DETAIL_MODULE_NAME(name, text, layer) text,
    EZ_MODULES(EZ_DETAIL_MODULE_NAME)
#undef EZ_DETAIL_MODULE_NAME
};
inline constexpr u8 module_layers[] = {
#define EZ_DETAIL_MODULE_LAYER(name, text, layer) layer,
    EZ_MODULES(EZ_DETAIL_MODULE_LAYER)
#undef EZ_DETAIL_MODULE_LAYER
};
}  // namespace ez::detail

namespace ez {

inline constexpr usize module_count = count_of(detail::module_names);

constexpr std::string_view module_name(Module m) noexcept {
    return detail::module_names[static_cast<usize>(m)];
}
constexpr u8 module_layer(Module m) noexcept {
    return detail::module_layers[static_cast<usize>(m)];
}

}  // namespace ez

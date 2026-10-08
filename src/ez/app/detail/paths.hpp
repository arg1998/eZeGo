// The executable's directory, to find assets copied next to it. A stub until the platform module.
#pragma once

#include "ez/base/fixed_string.hpp"

namespace ez::app::detail {

// Directory of the running executable, '/' separators, no trailing '/'. Empty if unknown.
const FixedString<1023>& executable_dir() noexcept;

}  // namespace ez::app::detail

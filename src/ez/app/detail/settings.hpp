#pragma once

#include "ez/cvars/cvar.hpp"

namespace ez::app::detail {

extern cvars::CVar<f64> cv_app_quit_after_s;
extern cvars::CVar<i64> cv_app_quit_after_frames;
extern cvars::CVar<bool> cv_app_developer;

}  // namespace ez::app::detail

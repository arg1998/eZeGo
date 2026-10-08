// Cvars shared by the cvars tests. Declared in tests/cvars/fixtures.cpp.
#pragma once

#include "ez/cvars/cvar.hpp"
#include "ez/cvars/registry.hpp"

#include <memory>
#include <string>

namespace ez::test {

enum class Backend : i32 { Auto, Alsa, Pulse };

extern cvars::CVar<bool> cv_watch;
extern cvars::CVar<i32> cv_drain_ms;
extern cvars::CVar<f32> cv_gain;
extern cvars::CVar<i64> cv_big;
extern cvars::CVarEnum<Backend> cv_backend;
extern cvars::CVarString cv_dir;
extern cvars::CVar<i32> cv_ring_kib;
extern cvars::CVar<bool> cv_secret;

// A fresh registry with every fixture cvar registered. The registry is large (the table is fixed),
// so it lives on the heap in tests. Each test case runs in its own process, so registering the
// same global cvars in each case is safe.
std::unique_ptr<cvars::Registry> make_registry();

// A Writer that appends to a string.
cvars::Writer string_writer(std::string& out);

}  // namespace ez::test

#include "host/persist.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <imgui.h>

#include <cstdlib>

EM_JS_DEPS(design_persist, "$UTF8ToString,$stringToNewUTF8");

// The bodies below are JavaScript; clang-format would rewrite `===` as C++.
// clang-format off

// A malloc'd copy of the stored text, or null. Storage can be unavailable (private windows,
// blocked site data); the design then starts from defaults.
EM_JS(char*, design_storage_read, (const char* key), {
    try {
        const value = localStorage.getItem(UTF8ToString(key));
        return value === null ? 0 : stringToNewUTF8(value);
    } catch (e) {
        return 0;
    }
});

EM_JS(void, design_storage_write, (const char* key, const char* value), {
    try {
        localStorage.setItem(UTF8ToString(key), UTF8ToString(value));
    } catch (e) {
    }
});

// clang-format on

namespace design::host {
namespace {

constexpr const char* storage_key = "ezego-design/imgui.ini";

void save() {
    design_storage_write(storage_key, ImGui::SaveIniSettingsToMemory());
    ImGui::GetIO().WantSaveIniSettings = false;
}

const char* on_unload(int, const void*, void*) {
    save();
    return nullptr;  // anything else asks the user to confirm leaving the page
}

}  // namespace

void load() {
    if (char* ini = design_storage_read(storage_key)) {
        ImGui::LoadIniSettingsFromMemory(ini);
        std::free(ini);
    }
    emscripten_set_beforeunload_callback(nullptr, on_unload);
}

void save_if_dirty() {
    if (ImGui::GetIO().WantSaveIniSettings) {
        save();
    }
}

}  // namespace design::host

#include "commands.hpp"

#include "core/logger/logger.hpp"

#include <algorithm>
#include <tracy/Tracy.hpp>

std::shared_ptr<CommandRegistry> global_command_registery;
b8 is_initialized = false;

b8 initCommandSystem() {
    if (is_initialized) {
        EZ_LOG_ERROR("Connot initialize Command system as it is already initialized");
    } else {
        // TODO(Argosta): decide wether we go by smart pointers or regular
        global_command_registery = std::make_shared<CommandRegistry>();
        is_initialized = true;
    }

    return true;
}
void shutdownCommandSystem() {
    global_command_registery.reset();
    is_initialized = false;
}

std::shared_ptr<CommandRegistry> getCommandRegistry() {
    if (!is_initialized) {
        EZ_LOG_FATAL("Cannot access CommandRegistry as it is not initialized yet.");
        return nullptr;
    }

    return global_command_registery;
}

//-------------------------------------------------------------------------------------------
//                              CommandContext Class
//-------------------------------------------------------------------------------------------

CommandContext::CommandContext(const std::string owner_uid) : owner(owner_uid) {}

std::string CommandContext::getOwner() {
    return owner;
}

b8 CommandContext::containsCommand(const InputEventCode event_cmd) {
    if (isCommandKeyChord(event_cmd)) {
        return commands_chord.find(event_cmd) != commands_chord.end();
    }
    return commands_single.find(event_cmd) != commands_single.end();
}

b8 CommandContext::isCommandInitated(const InputEventCode event_cmd) {
    if (!containsCommand(event_cmd)) {
        EZ_LOG_ERROR("Command state cannot be determined for [%s] CommandContext. [%d] input event code is not bound yet.", owner.c_str(), event_cmd);
        return false;
    }

    if (isCommandKeyChord(event_cmd)) {
        return commands_chord[event_cmd] != COMMAND_IDLE;
    }

    return commands_single[event_cmd] == COMMAND_INITIATED;
}

b8 CommandContext::isCommandCommitted(const InputEventCode event_cmd) {
    if (!containsCommand(event_cmd)) {
        EZ_LOG_ERROR("Command state cannot be determined for [%s] CommandContext. [%d] input event code is not bound yet.", owner.c_str(), event_cmd);
        return false;
    }

    if (isCommandKeyChord(event_cmd)) {
        return commands_chord[event_cmd] != COMMAND_IDLE;
    }

    return commands_single[event_cmd] == COMMAND_COMMITTED;
}

b8 CommandContext::isCommandIdle(const InputEventCode event_cmd) {
    if (!containsCommand(event_cmd)) {
        EZ_LOG_ERROR("Command state cannot be determined for [%s] CommandContext. [%d] input event code is not bound yet.", owner.c_str(), event_cmd);
        return false;
    }

    if (isCommandKeyChord(event_cmd)) {
        return commands_chord[event_cmd] == COMMAND_IDLE;
    }

    return commands_single[event_cmd] == COMMAND_IDLE;
}
b8 CommandContext::isCommandKeyChord(const InputEventCode event_cmd) {
    const b8 has_mods = (ImGuiKey(event_cmd) & ImGuiMod_Mask_) > 0;
    const ImGuiKey _key = ImGuiKey(event_cmd & (0x0FFF));
    const b8 has_keys = (_key < ImGuiKey_NamedKey_END) && (_key >= ImGuiKey_NamedKey_BEGIN);

    return (has_mods && has_keys);
}

b8 CommandContext::isCommandKeySingle(const InputEventCode event_cmd) {
    const b8 has_mods = (ImGuiKey(event_cmd) & ImGuiMod_Mask_) > 0;
    const ImGuiKey _key = ImGuiKey(event_cmd & (0x0FFF));
    const b8 has_keys = (_key < ImGuiKey_NamedKey_END) && (_key >= ImGuiKey_NamedKey_BEGIN);

    return (!has_mods && has_keys);
}

b8 CommandContext::bindCommand(InputEventCode event_cmd) {
    if (containsCommand(event_cmd)) {
        EZ_LOG_ERROR("Cannot bind command [%d]. It already exists in the [%s] CommandContext", event_cmd, owner.c_str());
        return false;
    }

    if (isCommandKeyChord(event_cmd)) {
        commands_chord[event_cmd] = COMMAND_IDLE;
        return true;
    } else if (isCommandKeySingle(event_cmd)) {
        commands_single[event_cmd] = COMMAND_IDLE;
        return true;
    }

    EZ_LOG_ERROR("Cannot bind command [%d] to [%s] CommandContext. It is neither a key chord nor a single key.", event_cmd, owner.c_str());
    return false;
}

b8 CommandContext::unbindCommand(InputEventCode event_cmd) {
    if (!containsCommand(event_cmd)) {
        EZ_LOG_ERROR("Cannot unbind command [%d]. It does not exists in the [%s] CommandContext", event_cmd, owner.c_str());
        return false;
    }

    if (isCommandKeyChord(event_cmd)) {
        commands_chord.erase(event_cmd);
        return true;
    } else if (isCommandKeySingle(event_cmd)) {
        commands_single.erase(event_cmd);
        return true;
    }

    EZ_LOG_ERROR("Cannot unbind command [%d] to [%s] CommandContext. It is neither a key chord nor a single key.", event_cmd, owner.c_str());
    return false;
}

//-------------------------------------------------------------------------------------------
//                              CommandRegistry Class
//-------------------------------------------------------------------------------------------

std::vector<std::string_view> CommandRegistry::getAllCommandOwners() {
    // TODO(Argosta): optimize this when we figure out what data structure we are using
    std::vector<std::string_view> owners;
    owners.reserve(this->command_contexts.size());

    for (const auto& pair : this->command_contexts) {
        owners.push_back(pair.first);
    }

    return owners;
}

b8 CommandRegistry::containsCommandContext(const std::string& owner_uid) {
    const auto it = this->command_contexts.find(owner_uid);
    if (it == this->command_contexts.end()) {
        return false;
    }
    return true;
}

std::shared_ptr<CommandContext> CommandRegistry::registerCommandContext(std::string owner_uid) {
    if (containsCommandContext(owner_uid)) {
        EZ_LOG_ERROR("Cannot register [%s] CommandContext as it already exists.", owner_uid.c_str());
        return this->command_contexts.at(owner_uid);
    }

    this->command_contexts[owner_uid] = std::make_shared<CommandContext>(owner_uid);
    return this->command_contexts[owner_uid];
}

b8 CommandRegistry::unregisterCommandContext(std::shared_ptr<CommandContext> cmd_manager) {
    const auto& command_owner = cmd_manager->getOwner();

    if (!containsCommandContext(command_owner)) {
        EZ_LOG_ERROR("Cannon un-register [%s] CommandContext as it does not exists.", command_owner.c_str());
        return false;
    }

    this->command_contexts.erase(command_owner);
    return true;
}

void CommandRegistry::processInputCommands() {
    ZoneScopedN("Processing Commands");

    // NOTE(all): we separate key chord processing logic from single keys for reasons bellow:
    //                  1. key chords have only two states: IDLE and COMMITTED
    //                  2. if a key chord is pressed down, we mark it as COMMITTED and don't care about key release
    //                  3. if a key chord is pressed, no other single keys or key chords are processed. only one key chord per frame is allowed
    // But for single keys, we can track its states when user:
    //      - press down a key on one frame(IDLE --> INITIATED)
    //      - still holding down the key for many frames (INITIATED)
    //      - user releases the key on one frame (INITIATED --> COMMITTED)
    //      - next frame (COMMITTED --> IDLE)
    // This behaviour for single keys are useful when we want to trigger an event when a key is pressed during multiple frames
    const auto& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    s32 _mods_down = ImGuiMod_None;

    // Ctrl (non-macOS), Cmd (macOS)
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl)) {
        _mods_down |= ImGuiMod_Ctrl;
    }
    // Shift
    if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
        _mods_down |= ImGuiMod_Shift;
    }
    // Option/Menu
    if (ImGui::IsKeyDown(ImGuiMod_Alt)) {
        _mods_down |= ImGuiMod_Alt;
    }
    // Windows/Super (non-macOS), Ctrl (macOS)
    if (ImGui::IsKeyDown(ImGuiMod_Super)) {
        _mods_down |= ImGuiMod_Super;
    }

    static const ImGuiKey keyboard_single_keys[] = {
        ImGuiKey_Tab, ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_PageUp, ImGuiKey_PageDown,
        ImGuiKey_Home, ImGuiKey_End, ImGuiKey_Insert, ImGuiKey_Delete, ImGuiKey_Backspace, ImGuiKey_Space, ImGuiKey_Enter, ImGuiKey_Escape, ImGuiKey_Menu,
        ImGuiKey_0, ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9, ImGuiKey_A, ImGuiKey_B,
        ImGuiKey_C, ImGuiKey_D, ImGuiKey_E, ImGuiKey_F, ImGuiKey_G, ImGuiKey_H, ImGuiKey_I, ImGuiKey_J, ImGuiKey_K, ImGuiKey_L, ImGuiKey_M, ImGuiKey_N,
        ImGuiKey_O, ImGuiKey_P, ImGuiKey_Q, ImGuiKey_R, ImGuiKey_S, ImGuiKey_T, ImGuiKey_U, ImGuiKey_V, ImGuiKey_W, ImGuiKey_X, ImGuiKey_Y, ImGuiKey_Z,
        ImGuiKey_F1, ImGuiKey_F2, ImGuiKey_F3, ImGuiKey_F4, ImGuiKey_F5, ImGuiKey_F6,
        ImGuiKey_F7, ImGuiKey_F8, ImGuiKey_F9, ImGuiKey_F10, ImGuiKey_F11, ImGuiKey_F12,
        ImGuiKey_F13, ImGuiKey_F14, ImGuiKey_F15, ImGuiKey_F16, ImGuiKey_F17, ImGuiKey_F18,
        ImGuiKey_F19, ImGuiKey_F20, ImGuiKey_F21, ImGuiKey_F22, ImGuiKey_F23, ImGuiKey_F24,
        ImGuiKey_Apostrophe, ImGuiKey_Comma, ImGuiKey_Minus, ImGuiKey_Period, ImGuiKey_Slash, ImGuiKey_Semicolon, ImGuiKey_Equal, ImGuiKey_LeftBracket,
        ImGuiKey_Backslash, ImGuiKey_RightBracket, ImGuiKey_GraveAccent, ImGuiKey_ScrollLock, ImGuiKey_NumLock, ImGuiKey_PrintScreen, ImGuiKey_Pause,
        ImGuiKey_KeypadDecimal, ImGuiKey_KeypadDivide, ImGuiKey_KeypadMultiply, ImGuiKey_KeypadSubtract, ImGuiKey_KeypadAdd, ImGuiKey_KeypadEnter, ImGuiKey_KeypadEqual,
        ImGuiKey_Keypad0, ImGuiKey_Keypad1, ImGuiKey_Keypad2, ImGuiKey_Keypad3, ImGuiKey_Keypad4,
        ImGuiKey_Keypad5, ImGuiKey_Keypad6, ImGuiKey_Keypad7, ImGuiKey_Keypad8, ImGuiKey_Keypad9
    };

    const u32 _num_keys = sizeof(keyboard_single_keys) / sizeof(ImGuiKey);

    u32 num_single_keys_down = 0;
    for (u32 i = 0; i < _num_keys; i++) {
        const ImGuiKey _key = keyboard_single_keys[i];
        if (ImGui::IsKeyDown(_key)) {
            num_single_keys_down++;
        }
    }

    // no mod key is pressed
    if (_mods_down == ImGuiMod_None) {
        for (const auto& cmd_ctx_pair : command_contexts) {
            const auto& cmd_ctx = cmd_ctx_pair.second;
            // now process single keys becuase we don't want register a single key twice if it is included in a key chord
            for (auto& cmd_input : cmd_ctx->commands_single) {
                const auto& event_code = cmd_input.first;
                auto& event_state = cmd_input.second;

                // state transition for Single Keys: idle --> initiated --> committed --> idle
                if (event_state == InputCommandState::COMMAND_IDLE && ImGui::IsKeyDown(event_code)) {
                    event_state = InputCommandState::COMMAND_INITIATED;
                } else if (event_state == InputCommandState::COMMAND_INITIATED && !ImGui::IsKeyDown(event_code)) {
                    event_state = InputCommandState::COMMAND_COMMITTED;
                } else if (event_state == InputCommandState::COMMAND_COMMITTED) {
                    event_state = InputCommandState::COMMAND_IDLE;
                }
            }
        }
    }
    // if a combination of MOD keys and 1 single key is pressed (e.g. Ctrl+Shift+S)
    else if (num_single_keys_down == 1) {
        for (const auto& cmd_ctx_pair : command_contexts) {
            const auto& cmd_ctx = cmd_ctx_pair.second;

            // process chord keys first
            for (auto& cmd_input : cmd_ctx->commands_chord) {
                const auto& event_code = cmd_input.first;
                auto& event_state = cmd_input.second;

                // state transition for Key Chords: idle --> committed --> idle
                if (event_state == InputCommandState::COMMAND_IDLE && ImGui::IsKeyChordPressed(event_code)) {
                    event_state = InputCommandState::COMMAND_COMMITTED;
                } else if (event_state != InputCommandState::COMMAND_IDLE) {
                    event_state = InputCommandState::COMMAND_IDLE;
                }
            }
        }
    }
}

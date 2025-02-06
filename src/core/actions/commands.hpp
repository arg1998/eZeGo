#pragma once

#include "core/definitions.hpp"

#include <imgui.h>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

using InputEventCode = ImGuiKey;

enum InputCommandState {
    COMMAND_INITIATED,
    COMMAND_COMMITTED,
    COMMAND_IDLE
};

class CommandContext {
    friend class CommandRegistry;

   private:
    std::string owner;
    std::unordered_map<InputEventCode, InputCommandState> commands_single;
    std::unordered_map<InputEventCode, InputCommandState> commands_chord;

   public:
    CommandContext(const std::string owner_uid);
    std::string getOwner();
    b8 containsCommand(const InputEventCode event_cmd);
    b8 isCommandInitated(const InputEventCode event_cmd);
    b8 isCommandCommitted(const InputEventCode event_cmd);
    b8 isCommandIdle(const InputEventCode event_cmd);
    b8 isCommandKeyChord(const InputEventCode event_cmd);
    b8 isCommandKeySingle(const InputEventCode event_cmd);

    b8 bindCommand(InputEventCode event_cmd);
    b8 unbindCommand(InputEventCode event_cmd);
};

class CommandRegistry {
    std::unordered_map<std::string, std::shared_ptr<CommandContext>> command_contexts;
    
   public:
    std::vector<std::string_view> getAllCommandOwners();
    std::shared_ptr<CommandContext> registerCommandContext(std::string owner_uid);
    b8 containsCommandContext(const std::string& owner_uid);
    b8 unregisterCommandContext(std::shared_ptr<CommandContext> cmd_ctx);
    void processInputCommands();
};

b8 initCommandSystem();
void shutdownCommandSystem();
std::shared_ptr<CommandRegistry> getCommandRegistry();
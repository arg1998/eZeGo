#include "actions.hpp"

#include "core/logger/logger.hpp"
#include "core/containers/ring_stack.hpp"

#define DEFAULT_ACTION_HISTORY_LENGHT 64

static b8 initialized = false;
static ActionHandler action_handlers[ACTION_TYPE_COUNT];

static s32 head = -1, tail = -1, pos = -1;
static RingStack<ActionRecord> action_history_undo(DEFAULT_ACTION_HISTORY_LENGHT);
static RingStack<ActionRecord> action_history_redo(DEFAULT_ACTION_HISTORY_LENGHT);

b8 initActionManager() {
    if (initialized) {
        EZ_LOG_ERROR("Cannot initialize ActionManager as it is already initialized");
        return false;
    }
    initialized = true;
    return true;
}

void shutdownActionManager() {
    initialized = false;
}

void registerActionHandler(ActionType type, ActionFunc execute, ActionFunc undo) {
    ActionHandler& handler = action_handlers[type];
    if (handler.execute != nullptr || handler.undo != nullptr) {
        EZ_LOG_ERROR("Avoid re-registering Action Handlers for ActionType [%d]. Overriding Action Handlers for execute() and undo() functions.", type);
    }

    handler.execute = execute;
    handler.undo = undo;
}

void addAction(ActionRecord* action) {
    action_history_undo.push(*action);
    // invlaidate redo stack since new actions happened
    action_history_redo.clear();
    EZ_LOG_INFO("ADDING ACTION");
}

void undoAction() {
    if (action_history_undo.empty()) return;

    ActionRecord action = action_history_undo.pop();
    action_handlers[action.type].undo(&action);
    action_history_redo.push(action);
}

void redoAction() {
    if (action_history_redo.empty()) return;

    ActionRecord action = action_history_redo.pop();
    action_handlers[action.type].execute(&action);
    action_history_undo.push(action);
}

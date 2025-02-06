#pragma once

#include "core/definitions.hpp"
#include "action_records.hpp"

#include <vector>

b8 initActionManager();
void shutdownActionManager();
void registerActionHandler(ActionType type, ActionFunc execute, ActionFunc undo);
void addAction(ActionRecord *action);
void undoAction();
void redoAction();

struct ActionHandler {
    ActionFunc execute;
    ActionFunc undo;
};

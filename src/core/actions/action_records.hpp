#pragma once

#include "core/definitions.hpp"

//                                       NOTE 
// For each Undoable/Redoable action, you must create an Atomic Action State (AAS)
// where the previous and the current changes are stored in a dedicated memory space.
// Only one peice of information (state) is changed withing one AAS. Don't store heacy data inside.
// If you just rely on pointers to data, then that data might not exist by the time you 
// undo/redo it! So copy the data! 
// One example is `IntValueChangeActionInfo` struct. Add your own AAS, and include it the `union`
// section of the `ActionRecord` struct. Also, add the appropriate action type to the `ActionType`
// enum. 
//
// To actually be able to undo/redo, you must implement two unique functions that chnages the atomic state
// based on the `ActionFunc` function signature! Good luck!




struct IntValueChangeActionInfo {
    s32 *target; 
    s32 prev_value; 
    s32 current_value;
};


enum ActionType : u32 {
    ActionType_ChangeIntValue,
    ACTION_TYPE_COUNT
};


struct ActionRecord {
    ActionType type; 
    union {
        IntValueChangeActionInfo intChangeValue;
    } payload;
};


typedef void (*ActionFunc)(const ActionRecord*);
#pragma once

#include "core/definitions.hpp"
#include "core/containers/ring_stack.hpp"

#include <function2/function2.hpp>

constexpr u32 FUNCTION_STATIC_INTERNAL_SIZE = 64;

// TODO(Argosta): see if you can set the IsCopyable parameter to false! (it's the seond argument)
using ActionCallback64B = fu2::function_base<true, true, fu2::capacity_fixed<FUNCTION_STATIC_INTERNAL_SIZE>, false, false, void()>;

struct Action;
template <typename T>
class TrackableState;
class StateHistoryContext;

struct Action {
    ActionCallback64B undo;
    ActionCallback64B redo;
};

template <typename T>
class TrackableState {
   private:
    T value;

   public:
    TrackableState() = default;
    TrackableState(T initial_value) : value(initial_value) {}

    EZ_NO_DISCARD Action setState(const T& new_value) {
        T prev_value = value;
        value = new_value;

        const auto undoClosure = [prev_value, this]() { this->value = prev_value; };

        const auto redoClosure = [new_value, this]() { this->value = new_value; };

#ifdef EZ_DEBUG_BUILD
        if (sizeof(undoClosure) > FUNCTION_STATIC_INTERNAL_SIZE) {
            EZ_LOG_WARN("A genreated Undo()/Redo() closure has exceeded %d bytes. Potential Heap Allocation from std::function", FUNCTION_STATIC_INTERNAL_SIZE);
        }
#endif

        Action outAction{std::move(undoClosure), std::move(redoClosure)};
        return outAction;
    }


    void setState(const T& new_value, const StateHistoryContext *stateHistoryCtx) {
        if(new_value == value) return;

        T prev_value = value;
        value = new_value;

        const auto undoClosure = [prev_value, this]() { this->value = prev_value; };

        const auto redoClosure = [new_value, this]() { this->value = new_value; };

#ifdef EZ_DEBUG_BUILD
        if (sizeof(undoClosure) > FUNCTION_STATIC_INTERNAL_SIZE) {
            EZ_LOG_WARN("A genreated Undo()/Redo() closure has exceeded %d bytes. Potential Heap Allocation from std::function", FUNCTION_STATIC_INTERNAL_SIZE);
        }
#endif

        Action outAction{std::move(undoClosure), std::move(redoClosure)};
        stateHistoryCtx->add(outAction);
    }

    inline const T& getState() const { return value; }
};

class StateHistoryContext {
   private:
    Action* _actions{nullptr};
    u32 _capacity;
    s32 _tail;
    s32 _head;
    s32 _cursor;
    b8 _can_undo;
    b8 _can_redo;
    b8 _is_packed;

   public:
    StateHistoryContext(u32 capacity = 64) {
        resizeUnsafe(capacity);
    }

    void add(const Action& value) {
        s32 temp_cursor = (_cursor + 1) % _capacity;
        if (temp_cursor != _head) {
            // this block means some actions have been UNDONE previousley
            // and this new action will invalidate all of the actions that have been UNDONE
            // reset the head pointer to override invalidated actions
            _head = temp_cursor;
        }

        _actions[_head] = value;
        _cursor = _head;
        // if the buffer is packed (full), then override the oldest element
        if (isPacked()) {
            _tail = (_tail + 1) % _capacity;
        }
        _head = (_head + 1) % _capacity;
        _is_packed = (_head == _tail);
        // when a new action is performed, we can undo that immediately
        _can_undo = true;
        // when a new action is performed, we discard all actions after it (if any)
        // this means we cannot perform redo until an undo happens again
        _can_redo = false;
    }

    b8 undoAction() {
        if (!canUndo()) {
            return false;
        }
        _actions[_cursor].undo();
        // if cursor was at the oldest action, then we cannot perform another undo
        if (_cursor == _tail) {
            _can_undo = false;
        }

        _cursor = (_cursor + _capacity - 1) % _capacity;
        // when we undo an action, we can always redo that immediately
        // (unless new action is performed which cueases redo space to override)
        _can_redo = true;
        return true;
    }

    b8 redoAction() {
        if (!canRedo()) {
            return false;
        }

        _cursor = (_cursor + 1) % _capacity;
        _actions[_cursor].redo();

        // we can perform another redo if next cursor is not the same as head pointer
        _can_redo = ((_cursor + 1) % _capacity) != _head;
        _can_undo = true;
        return true;
    }

    void clear() {
        _is_packed = false;
        _can_undo = false;
        _can_redo = false;
        _cursor = -1;  // points to the latest action performed
        _tail = 0;     // points to the oldest element
        _head = 0;     // points to the next free slot
    }

    b8 canUndo() {
        return _can_undo;
    }

    b8 canRedo() {
        return _can_redo;
    }

    b8 empty() {
        return (!_is_packed && ((_head == _tail)));
    }

    b8 isPacked() {
        return _is_packed;
    }

    void resizeUnsafe(u32 new_capacity) {
        if (_actions != nullptr) {
            delete[] _actions;
        }
        clear();
        _capacity = new_capacity;
        _actions = new Action[new_capacity];
    }
};
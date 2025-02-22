#pragma once

#include "core/definitions.hpp"
#include "core/logger/logger.hpp"

template <typename T>
class RingStack {
   private:
    T* _buffer;
    s32 _tail;
    s32 _head;
    b8 _full;
    u32 _capacity;

   public:
    RingStack(u32 capacity) {
        this->_capacity = capacity;
        this->_full = false;
        this->_tail = 0;  // points to the oldest element
        this->_head = 0;  // points to the next free slot
        _buffer = new T[capacity];
    }

    void push(const T& value) {
        _buffer[_head] = value;
        if (full()) {
            _tail = (_tail + 1) % _capacity;
        }
        _head = (_head + 1) % _capacity;
        _full = (_head == _tail);
    }

    T& pop() {
        if (empty()) {
            EZ_LOG_FATAL("Cannot pop an EMPTY RingStack!");
        }
        _head = (_head + _capacity - 1) % _capacity;
        T& _out = _buffer[_head];
        _full = false;
        return _out;
    }

    const T& peek() const {
        if (empty()) {
            EZ_LOG_FATAL("Cannot peek from an EMPTY RingStack!");
        }

        const u32 _index = (_head + _capacity - 1) % _capacity;
        return _buffer[_index];
    }

    void clear() {
        _head = _tail;
        _full = false;
    }

    b8 empty() {
        return (!_full && (_head == _tail));
    }

    b8 full() {
        return _full;
    }

    u32 size() {
        if (_full) {
            return _capacity;
        }

        if (_head >= _tail) {
            return _head - _tail;
        }
        return _capacity - _tail + _head;
    }
};
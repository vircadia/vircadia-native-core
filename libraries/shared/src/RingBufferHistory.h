//
//  RingBufferHistory.h
//  libraries/shared/src
//
//  Created by Yixin Wang on 7/9/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RingBufferHistory_h
#define hifi_RingBufferHistory_h

#include <stdlib.h>
#include <iterator>

template <typename T>
class RingBufferHistory {

public:
    
    RingBufferHistory(int capacity = 10)
        : _size(capacity + 1),
        _capacity(capacity),
        _newestEntryAtIndex(0),
        _numEntries(0)
    {
        _buffer = new T[_size];
    }

    RingBufferHistory(const RingBufferHistory& other)
        : _size(other._size),
        _capacity(other._capacity),
        _newestEntryAtIndex(other._newestEntryAtIndex),
        _numEntries(other._numEntries)
    {
        _buffer = new T[_size];
        memcpy(_buffer, other._buffer, _size*sizeof(T));
    }

    RingBufferHistory& operator= (const RingBufferHistory& rhs) {
        _size = rhs._size;
        _capacity = rhs._capacity;
        _newestEntryAtIndex = rhs._newestEntryAtIndex;
        _numEntries = rhs._numEntries;
        delete[] _buffer;
        _buffer = new T[_size];
        memcpy(_buffer, rhs._buffer, _size*sizeof(T));
        return *this;
    }

    ~RingBufferHistory() {
        delete[] _buffer;
    }

    void clear() {
        _numEntries = 0;
    }

    void insert(const T& entry) {
        // increment newest entry index cyclically
        _newestEntryAtIndex = (_newestEntryAtIndex == _size - 1) ? 0 : _newestEntryAtIndex + 1;

        // insert new entry
        _buffer[_newestEntryAtIndex] = entry;
        if (_numEntries < _capacity) {
            _numEntries++;
        }
    }

    // 0 retrieves the most recent entry, _numEntries - 1 retrieves the oldest.
    // returns NULL if entryAge not within [0, _numEntries-1]
    const T* get(int entryAge) const {
        if (!(entryAge >= 0 && entryAge < _numEntries)) {
            return NULL;
        }
        int entryAt = _newestEntryAtIndex - entryAge;
        if (entryAt < 0) {
            entryAt += _size;
        }
        return &_buffer[entryAt];
    }

    T* get(int entryAge) {
        return const_cast<T*>((static_cast<const RingBufferHistory*>(this))->get(entryAge));
    }

    const T* getNewestEntry() const {
        return &_buffer[_newestEntryAtIndex];
    }

    T* getNewestEntry() {
        return &_buffer[_newestEntryAtIndex];
    }

    int getCapacity() const { return _capacity; }
    int getNumEntries() const { return _numEntries; }

private:
    T* _buffer;
    int _size;
    int _capacity;
    int _newestEntryAtIndex;
    int _numEntries;

public:
    class Iterator : public std::iterator < std::forward_iterator_tag, T > {
    public:
        Iterator(T* buffer, int size, T* at) : _buffer(buffer), _bufferEnd(buffer+size), _at(at) {}

        bool operator==(const Iterator& rhs) { return _at == rhs._at; }
        bool operator!=(const Iterator& rhs) { return _at != rhs._at; }
        T& operator*() { return *_at; }
        T* operator->() { return _at; }

        Iterator& operator++() {
            _at = (_at == _buffer) ? _bufferEnd - 1 : _at - 1;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp(*this);
            ++(*this);
            return tmp;
        }

    private:
        T* const _buffer;
        T* const _bufferEnd;
        T* _at;
    };

    Iterator begin() { return Iterator(_buffer, _size, &_buffer[_newestEntryAtIndex]); }

    Iterator end() {
        int endAtIndex = _newestEntryAtIndex - _numEntries;
        if (endAtIndex < 0) {
            endAtIndex += _size;
        }
        return Iterator(_buffer, _size, &_buffer[endAtIndex]);
    }
};

#endif // hifi_RingBufferHistory_h

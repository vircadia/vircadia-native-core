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

#include <qvector.h>

template <typename T>
class RingBufferHistory {

public:
    
    RingBufferHistory(int capacity = 10)
        : _size(capacity + 1),
        _capacity(capacity),
        _newestEntryAtIndex(0),
        _numEntries(0),
        _buffer(capacity + 1)
    {
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
        return _numEntries == 0 ? NULL : &_buffer[_newestEntryAtIndex];
    }

    T* getNewestEntry() {
        return _numEntries == 0 ? NULL : &_buffer[_newestEntryAtIndex];
    }

    int getCapacity() const { return _capacity; }
    int getNumEntries() const { return _numEntries; }
    bool isFilled() const { return _numEntries == _capacity; }

private:
    int _size;
    int _capacity;
    int _newestEntryAtIndex;
    int _numEntries;
    QVector<T> _buffer;

public:
    class Iterator : public std::iterator < std::forward_iterator_tag, T > {
    public:
        Iterator(T* bufferFirst, T* bufferLast, T* at) : _bufferFirst(bufferFirst), _bufferLast(bufferLast), _at(at) {}

        bool operator==(const Iterator& rhs) { return _at == rhs._at; }
        bool operator!=(const Iterator& rhs) { return _at != rhs._at; }
        T& operator*() { return *_at; }
        T* operator->() { return _at; }

        Iterator& operator++() {
            _at = (_at == _bufferFirst) ? _bufferLast : _at - 1;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp(*this);
            ++(*this);
            return tmp;
        }

    private:
        T* const _bufferFirst;
        T* const _bufferLast;
        T* _at;
    };

    Iterator begin() { return Iterator(&_buffer.first(), &_buffer.last(), &_buffer[_newestEntryAtIndex]); }

    Iterator end() {
        int endAtIndex = _newestEntryAtIndex - _numEntries;
        if (endAtIndex < 0) {
            endAtIndex += _size;
        }
        return Iterator(&_buffer.first(), &_buffer.last(), &_buffer[endAtIndex]);
    }
};

#endif // hifi_RingBufferHistory_h

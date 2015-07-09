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

    void setCapacity(int capacity) {
        _size = capacity + 1;
        _capacity = capacity;
        _newestEntryAtIndex = 0;
        _numEntries = 0;
        _buffer.resize(_size);
    }

    void insert(const T& entry) {
        // increment newest entry index cyclically
        _newestEntryAtIndex = (_newestEntryAtIndex + 1) % _size;

        // insert new entry
        _buffer[_newestEntryAtIndex] = entry;
        if (_numEntries < _capacity) {
            _numEntries++;
        }
    }

    // std::unique_ptr need to be passed as an rvalue ref and moved into the vector
    void insert(T&& entry) {
        // increment newest entry index cyclically
        _newestEntryAtIndex = (_newestEntryAtIndex + 1) % _size;

        // insert new entry
        _buffer[_newestEntryAtIndex] = std::move(entry);
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
    std::vector<T> _buffer;

public:
    class Iterator : public std::iterator < std::random_access_iterator_tag, T > {
    public:
        Iterator(T* bufferFirst, T* bufferLast, T* newestAt, T* at)
            : _bufferFirst(bufferFirst),
            _bufferLast(bufferLast),
            _bufferLength(bufferLast - bufferFirst + 1),
            _newestAt(newestAt),
            _at(at) {}

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

        Iterator& operator--() {
            _at = (_at == _bufferLast) ? _bufferFirst : _at + 1;
            return *this;
        }

        Iterator operator--(int) {
            Iterator tmp(*this);
            --(*this);
            return tmp;
        }

        Iterator operator+(int add) {
            Iterator sum(*this);
            sum._at = atShiftedBy(add);
            return sum;
        }

        Iterator operator-(int sub) {
            Iterator sum(*this);
            sum._at = atShiftedBy(-sub);
            return sum;
        }

        Iterator& operator+=(int add) {
            _at = atShiftedBy(add);
            return *this;
        }

        Iterator& operator-=(int sub) {
            _at = atShiftedBy(-sub);
            return *this;
        }

        T& operator[](int i) {
            return *(atShiftedBy(i));
        }

        bool operator<(const Iterator& rhs) {
            return age() < rhs.age();
        }

        bool operator>(const Iterator& rhs) {
            return age() > rhs.age();
        }

        bool operator<=(const Iterator& rhs) {
            return age() < rhs.age();
        }

        bool operator>=(const Iterator& rhs) {
            return age() >= rhs.age();
        }

        int operator-(const Iterator& rhs) {
            return age() - rhs.age();
        }

    private:
        T* atShiftedBy(int i) { // shifts i places towards _bufferFirst (towards older entries)
            i = (_at - _bufferFirst - i) % _bufferLength;
            if (i < 0) {
                i += _bufferLength;
            }
            return _bufferFirst + i;
        }

        int age() {
            int age = _newestAt - _at;
            if (age < 0) {
                age += _bufferLength;
            }
            return age;
        }

        T* _bufferFirst;
        T* _bufferLast;
        int _bufferLength;
        T* _newestAt;
        T* _at;
    };

    Iterator begin() { return Iterator(&_buffer.front(), &_buffer.back(), &_buffer[_newestEntryAtIndex], &_buffer[_newestEntryAtIndex]); }

    Iterator end() {
        int endAtIndex = _newestEntryAtIndex - _numEntries;
        if (endAtIndex < 0) {
            endAtIndex += _size;
        }
        return Iterator(&_buffer.front(), &_buffer.back(), &_buffer[_newestEntryAtIndex], &_buffer[endAtIndex]);
    }
};

#endif // hifi_RingBufferHistory_h

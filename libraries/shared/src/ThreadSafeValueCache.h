//
//  ThreadSafeValueCache.h
//  interface/src/avatar
//
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ThreadSafeValueCache_h
#define hifi_ThreadSafeValueCache_h

#include <mutex>

template <typename T>
class ThreadSafeValueCache {
public:
    ThreadSafeValueCache(const T& v) : _value { v }, _pending { v }, _hasPending { false } {}

    template <typename F>
    void update(const F& callback) {
        std::lock_guard<std::mutex> guard(_mutex);
        _hasPending = callback((T&)_value, _hasPending, (const T&)_pending);
    }

    T get() const {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_hasPending) {
            return _pending;
        } else {
            return _value;
        }
    }

    void set(const T& v) {
        std::lock_guard<std::mutex> guard(_mutex);
        _hasPending = true;
        _pending = v;
    }

private:
    mutable std::mutex _mutex;
    T _value;
    T _pending;
    bool _hasPending;

    // no copies
    ThreadSafeValueCache(const ThreadSafeValueCache&) = delete;
    ThreadSafeValueCache& operator=(const ThreadSafeValueCache&) = delete;
};

#endif // #define hifi_ThreadSafeValueCache_h

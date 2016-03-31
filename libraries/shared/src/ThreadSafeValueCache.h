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

// Helper class for for sharing a value type between threads.
// It allows many threads to get or set a value atomically.
// This provides cache semantics, any get will return the last set value.
//
// For example: This can be used to copy values between C++ code running on the application thread
// and JavaScript which is running on a different thread.

template <typename T>
class ThreadSafeValueCache {
public:
    ThreadSafeValueCache() {}
    ThreadSafeValueCache(const T& v) : _value { v } {}

    // returns atomic copy of the cached value.
    T get() const {
        std::lock_guard<std::mutex> guard(_mutex);
        return _value;
    }

    // will reflect copy of value into the cache.
    void set(const T& v) {
        std::lock_guard<std::mutex> guard(_mutex);
        _value = v;
    }

private:
    mutable std::mutex _mutex;
    T _value;

    // no copies
    ThreadSafeValueCache(const ThreadSafeValueCache&) = delete;
    ThreadSafeValueCache& operator=(const ThreadSafeValueCache&) = delete;
};

#endif // #define hifi_ThreadSafeValueCache_h

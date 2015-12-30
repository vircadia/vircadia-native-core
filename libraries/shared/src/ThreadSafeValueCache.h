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
// It also provides a mechanism for the owner of the cached value to reconcile
// the cached value with it's own internal values, via the merge method.
//
// For example: This can be used to copy values between C++ code running on the application thread
// and JavaScript which is running on a different thread.

template <typename T>
class ThreadSafeValueCache {
public:
    ThreadSafeValueCache(const T& v) : _value { v }, _pending { v }, _hasPending { false } {}

    // The callback function should have the following prototype.
    // T func(const T& value, bool hasPending, const T& pendingValue);
    // It will be called synchronously on the current thread possibly blocking it for a short time.
    // it gives thread-safe access to the internal cache value, as well as the pending cache value
    // that was set via the last set call.  This gives the cache's owner the opportunity to update
    // the cached value by resolving it with it's own internal state.  The owner should then return
    // the resolved value which will be atomically reflected into the cache.
    template <typename F>
    void merge(F func) {
        std::lock_guard<std::mutex> guard(_mutex);
        _value = func((const T&)_value, _hasPending, (const T&)_pending);
        _hasPending = false;
    }

    // returns atomic copy of the cached value.
    T get() const {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_hasPending) {
            return _pending;
        } else {
            return _value;
        }
    }

    // will reflect copy of value into the cache.
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

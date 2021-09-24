//
//  SharedMutex.h
//  shared/src
//
//  Created by Heather Anderson on 9/23/2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SharedMutex_h
#define hifi_SharedMutex_h

#include <mutex>

#if __cplusplus >= 201402L /* C++14 */

using shared_mutex = std::shared_mutex;
template <class _Mutex> using shared_lock = std::shared_lock<_Mutex>;

#else

// Reference implementation taken from http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2406.html#shared_mutex_imp

class shared_mutex {
    using mutex = std::mutex;
    using cond_var = std::condition_variable;

    mutex mut_;
    cond_var gate1_;
    cond_var gate2_;
    unsigned state_;

    static const unsigned write_entered_ = 1U << (sizeof(unsigned) * CHAR_BIT - 1);
    static const unsigned n_readers_ = ~write_entered_;

public:
    inline shared_mutex() : state_(0) {}

    // Exclusive ownership

    void lock();
    bool try_lock();
    void unlock();

    // Shared ownership

    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();
};

// CLASS TEMPLATE shared_lock (copied from unique_lock)
template <class _Mutex>
class shared_lock {  // whizzy class with destructor that unlocks mutex
public:
    using mutex_type = _Mutex;

    // CONSTRUCT, ASSIGN, AND DESTROY
    shared_lock() noexcept : _Pmtx(nullptr), _Owns(false) {}

    explicit shared_lock(_Mutex& _Mtx) : _Pmtx(&_Mtx), _Owns(false) {  // construct and lock
        _Pmtx->lock_shared();
        _Owns = true;
    }

    shared_lock(_Mutex& _Mtx, std::adopt_lock_t) : _Pmtx(&_Mtx), _Owns(true) {}  // construct and assume already locked

    shared_lock(_Mutex& _Mtx, std::defer_lock_t) noexcept : _Pmtx(&_Mtx), _Owns(false) {}  // construct but don't lock

    shared_lock(_Mutex& _Mtx, std::try_to_lock_t) :
        _Pmtx(&_Mtx), _Owns(_Pmtx->try_lock_shared()) {}  // construct and try to lock

    shared_lock(shared_lock&& _Other) noexcept : _Pmtx(_Other._Pmtx), _Owns(_Other._Owns) {
        _Other._Pmtx = nullptr;
        _Other._Owns = false;
    }

    shared_lock& operator=(shared_lock&& _Other) {
        if (this != &_Other) {
            if (_Owns) {
                _Pmtx->unlock_shared();
            }

            _Pmtx = _Other._Pmtx;
            _Owns = _Other._Owns;
            _Other._Pmtx = nullptr;
            _Other._Owns = false;
        }
        return *this;
    }

    ~shared_lock() noexcept {
        if (_Owns) {
            _Pmtx->unlock_shared();
        }
    }

    shared_lock(const shared_lock&) = delete;
    shared_lock& operator=(const shared_lock&) = delete;

    void lock() {  // lock the mutex
        Validate();
        _Pmtx->lock_shared();
        _Owns = true;
    }

    bool try_lock() {
        Validate();
        _Owns = _Pmtx->try_lock_shared();
        return _Owns;
    }

    void unlock() {
        if (!_Pmtx || !_Owns) {
            throw std::system_error(std::errc::operation_not_permitted);
        }

        _Pmtx->unlock_shared();
        _Owns = false;
    }

    void swap(shared_lock& _Other) noexcept {
        std::swap(_Pmtx, _Other._Pmtx);
        std::swap(_Owns, _Other._Owns);
    }

    _Mutex* release() noexcept {
        _Mutex* _Res = _Pmtx;
        _Pmtx = nullptr;
        _Owns = false;
        return _Res;
    }

    bool owns_lock() const noexcept { return _Owns; }

    explicit operator bool() const noexcept { return _Owns; }

    _Mutex* mutex() const noexcept { return _Pmtx; }

private:
    _Mutex* _Pmtx;
    bool _Owns;

    void Validate() const {  // check if the mutex can be locked
        if (!_Pmtx) {
            throw std::system_error(std::errc::operation_not_permitted);
        }

        if (_Owns) {
            throw std::system_error(std::errc::resource_deadlock_would_occur);
        }
    }
};

// FUNCTION TEMPLATE swap FOR shared_lock
template <class _Mutex>
void swap(shared_lock<_Mutex>& _Left, shared_lock<_Mutex>& _Right) noexcept {
    _Left.swap(_Right);
}

#endif /* __cplusplus >= 201703L */

#endif /* hifi_SharedMutex_h */
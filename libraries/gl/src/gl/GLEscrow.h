//
//  Created by Bradley Austin Davis on 2015/08/06.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GLEscrow_h
#define hifi_GLEscrow_h

#include <utility>
#include <algorithm>
#include <deque>
#include <forward_list>
#include <functional>
#include <mutex>

#include <SharedUtil.h>
#include <NumericalConstants.h>

#include "Config.h"

// The GLEscrow class provides a simple mechanism for producer GL contexts to provide
// content to a consumer where the consumer is assumed to be connected to a display and
// therefore must never be blocked.
//
// So we need to accomplish a few things.
//
// First the producer context needs to be able to supply content to the primary thread
// in such a way that the consumer only gets it when it's actually valid for reading
// (meaning that the async writing operations have been completed)
//
// Second, the client thread should be able to release the resource when it's finished
// using it (but again the reading of the resource is likely asyncronous)
//
// Finally, blocking operations need to be minimal, and any potentially blocking operations
// that can't be avoided need to be pushed to the submission context to avoid impacting
// the framerate of the consumer
//
// This class acts as a kind of border guard and holding pen between the two contexts
// to hold resources which the CPU is no longer using, but which might still be
// in use by the GPU.  Fence sync objects are used to moderate the actual release of
// resources in either direction.
template <
    typename T
    //,
    //// Only accept numeric types
    //typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type
>
class GLEscrow {
public:
    static const uint64_t MAX_UNSIGNALED_TIME = USECS_PER_SECOND / 2;
    
    const T& invalid() const {
        static const T INVALID_RESULT;
        return INVALID_RESULT;
    }

    struct Item {
        const T _value;
        GLsync _sync;
        const uint64_t _created;

        Item(T value, GLsync sync) :
            _value(value), _sync(sync), _created(usecTimestampNow()) 
        {
        }

        uint64_t age() const {
            return usecTimestampNow() - _created;
        }

        bool signaled() const {
            auto result = glClientWaitSync(_sync, 0, 0);
            if (GL_TIMEOUT_EXPIRED != result && GL_WAIT_FAILED != result) {
                return true;
            }
            return false;
        }
    };

    using Mutex = std::mutex;
    using Recycler = std::function<void(T t)>;
    // deque gives us random access, double ended push & pop and size, all in constant time
    using Deque = std::deque<Item>;
    using List = std::forward_list<Item>;
    
    void setRecycler(Recycler recycler) {
        _recycler = recycler;
    }

    template <typename F> 
    void withLock(F f) {
        using Lock = std::unique_lock<Mutex>;
        Lock lock(_mutex);
        f();
    }

    template <typename F>
    bool tryLock(F f) {
        using Lock = std::unique_lock<Mutex>;
        bool result = false;
        Lock lock(_mutex, std::try_to_lock_t());
        if (lock.owns_lock()) {
            f();
            result = true;
        }
        return result;
    }


    size_t depth() {
        size_t result{ 0 };
        withLock([&]{
            result = _submits.size();
        });
        return result;
    }

    // Submit a new resource from the producer context
    // returns the number of prior submissions that were
    // never consumed before becoming available.
    // producers should self-limit if they start producing more
    // work than is being consumed;
    size_t submit(T t, GLsync writeSync = 0) {
        if (!writeSync) {
            // FIXME should the release and submit actually force the creation of a fence?
            writeSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glFlush();
        }

        withLock([&]{
            _submits.push_back(Item(t, writeSync));
        });
        return cleanTrash();
    }

    // Returns the next available resource provided by the submitter,
    // or if none is available (which could mean either the submission
    // list is empty or that the first item on the list isn't yet signaled
    // Deprecated... will inject an unecessary GPU bubble
    bool fetchSignaled(T& t) {
        bool result = false;
        // On the one hand using try_lock() reduces the chance of blocking the consumer thread,
        // but if the produce thread is going fast enough, it could effectively
        // starve the consumer out of ever actually getting resources.
        tryLock([&] {
            // May be called on any thread, but must be inside a locked section
            if (signaled(_submits, 0)) {
                result = true;
                t = _submits.at(0)._value;
                _submits.pop_front();
            }
        });
        return result;
    }

    // Populates t with the next available resource provided by the submitter
    // and sync with a fence that will be signaled when all write commands for the 
    // item have completed.  Returns false if no resources are available
    bool fetchWithFence(T& t, GLsync& sync) {
        bool result = false;
        // On the one hand using try_lock() reduces the chance of blocking the consumer thread,
        // but if the produce thread is going fast enough, it could effectively
        // starve the consumer out of ever actually getting resources.
        tryLock([&] {
            if (!_submits.empty()) {
                result = true;
                // When fetching with sync, we only want the latest item
                auto item = _submits.back();
                _submits.pop_back();

                // Throw everything else in the trash
                for (const auto& oldItem : _submits) {
                    _trash.push_front(oldItem);
                }
                _submits.clear();

                t = item._value;
                sync = item._sync;
            }
        });
        return result;
    }

    bool fetchWithGpuWait(T& t) {
        GLsync sync { 0 };
        if (fetchWithFence(t, sync)) {
            // Texture was updated, inject a wait into the GL command stream to ensure 
            // commands on this context until the commands to generate t are finished.
            if (sync != 0) {
                glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
                glDeleteSync(sync);
            }
            return true;
        }
        return false;
    }

    // Returns the next available resource provided by the submitter,
    // or if none is available (which could mean either the submission
    // list is empty or that the first item on the list isn't yet signaled
    // Also releases any previous texture held by the caller
    bool fetchSignaledAndRelease(T& value) {
        T originalValue = value;
        if (fetchSignaled(value)) {
            if (originalValue != invalid()) {
                release(originalValue);
            }
            return true;
        }
        return false;
    }

    bool fetchAndReleaseWithFence(T& value, GLsync& sync) {
        T originalValue = value;
        if (fetchWithFence(value, sync)) {
            if (originalValue != invalid()) {
                release(originalValue);
            }
            return true;
        }
        return false;
    }

    bool fetchAndReleaseWithGpuWait(T& value) {
        T originalValue = value;
        if (fetchWithGpuWait(value)) {
            if (originalValue != invalid()) {
                release(originalValue);
            }
            return true;
        }
        return false;
    }

    // If fetch returns a non-zero value, it's the responsibility of the
    // client to release it at some point
    void release(const T& t, GLsync readSync = 0) {
        if (!readSync) {
            // FIXME should the release and submit actually force the creation of a fence?
            readSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glFlush();
        }

        withLock([&]{
            _releases.push_back(Item(t, readSync));
        });
    }
    
private:
    size_t cleanTrash() {
        size_t wastedWork{ 0 };
        List trash;
        tryLock([&]{
            while (!_submits.empty()) {
                const auto& item = _submits.front();
                if (!item._sync || item.age() < MAX_UNSIGNALED_TIME) {
                    break;
                }
                qWarning() << "Long unsignaled sync " << item._sync << " unsignaled for " << item.age();
                _trash.push_front(item);
                _submits.pop_front();
            }

            // We only ever need one ready item available in the list, so if the
            // second item is signaled (implying the first is as well, remove the first
            // item.  Iterate until the SECOND item in the list is not in the ready state
            // The signaled function takes care of checking against the deque size
            while (signaled(_submits, 1)) {
                _trash.push_front(_submits.front());
                _submits.pop_front();
                ++wastedWork;
            }

            // Stuff in the release queue can be cleared out as soon as it's signaled
            while (signaled(_releases, 0)) {
                _trash.push_front(_releases.front());
                _releases.pop_front();
            }

            trash.swap(_trash);
        });
           
        // FIXME maybe doing a timing on the deleters and warn if it's taking excessive time?
        // although we are out of the lock, so it shouldn't be blocking anything
        std::for_each(trash.begin(), trash.end(), [&](typename List::const_reference item) {
            if (item._value) {
                _recycler(item._value);
            }
            if (item._sync) {
                glDeleteSync(item._sync);
            }
        });
        return wastedWork;
    }

    // May be called on any thread, but must be inside a locked section
    bool signaled(Deque& deque, size_t i) {
        if (i >= deque.size()) {
            return false;
        }
        
        auto& item = deque.at(i);
        // If there's no sync object, either it's not required or it's already been found to be signaled
        if (!item._sync) {
            return true;
        }
        
        // Check the sync value using a zero timeout to ensure we don't block
        // This is critically important as this is the only GL function we'll call
        // inside the locked sections, so it cannot have any latency
        if (item.signaled()) {
            // if the sync is signaled, queue it for deletion
            _trash.push_front(Item(invalid(), item._sync));
            // And change the stored value to 0 so we don't check it again
            item._sync = 0;
            return true;
        }

        return false;
    }

    Mutex _mutex;
    Recycler _recycler;
    // Items coming from the submission / writer context
    Deque _submits;
    // Items coming from the client context.
    Deque _releases;
    // Items which are no longer in use.
    List _trash;
};

template<>
inline const GLuint& GLEscrow<GLuint>::invalid() const {
    static const GLuint INVALID_RESULT { 0 };
    return INVALID_RESULT;
}

using GLTextureEscrow = GLEscrow<GLuint>;

#endif


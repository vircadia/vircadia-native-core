/**
 *    @file Threading.h
 *    A delegate pattern to encapsulate threading policy semantics.
 *
 *    @author: Norman Crafts
 *    @copyright 2014, All rights reserved.
 */
#ifndef __THREADING_H__
#define __THREADING_H__

/** 
 *    @class SingleThreaded
 */
template
<
    class MutexPolicy = Mutex                    ///< Mutex policy
>
class SingleThreaded
{
public:

    void lock();

    void unlock();
};

/**
 *    @class MultiThreaded
 */
template
<
    class MutexPolicy = Mutex                    ///< Mutex policy
>
class MultiThreaded
{
public:

    void lock();

    void unlock();

private:
    MutexPolicy _mutex;
};




template <class MutexPolicy>
inline
void SingleThreaded<MutexPolicy>::lock()
{}

template <class MutexPolicy>
inline
void SingleThreaded<MutexPolicy>::unlock()
{}





template <class MutexPolicy>
inline
void MultiThreaded<MutexPolicy>::lock()
{
    _mutex.lock();
}

template <class MutexPolicy>
inline
void MultiThreaded<MutexPolicy>::unlock()
{
    _mutex.unlock();
}


#endif    // __THREADING_H__

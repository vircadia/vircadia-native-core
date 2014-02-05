/**
 *    @file Queue.h
 *    An efficient FIFO queue featuring lock-free interaction between
 *    producer and consumer.
 *
 *    @author:  Norman Crafts
 *    @copyright 2014, All rights reserved.
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "AutoLock.h"
#include "Mutex.h"
#include "Threading.h"

/**
 *    @class Queue
 *
 *    A template based, efficient first-in/first-out queue featuring lock free 
 *    access between producer and consumer.
 */
template
<
    class Client,                                // Managed client class
    template <class> class ProducerThreadingPolicy = MultiThreaded,
    template <class> class ConsumerThreadingPolicy = MultiThreaded,
    class ProducerMutexPolicy = Mutex,
    class ConsumerMutexPolicy = Mutex
>
class Queue
{
public:
    /** Default constructor
     */
    Queue();

    ~Queue();

    /** Add an object of managed client class to the tail of the queue
     */
    void add(
        Client item
        );

    /** Remove an object of managed client class from the head of the queue
     */
    Client remove();

    /** Peek at an object of managed client class at the head of the queue
     *    but do not remove it.
     */
    Client peek();

    /** Is the queue empty?
     */
    bool isEmpty();

private:
    struct __node
    {
        /** Default node constructor
         */
        __node() :
            _item(),
            _next(0)
            {}

        /** Item injection constructor
         */
        __node(
            Client item
            ) :
            _item(item),
            _next(0)
            {}

        Client _item;
        struct __node *_next;
    };

    static const int s_cacheLineSize = 64;        ///< Cache line size (in bytes) of standard Intel architecture
    char pad0[s_cacheLineSize];                    ///< Padding to eliminate cache line contention between threads

    ProducerThreadingPolicy<ProducerMutexPolicy> _producerGuard;
    char pad1[s_cacheLineSize - sizeof(ProducerThreadingPolicy<ProducerMutexPolicy>)];

    ConsumerThreadingPolicy<ConsumerMutexPolicy> _consumerGuard;
    char pad2[s_cacheLineSize - sizeof(ConsumerThreadingPolicy<ConsumerMutexPolicy>)];

    struct __node *_first;                        ///< Queue management
    char pad3[s_cacheLineSize - sizeof(struct __node *)];

    struct __node *_last;                        ///< Queue management
    char pad4[s_cacheLineSize - sizeof(struct __node *)];
};








template
    <
        class Client,
        template <class> class ProducerThreadingPolicy,
        template <class> class ConsumerThreadingPolicy,
        class ProducerMutexPolicy,
        class ConsumerMutexPolicy
    >
Queue
    <
        Client,
        ProducerThreadingPolicy,
        ConsumerThreadingPolicy,
        ProducerMutexPolicy,
        ConsumerMutexPolicy
    >::Queue()
{
    _first = new __node();
    _last = _first;
    memset(pad0, 0, sizeof(pad0));
    memset(pad1, 0, sizeof(pad1));
    memset(pad2, 0, sizeof(pad2));
    memset(pad3, 0, sizeof(pad3));
    memset(pad4, 0, sizeof(pad4));
}

template
    <
        class Client,
        template <class> class ProducerThreadingPolicy,
        template <class> class ConsumerThreadingPolicy,
        class ProducerMutexPolicy,
        class ConsumerMutexPolicy
    >
Queue
    <
        Client,
        ProducerThreadingPolicy,
        ConsumerThreadingPolicy,
        ProducerMutexPolicy,
        ConsumerMutexPolicy
    >::~Queue()
{
    AutoLock<ConsumerThreadingPolicy<ConsumerMutexPolicy> > lock(_consumerGuard);
    while (_first)
    {
        struct __node *t = _first;
        _first = t->_next;
        delete t;
    }
}


template
    <
        class Client,
        template <class> class ProducerThreadingPolicy,
        template <class> class ConsumerThreadingPolicy,
        class ProducerMutexPolicy,
        class ConsumerMutexPolicy
    >
void Queue
    <
        Client,
        ProducerThreadingPolicy,
        ConsumerThreadingPolicy,
        ProducerMutexPolicy,
        ConsumerMutexPolicy
    >::add(
    Client item
    )
{
    struct __node *n = new __node(item);
    AutoLock<ProducerThreadingPolicy<ProducerMutexPolicy> > lock(_producerGuard);
    _last->_next = n;
    _last = n;
}



template
    <
        class Client,
        template <class> class ProducerThreadingPolicy,
        template <class> class ConsumerThreadingPolicy,
        class ProducerMutexPolicy,
        class ConsumerMutexPolicy
    >
Client Queue
    <
        Client,
        ProducerThreadingPolicy,
        ConsumerThreadingPolicy,
        ProducerMutexPolicy,
        ConsumerMutexPolicy
    >::remove()
{
    AutoLock<ConsumerThreadingPolicy<ConsumerMutexPolicy> > lock(_consumerGuard);
    struct __node *next  = _first->_next;
    if (next)
    {
        struct __node *first = _first;
        Client item = next->_item;
        next->_item = 0;
        _first = next;
        delete first;
        return item;
    }
    return 0;
}



template
    <
        class Client,
        template <class> class ProducerThreadingPolicy,
        template <class> class ConsumerThreadingPolicy,
        class ProducerMutexPolicy,
        class ConsumerMutexPolicy
    >
Client Queue
    <
        Client,
        ProducerThreadingPolicy,
        ConsumerThreadingPolicy,
        ProducerMutexPolicy,
        ConsumerMutexPolicy
    >::peek()
{
    AutoLock<ConsumerThreadingPolicy<ConsumerMutexPolicy> > lock(_consumerGuard);
    struct __node *next  = _first->_next;
    if (next)
    {
        Client item = next->_item;
        return item;
    }
    return 0;
}



template
    <
        class Client,
        template <class> class ProducerThreadingPolicy,
        template <class> class ConsumerThreadingPolicy,
        class ProducerMutexPolicy,
        class ConsumerMutexPolicy
    >
bool Queue
    <
        Client,
        ProducerThreadingPolicy,
        ConsumerThreadingPolicy,
        ProducerMutexPolicy,
        ConsumerMutexPolicy
    >::isEmpty()
{
    bool empty = true;
    AutoLock<ConsumerThreadingPolicy<ConsumerMutexPolicy> > lock(_consumerGuard);
    struct __node *next  = _first->_next;
    if (next)
        empty = false;

    return empty;
}

#endif    // __QUEUE_H__

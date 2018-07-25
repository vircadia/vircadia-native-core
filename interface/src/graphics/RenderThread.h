#ifndef PRODUCERCONSUMERPIPE_H
#define PRODUCERCONSUMERPIPE_H

#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>

// Producer is blocked if the consumer doesn't consume enough
// Consumer reads same value if producer doesn't produce enough
template <class T>
class ProducerConsumerPipe {
public:

    ProducerConsumerPipe();
    ProducerConsumerPipe(const T& initValue);

    const T& read();
    void read(T& value, const T& resetValue);
    void write(const T& value);
    bool isWritePossible();

private:

    short _readIndex;
    short _writeIndex;
    std::array<T, 3> _values;
    std::array<std::atomic_flag, 3> _used;

    void initialize();
    void updateReadIndex();
};

template <class T>
ProducerConsumerPipe<T>::ProducerConsumerPipe() {
    initialize();
}

template <class T>
ProducerConsumerPipe<T>::ProducerConsumerPipe(const T& initValue) {
    _values.fill(initValue);
    initialize();
}

template <class T>
void ProducerConsumerPipe<T>::initialize() {
    _readIndex = 0;
    _writeIndex = 2;
    _used[_readIndex].test_and_set(std::memory_order_acquire);
    _used[_writeIndex].test_and_set(std::memory_order_acquire);
    _used[1].clear();
}

template <class T>
void ProducerConsumerPipe<T>::updateReadIndex() {
    int nextReadIndex = (_readIndex + 1) % _values.size();

    if (!_used[nextReadIndex].test_and_set(std::memory_order_acquire)) {
        int readIndex = _readIndex;
        _used[readIndex].clear(std::memory_order_release);
        _readIndex = nextReadIndex;
    }
}

template <class T>
const T& ProducerConsumerPipe<T>::read() {
    updateReadIndex();
    return _values[_readIndex];
}

template <class T>
void ProducerConsumerPipe<T>::read(T& value, const T& resetValue) {
    updateReadIndex();
    value = _values[_readIndex];
    _values[_readIndex] = resetValue;
}

template <class T>
bool ProducerConsumerPipe<T>::isWritePossible() {
    int nextWriteIndex = (_writeIndex + 1) % _values.size();
    return (_used[nextWriteIndex].test_and_set(std::memory_order_acquire));
}

template <class T>
void ProducerConsumerPipe<T>::write(const T& value) {
    int nextWriteIndex = (_writeIndex + 1) % _values.size();
    int writeIndex = _writeIndex;

    _values[writeIndex] = value;

    while (_used[nextWriteIndex].test_and_set(std::memory_order_acquire)) {
        // spin
        std::this_thread::yield();
    }
    _used[writeIndex].clear(std::memory_order_release);
    _writeIndex = nextWriteIndex;
}


#include <gpu/Frame.h>

using FrameQueue = ProducerConsumerPipe<gpu::FramePointer>;

using FrameQueuePointer = std::shared_ptr<FrameQueue>;


#endif

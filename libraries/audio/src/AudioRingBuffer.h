//
//  AudioRingBuffer.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioRingBuffer_h
#define hifi_AudioRingBuffer_h

#include <limits>
#include <stdint.h>

#include <glm/glm.hpp>

#include <QtCore/QIODevice>

#include "NodeData.h"
#include "SharedUtil.h"

const int SAMPLE_RATE = 24000;

const int NETWORK_BUFFER_LENGTH_BYTES_STEREO = 1024;
const int NETWORK_BUFFER_LENGTH_SAMPLES_STEREO = NETWORK_BUFFER_LENGTH_BYTES_STEREO / sizeof(int16_t);
const int NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL = 512;
const int NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL = NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL / sizeof(int16_t);

const unsigned int BUFFER_SEND_INTERVAL_USECS = floorf((NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL
                                                        / (float) SAMPLE_RATE) * USECS_PER_SECOND);

const int MAX_SAMPLE_VALUE = std::numeric_limits<int16_t>::max();
const int MIN_SAMPLE_VALUE = std::numeric_limits<int16_t>::min();

const int DEFAULT_RING_BUFFER_FRAME_CAPACITY = 10;

class AudioRingBuffer : public NodeData {
    Q_OBJECT
public:
    AudioRingBuffer(int numFrameSamples, bool randomAccessMode = false, int numFramesCapacity = DEFAULT_RING_BUFFER_FRAME_CAPACITY);
    ~AudioRingBuffer();

    void reset();
    void resizeForFrameSize(int numFrameSamples);
    
    int getSampleCapacity() const { return _sampleCapacity; }
    int getFrameCapacity() const { return _frameCapacity; }
    
    int parseData(const QByteArray& packet);
    
    // assume callers using this will never wrap around the end
    const int16_t* getNextOutput() const { return _nextOutput; }
    const int16_t* getBuffer() const { return _buffer; }

    int readSamples(int16_t* destination, int maxSamples);
    int writeSamples(const int16_t* source, int maxSamples);
    
    int readData(char* data, int maxSize);
    int writeData(const char* data, int maxSize);
    
    int16_t& operator[](const int index);
    const int16_t& operator[] (const int index) const;
    
    void shiftReadPosition(unsigned int numSamples);

    float getNextOutputFrameLoudness() const;
    
    int samplesAvailable() const;
    int framesAvailable() const { return samplesAvailable() / _numFrameSamples; }

    int getNumFrameSamples() const { return _numFrameSamples; }
    
    bool isNotStarvedOrHasMinimumSamples(int numRequiredSamples) const;
    
    bool isStarved() const { return _isStarved; }
    void setIsStarved(bool isStarved) { _isStarved = isStarved; }
    
    int getOverflowCount() const { return _overflowCount; } /// how many times has the ring buffer has overwritten old data
    bool hasStarted() const { return _hasStarted; }
    
    int addSilentFrame(int numSilentSamples);
protected:
    // disallow copying of AudioRingBuffer objects
    AudioRingBuffer(const AudioRingBuffer&);
    AudioRingBuffer& operator= (const AudioRingBuffer&);
    
    int16_t* shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const;

    int _overflowCount; /// how many times has the ring buffer has overwritten old data
    
    int _frameCapacity;
    int _sampleCapacity;
    bool _isFull;
    int _numFrameSamples;
    int16_t* _nextOutput;
    int16_t* _endOfLastWrite;
    int16_t* _buffer;
    bool _isStarved;
    bool _hasStarted;
    bool _randomAccessMode; /// will this ringbuffer be used for random access? if so, do some special processing

public:
    class ConstIterator { //public std::iterator < std::forward_iterator_tag, int16_t > {
    public:
        ConstIterator()
            : _capacity(0),
            _bufferFirst(NULL),
            _bufferLast(NULL),
            _at(NULL) {}

        ConstIterator(int16_t* bufferFirst, int capacity, int16_t* at)
            : _capacity(capacity),
            _bufferFirst(bufferFirst),
            _bufferLast(bufferFirst + capacity - 1),
            _at(at) {}

        bool operator==(const ConstIterator& rhs) { return _at == rhs._at; }
        bool operator!=(const ConstIterator& rhs) { return _at != rhs._at; }
        int16_t operator*() { return *_at; }

        ConstIterator& operator=(const ConstIterator& rhs) {
            _capacity = rhs._capacity;
            _bufferFirst = rhs._bufferFirst;
            _bufferLast = rhs._bufferLast;
            _at = rhs._at;
            return *this;
        }

        ConstIterator& operator++() {
            _at = (_at == _bufferLast) ? _bufferFirst : _at + 1;
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp(*this);
            ++(*this);
            return tmp;
        }

        ConstIterator& operator--() {
            _at = (_at == _bufferFirst) ? _bufferLast : _at - 1;
            return *this;
        }

        ConstIterator operator--(int) {
            ConstIterator tmp(*this);
            --(*this);
            return tmp;
        }

        int16_t operator[] (int i) {
            return *atShiftedBy(i);
        }

        ConstIterator operator+(int i) {
            return ConstIterator(_bufferFirst, _capacity, atShiftedBy(i));
        }

        ConstIterator operator-(int i) {
            return ConstIterator(_bufferFirst, _capacity, atShiftedBy(-i));
        }
    
    private:
        int16_t* atShiftedBy(int i) {
            i = (_at - _bufferFirst + i) % _capacity;
            if (i < 0) {
                i += _capacity;
            }
            return _bufferFirst + i;
        }

    private:
        int _capacity;
        int16_t* _bufferFirst;
        int16_t* _bufferLast;
        int16_t* _at;
    };


    ConstIterator nextOutput() const { return ConstIterator(_buffer, _sampleCapacity, _nextOutput); }
};

#endif // hifi_AudioRingBuffer_h

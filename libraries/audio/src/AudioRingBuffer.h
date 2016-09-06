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

#include "AudioConstants.h"

#include <QtCore/QIODevice>

#include <SharedUtil.h>
#include <NodeData.h>

const int DEFAULT_RING_BUFFER_FRAME_CAPACITY = 10;

class AudioRingBuffer {
public:
    AudioRingBuffer(int numFrameSamples, int numFramesCapacity = DEFAULT_RING_BUFFER_FRAME_CAPACITY);
    ~AudioRingBuffer();

    // disallow copying
    AudioRingBuffer(const AudioRingBuffer&) = delete;
    AudioRingBuffer(AudioRingBuffer&&) = delete;
    AudioRingBuffer& operator=(const AudioRingBuffer&) = delete;

    /// Invalidate any data in the buffer
    void clear();

    /// Clear and reset the overflow count
    void reset();

    /// Resize frame size (causes a reset())
    // FIXME: discards any data in the buffer
    void resizeForFrameSize(int numFrameSamples);

    /// Read up to maxSamples into destination (will only read up to samplesAvailable())
    /// Returns number of read samples
    int readSamples(int16_t* destination, int maxSamples);

    /// Write up to maxSamples from source (will only write up to sample capacity)
    /// Returns number of written samples
    int writeSamples(const int16_t* source, int maxSamples);

    /// Write up to maxSamples silent samples (will only write until other data exists in the buffer)
    /// This method will not overwrite existing data in the buffer, instead dropping silent samples that would overflow
    /// Returns number of written silent samples
    int addSilentSamples(int maxSamples);

    /// Read up to maxSize into destination
    /// Returns number of read bytes
    int readData(char* destination, int maxSize);

    /// Write up to maxSize from source
    /// Returns number of written bytes
    int writeData(const char* source, int maxSize);

    /// Returns a reference to the index-th sample offset from the current read sample
    int16_t& operator[](const int index) { return *shiftedPositionAccomodatingWrap(_nextOutput, index); }
    const int16_t& operator[] (const int index) const { return *shiftedPositionAccomodatingWrap(_nextOutput, index); }

    /// Essentially discards the next numSamples from the ring buffer
    /// NOTE: This is not checked - it is possible to shift past written data
    ///       Use samplesAvailable() to see the distance a valid shift can go
    void shiftReadPosition(unsigned int numSamples) { _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, numSamples); }

    int samplesAvailable() const;
    int framesAvailable() const { return (_numFrameSamples == 0) ? 0 : samplesAvailable() / _numFrameSamples; }
    float getNextOutputFrameLoudness() const { return getFrameLoudness(_nextOutput); }


    int getNumFrameSamples() const { return _numFrameSamples; }
    int getFrameCapacity() const { return _frameCapacity; }
    int getSampleCapacity() const { return _sampleCapacity; }
    /// Return times the ring buffer has overwritten old data
    int getOverflowCount() const { return _overflowCount; }

    class ConstIterator {
    public:
        ConstIterator();
        ConstIterator(int16_t* bufferFirst, int capacity, int16_t* at);
        ConstIterator(const ConstIterator& rhs) = default;

        bool isNull() const { return _at == NULL; }

        bool operator==(const ConstIterator& rhs) { return _at == rhs._at; }
        bool operator!=(const ConstIterator& rhs) { return _at != rhs._at; }
        const int16_t& operator*() { return *_at; }

        ConstIterator& operator=(const ConstIterator& rhs);
        ConstIterator& operator++();
        ConstIterator operator++(int);
        ConstIterator& operator--();
        ConstIterator operator--(int);
        const int16_t& operator[] (int i);
        ConstIterator operator+(int i);
        ConstIterator operator-(int i);

        void readSamples(int16_t* dest, int numSamples);
        void readSamplesWithFade(int16_t* dest, int numSamples, float fade);

    private:
        int16_t* atShiftedBy(int i);

        int _bufferLength;
        int16_t* _bufferFirst;
        int16_t* _bufferLast;
        int16_t* _at;
    };

    ConstIterator nextOutput() const;
    ConstIterator lastFrameWritten() const;

    int writeSamples(ConstIterator source, int maxSamples);
    int writeSamplesWithFade(ConstIterator source, int maxSamples, float fade);

    float getFrameLoudness(ConstIterator frameStart) const;

protected:
    int16_t* shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const;
    float getFrameLoudness(const int16_t* frameStart) const;

    int _numFrameSamples;
    int _frameCapacity;
    int _sampleCapacity;
    int _bufferLength; // actual _buffer length (_sampleCapacity + 1)
    int _overflowCount{ 0 }; // times the ring buffer has overwritten data

    int16_t* _nextOutput{ nullptr };
    int16_t* _endOfLastWrite{ nullptr };
    int16_t* _buffer{ nullptr };
};

// inline the iterator:
inline AudioRingBuffer::ConstIterator::ConstIterator() :
    _bufferLength(0),
    _bufferFirst(NULL),
    _bufferLast(NULL),
    _at(NULL) {}

inline AudioRingBuffer::ConstIterator::ConstIterator(int16_t* bufferFirst, int capacity, int16_t* at) :
    _bufferLength(capacity),
    _bufferFirst(bufferFirst),
    _bufferLast(bufferFirst + capacity - 1),
    _at(at) {}

inline AudioRingBuffer::ConstIterator& AudioRingBuffer::ConstIterator::operator=(const ConstIterator& rhs) {
    _bufferLength = rhs._bufferLength;
    _bufferFirst = rhs._bufferFirst;
    _bufferLast = rhs._bufferLast;
    _at = rhs._at;
    return *this;
}

inline AudioRingBuffer::ConstIterator& AudioRingBuffer::ConstIterator::operator++() {
    _at = (_at == _bufferLast) ? _bufferFirst : _at + 1;
    return *this;
}

inline AudioRingBuffer::ConstIterator AudioRingBuffer::ConstIterator::operator++(int) {
    ConstIterator tmp(*this);
    ++(*this);
    return tmp;
}

inline AudioRingBuffer::ConstIterator& AudioRingBuffer::ConstIterator::operator--() {
    _at = (_at == _bufferFirst) ? _bufferLast : _at - 1;
    return *this;
}

inline AudioRingBuffer::ConstIterator AudioRingBuffer::ConstIterator::operator--(int) {
    ConstIterator tmp(*this);
    --(*this);
    return tmp;
}

inline const int16_t& AudioRingBuffer::ConstIterator::operator[] (int i) {
    return *atShiftedBy(i);
}

inline AudioRingBuffer::ConstIterator AudioRingBuffer::ConstIterator::operator+(int i) {
    return ConstIterator(_bufferFirst, _bufferLength, atShiftedBy(i));
}

inline AudioRingBuffer::ConstIterator AudioRingBuffer::ConstIterator::operator-(int i) {
    return ConstIterator(_bufferFirst, _bufferLength, atShiftedBy(-i));
}

inline int16_t* AudioRingBuffer::ConstIterator::atShiftedBy(int i) {
    i = (_at - _bufferFirst + i) % _bufferLength;
    if (i < 0) {
        i += _bufferLength;
    }
    return _bufferFirst + i;
}

inline void AudioRingBuffer::ConstIterator::readSamples(int16_t* dest, int numSamples) {
    auto samplesToEnd = _bufferLast - _at + 1;

    if (samplesToEnd >= numSamples) {
        memcpy(dest, _at, numSamples * sizeof(int16_t));
        _at += numSamples;
    } else {
        auto samplesFromStart = numSamples - samplesToEnd;
        memcpy(dest, _at, samplesToEnd * sizeof(int16_t));
        memcpy(dest + samplesToEnd, _bufferFirst, samplesFromStart * sizeof(int16_t));

        _at = _bufferFirst + samplesFromStart;
    }
}

inline void AudioRingBuffer::ConstIterator::readSamplesWithFade(int16_t* dest, int numSamples, float fade) {
    int16_t* at = _at;
    for (int i = 0; i < numSamples; i++) {
        *dest = (float)*at * fade;
        ++dest;
        at = (at == _bufferLast) ? _bufferFirst : at + 1;
    }
}

inline AudioRingBuffer::ConstIterator AudioRingBuffer::nextOutput() const {
    return ConstIterator(_buffer, _bufferLength, _nextOutput);
}

inline AudioRingBuffer::ConstIterator AudioRingBuffer::lastFrameWritten() const {
    return ConstIterator(_buffer, _bufferLength, _endOfLastWrite) - _numFrameSamples;
}

#endif // hifi_AudioRingBuffer_h

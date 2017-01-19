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

template <class T>
class AudioRingBufferTemplate {
    using Sample = T;
    static size_t SampleSize = sizeof(Sample);

public:
    AudioRingBufferTemplate(int numFrameSamples, int numFramesCapacity = DEFAULT_RING_BUFFER_FRAME_CAPACITY);
    ~AudioRingBufferTemplate();

    // disallow copying
    AudioRingBufferTemplate(const AudioRingBufferTemplate&) = delete;
    AudioRingBufferTemplate(AudioRingBufferTemplate&&) = delete;
    AudioRingBufferTemplate& operator=(const AudioRingBufferTemplate&) = delete;

    /// Invalidate any data in the buffer
    void clear();

    /// Clear and reset the overflow count
    void reset();

    /// Resize frame size (causes a reset())
    // FIXME: discards any data in the buffer
    void resizeForFrameSize(int numFrameSamples);

    /// Read up to maxSamples into destination (will only read up to samplesAvailable())
    /// Returns number of read samples
    int readSamples(Sample* destination, int maxSamples);

    /// Write up to maxSamples from source (will only write up to sample capacity)
    /// Returns number of written samples
    int writeSamples(const Sample* source, int maxSamples);

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
    Sample& operator[](const int index) { return *shiftedPositionAccomodatingWrap(_nextOutput, index); }
    const Sample& operator[] (const int index) const { return *shiftedPositionAccomodatingWrap(_nextOutput, index); }

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
        ConstIterator(Sample* bufferFirst, int capacity, Sample* at);
        ConstIterator(const ConstIterator& rhs) = default;

        bool isNull() const { return _at == NULL; }

        bool operator==(const ConstIterator& rhs) { return _at == rhs._at; }
        bool operator!=(const ConstIterator& rhs) { return _at != rhs._at; }
        const Sample& operator*() { return *_at; }

        ConstIterator& operator=(const ConstIterator& rhs);
        ConstIterator& operator++();
        ConstIterator operator++(int);
        ConstIterator& operator--();
        ConstIterator operator--(int);
        const Sample& operator[] (int i);
        ConstIterator operator+(int i);
        ConstIterator operator-(int i);

        void readSamples(Sample* dest, int numSamples);
        void readSamplesWithFade(Sample* dest, int numSamples, float fade);
        void readSamplesWithUpmix(Sample* dest, int numSamples, int numExtraChannels);
        void readSamplesWithDownmix(Sample* dest, int numSamples);

    private:
        Sample* atShiftedBy(int i);

        int _bufferLength;
        Sample* _bufferFirst;
        Sample* _bufferLast;
        Sample* _at;
    };

    ConstIterator nextOutput() const;
    ConstIterator lastFrameWritten() const;

    int writeSamples(ConstIterator source, int maxSamples);
    int writeSamplesWithFade(ConstIterator source, int maxSamples, float fade);

    float getFrameLoudness(ConstIterator frameStart) const;

protected:
    Sample* shiftedPositionAccomodatingWrap(Sample* position, int numSamplesShift) const;
    float getFrameLoudness(const Sample* frameStart) const;

    int _numFrameSamples;
    int _frameCapacity;
    int _sampleCapacity;
    int _bufferLength; // actual _buffer length (_sampleCapacity + 1)
    int _overflowCount{ 0 }; // times the ring buffer has overwritten data

    Sample* _nextOutput{ nullptr };
    Sample* _endOfLastWrite{ nullptr };
    Sample* _buffer{ nullptr };
};

using AudioRingBuffer = AudioRingBufferTemplate<int16_t>;
using AudioRingMixBuffer = AudioRingBufferTemplate<float>;

// inline the iterator:
inline AudioRingBufferTemplate<int16_t>::ConstIterator::ConstIterator() :
    _bufferLength(0),
    _bufferFirst(NULL),
    _bufferLast(NULL),
    _at(NULL) {}

inline AudioRingBufferTemplate<int16_t>::ConstIterator::ConstIterator(Sample* bufferFirst, int capacity, Sample* at) :
    _bufferLength(capacity),
    _bufferFirst(bufferFirst),
    _bufferLast(bufferFirst + capacity - 1),
    _at(at) {}

inline AudioRingBufferTemplate<int16_t>::ConstIterator& AudioRingBufferTemplate<int16_t>::ConstIterator::operator=(const ConstIterator& rhs) {
    _bufferLength = rhs._bufferLength;
    _bufferFirst = rhs._bufferFirst;
    _bufferLast = rhs._bufferLast;
    _at = rhs._at;
    return *this;
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator& AudioRingBufferTemplate<int16_t>::ConstIterator::operator++() {
    _at = (_at == _bufferLast) ? _bufferFirst : _at + 1;
    return *this;
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator AudioRingBufferTemplate<int16_t>::ConstIterator::operator++(int) {
    ConstIterator tmp(*this);
    ++(*this);
    return tmp;
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator& AudioRingBufferTemplate<int16_t>::ConstIterator::operator--() {
    _at = (_at == _bufferFirst) ? _bufferLast : _at - 1;
    return *this;
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator AudioRingBufferTemplate<int16_t>::ConstIterator::operator--(int) {
    ConstIterator tmp(*this);
    --(*this);
    return tmp;
}

inline const int16_t& AudioRingBufferTemplate<int16_t>::ConstIterator::operator[] (int i) {
    return *atShiftedBy(i);
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator AudioRingBufferTemplate<int16_t>::ConstIterator::operator+(int i) {
    return ConstIterator(_bufferFirst, _bufferLength, atShiftedBy(i));
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator AudioRingBufferTemplate<int16_t>::ConstIterator::operator-(int i) {
    return ConstIterator(_bufferFirst, _bufferLength, atShiftedBy(-i));
}

inline int16_t* AudioRingBufferTemplate<int16_t>::ConstIterator::atShiftedBy(int i) {
    i = (_at - _bufferFirst + i) % _bufferLength;
    if (i < 0) {
        i += _bufferLength;
    }
    return _bufferFirst + i;
}

inline void AudioRingBufferTemplate<int16_t>::ConstIterator::readSamples(Sample* dest, int numSamples) {
    auto samplesToEnd = _bufferLast - _at + 1;

    if (samplesToEnd >= numSamples) {
        memcpy(dest, _at, numSamples * SampleSize);
        _at += numSamples;
    } else {
        auto samplesFromStart = numSamples - samplesToEnd;
        memcpy(dest, _at, samplesToEnd * SampleSize);
        memcpy(dest + samplesToEnd, _bufferFirst, samplesFromStart * SampleSize);

        _at = _bufferFirst + samplesFromStart;
    }
}

inline void AudioRingBufferTemplate<int16_t>::ConstIterator::readSamplesWithFade(Sample* dest, int numSamples, float fade) {
    Sample* at = _at;
    for (int i = 0; i < numSamples; i++) {
        *dest = (float)*at * fade;
        ++dest;
        at = (at == _bufferLast) ? _bufferFirst : at + 1;
    }
}

inline void AudioRingBufferTemplate<int16_t>::ConstIterator::readSamplesWithUpmix(Sample* dest, int numSamples, int numExtraChannels) {
    Sample* at = _at;
    for (int i = 0; i < numSamples/2; i++) {

        // read 2 samples
        Sample left = *at;
        at = (at == _bufferLast) ? _bufferFirst : at + 1;
        Sample right = *at;
        at = (at == _bufferLast) ? _bufferFirst : at + 1;

        // write 2 + N samples
        *dest++ = left;
        *dest++ = right;
        for (int n = 0; n < numExtraChannels; n++) {
            *dest++ = 0;
        }
    }
}

inline void AudioRingBufferTemplate<int16_t>::ConstIterator::readSamplesWithDownmix(Sample* dest, int numSamples) {
    Sample* at = _at;
    for (int i = 0; i < numSamples/2; i++) {

        // read 2 samples
        Sample left = *at;
        at = (at == _bufferLast) ? _bufferFirst : at + 1;
        Sample right = *at;
        at = (at == _bufferLast) ? _bufferFirst : at + 1;

        // write 1 sample
        *dest++ = (Sample)((left + right) / 2);
    }
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator AudioRingBufferTemplate<int16_t>::nextOutput() const {
    return ConstIterator(_buffer, _bufferLength, _nextOutput);
}

inline AudioRingBufferTemplate<int16_t>::ConstIterator AudioRingBufferTemplate<int16_t>::lastFrameWritten() const {
    return ConstIterator(_buffer, _bufferLength, _endOfLastWrite) - _numFrameSamples;
}

#endif // hifi_AudioRingBuffer_h

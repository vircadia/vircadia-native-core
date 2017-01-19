//
//  AudioRingBuffer.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstdlib>
#include <cstring>
#include <functional>
#include <math.h>

#include <QtCore/QDebug>

#include <udt/PacketHeaders.h>
#include <LogHandler.h>

#include "AudioLogging.h"

#include "AudioRingBuffer.h"

static const QString RING_BUFFER_OVERFLOW_DEBUG { "AudioRingBuffer::writeData has overflown the buffer. Overwriting old data." };
static const QString DROPPED_SILENT_DEBUG { "AudioRingBuffer::addSilentSamples dropping silent samples to prevent overflow." };

template<>
AudioRingBufferTemplate<int16_t>::AudioRingBufferTemplate(int numFrameSamples, int numFramesCapacity) :
    _numFrameSamples(numFrameSamples),
    _frameCapacity(numFramesCapacity),
    _sampleCapacity(numFrameSamples * numFramesCapacity),
    _bufferLength(numFrameSamples * (numFramesCapacity + 1))
{
    if (numFrameSamples) {
        _buffer = new Sample[_bufferLength];
        memset(_buffer, 0, _bufferLength * SampleSize);
        _nextOutput = _buffer;
        _endOfLastWrite = _buffer;
    }

    static QString repeatedOverflowMessage = LogHandler::getInstance().addRepeatedMessageRegex(RING_BUFFER_OVERFLOW_DEBUG);
    static QString repeatedDroppedMessage = LogHandler::getInstance().addRepeatedMessageRegex(DROPPED_SILENT_DEBUG);
}

template<>
AudioRingBufferTemplate<int16_t>::~AudioRingBufferTemplate() {
    delete[] _buffer;
}

template<>
void AudioRingBufferTemplate<int16_t>::clear() {
    _endOfLastWrite = _buffer;
    _nextOutput = _buffer;
}

template<>
void AudioRingBufferTemplate<int16_t>::reset() {
    clear();
    _overflowCount = 0;
}

template<>
void AudioRingBufferTemplate<int16_t>::resizeForFrameSize(int numFrameSamples) {
    delete[] _buffer;
    _numFrameSamples = numFrameSamples;
    _sampleCapacity = numFrameSamples * _frameCapacity;
    _bufferLength = numFrameSamples * (_frameCapacity + 1);

    if (numFrameSamples) {
        _buffer = new Sample[_bufferLength];
        memset(_buffer, 0, _bufferLength * SampleSize);
    } else {
        _buffer = nullptr;
    }

    reset();
}

template<>
int AudioRingBufferTemplate<int16_t>::readSamples(Sample* destination, int maxSamples) {
    return readData((char*)destination, maxSamples * SampleSize) / SampleSize;
}

template<>
int AudioRingBufferTemplate<int16_t>::writeSamples(const Sample* source, int maxSamples) {
    return writeData((char*)source, maxSamples * SampleSize) / SampleSize;
}

template<>
int AudioRingBufferTemplate<int16_t>::readData(char *data, int maxSize) {
    // only copy up to the number of samples we have available
    int maxSamples = maxSize / SampleSize;
    int numReadSamples = std::min(maxSamples, samplesAvailable());

    if (_nextOutput + numReadSamples > _buffer + _bufferLength) {
        // we're going to need to do two reads to get this data, it wraps around the edge
        int numSamplesToEnd = (_buffer + _bufferLength) - _nextOutput;

        // read to the end of the buffer
        memcpy(data, _nextOutput, numSamplesToEnd * SampleSize);

        // read the rest from the beginning of the buffer
        memcpy(data + (numSamplesToEnd * SampleSize), _buffer, (numReadSamples - numSamplesToEnd) * SampleSize);
    } else {
        memcpy(data, _nextOutput, numReadSamples * SampleSize);
    }

    shiftReadPosition(numReadSamples);

    return numReadSamples * SampleSize;
}

template<>
int AudioRingBufferTemplate<int16_t>::writeData(const char* data, int maxSize) {
    // only copy up to the number of samples we have capacity for
    int maxSamples = maxSize / SampleSize;
    int numWriteSamples = std::min(maxSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();

    if (numWriteSamples > samplesRoomFor) {
        // there's not enough room for this write. erase old data to make room for this new data
        int samplesToDelete = numWriteSamples - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;

        qCDebug(audio) << qPrintable(RING_BUFFER_OVERFLOW_DEBUG);
    }

    if (_endOfLastWrite + numWriteSamples > _buffer + _bufferLength) {
        // we're going to need to do two writes to set this data, it wraps around the edge
        int numSamplesToEnd = (_buffer + _bufferLength) - _endOfLastWrite;

        // write to the end of the buffer
        memcpy(_endOfLastWrite, data, numSamplesToEnd * SampleSize);

        // write the rest to the beginning of the buffer
        memcpy(_buffer, data + (numSamplesToEnd * SampleSize), (numWriteSamples - numSamplesToEnd) * SampleSize);
    } else {
        memcpy(_endOfLastWrite, data, numWriteSamples * SampleSize);
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, numWriteSamples);

    return numWriteSamples * SampleSize;
}

template<>
int AudioRingBufferTemplate<int16_t>::samplesAvailable() const {
    if (!_endOfLastWrite) {
        return 0;
    }

    int sampleDifference = _endOfLastWrite - _nextOutput;
    if (sampleDifference < 0) {
        sampleDifference += _bufferLength;
    }
    return sampleDifference;
}

template<>
int16_t* AudioRingBufferTemplate<int16_t>::shiftedPositionAccomodatingWrap(Sample* position, int numSamplesShift) const {
    // NOTE: It is possible to shift out-of-bounds if (|numSamplesShift| > 2 * _bufferLength), but this should not occur
    if (numSamplesShift > 0 && position + numSamplesShift >= _buffer + _bufferLength) {
        // this shift will wrap the position around to the beginning of the ring
        return position + numSamplesShift - _bufferLength;
    } else if (numSamplesShift < 0 && position + numSamplesShift < _buffer) {
        // this shift will go around to the end of the ring
        return position + numSamplesShift + _bufferLength;
    } else {
        return position + numSamplesShift;
    }
}

template<>
int AudioRingBufferTemplate<int16_t>::addSilentSamples(int silentSamples) {
    // NOTE: This implementation is nearly identical to writeData save for s/memcpy/memset, refer to comments there
    int numWriteSamples = std::min(silentSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();

    if (numWriteSamples > samplesRoomFor) {
        numWriteSamples = samplesRoomFor;

        qCDebug(audio) << qPrintable(DROPPED_SILENT_DEBUG);
    }

    if (_endOfLastWrite + numWriteSamples > _buffer + _bufferLength) {
        int numSamplesToEnd = (_buffer + _bufferLength) - _endOfLastWrite;
        memset(_endOfLastWrite, 0, numSamplesToEnd * SampleSize);
        memset(_buffer, 0, (numWriteSamples - numSamplesToEnd) * SampleSize);
    } else {
        memset(_endOfLastWrite, 0, numWriteSamples * SampleSize);
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, numWriteSamples);

    return numWriteSamples;
}

template<>
float AudioRingBufferTemplate<int16_t>::getFrameLoudness(const Sample* frameStart) const {
    // FIXME: This is a bad measure of loudness - normal estimation uses sqrt(sum(x*x))
    float loudness = 0.0f;
    const Sample* sampleAt = frameStart;
    const Sample* bufferLastAt = _buffer + _bufferLength - 1;

    for (int i = 0; i < _numFrameSamples; ++i) {
        loudness += (float) std::abs(*sampleAt);
        // wrap if necessary
        sampleAt = sampleAt == bufferLastAt ? _buffer : sampleAt + 1;
    }
    loudness /= _numFrameSamples;
    loudness /= AudioConstants::MAX_SAMPLE_VALUE;

    return loudness;
}

template<>
float AudioRingBufferTemplate<int16_t>::getFrameLoudness(ConstIterator frameStart) const {
    if (frameStart.isNull()) {
        return 0.0f;
    }
    return getFrameLoudness(&(*frameStart));
}

template<>
int AudioRingBufferTemplate<int16_t>::writeSamples(ConstIterator source, int maxSamples) {
    int samplesToCopy = std::min(maxSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (samplesToCopy > samplesRoomFor) {
        // there's not enough room for this write.  erase old data to make room for this new data
        int samplesToDelete = samplesToCopy - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;
        qCDebug(audio) << qPrintable(RING_BUFFER_OVERFLOW_DEBUG);
    }

    Sample* bufferLast = _buffer + _bufferLength - 1;
    for (int i = 0; i < samplesToCopy; i++) {
        *_endOfLastWrite = *source;
        _endOfLastWrite = (_endOfLastWrite == bufferLast) ? _buffer : _endOfLastWrite + 1;
        ++source;
    }

    return samplesToCopy;
}

template<>
int AudioRingBufferTemplate<int16_t>::writeSamplesWithFade(ConstIterator source, int maxSamples, float fade) {
    int samplesToCopy = std::min(maxSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (samplesToCopy > samplesRoomFor) {
        // there's not enough room for this write.  erase old data to make room for this new data
        int samplesToDelete = samplesToCopy - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;
        qCDebug(audio) << qPrintable(RING_BUFFER_OVERFLOW_DEBUG);
    }

    Sample* bufferLast = _buffer + _bufferLength - 1;
    for (int i = 0; i < samplesToCopy; i++) {
        *_endOfLastWrite = (Sample)((float)(*source) * fade);
        _endOfLastWrite = (_endOfLastWrite == bufferLast) ? _buffer : _endOfLastWrite + 1;
        ++source;
    }

    return samplesToCopy;
}

template<>
AudioRingBufferTemplate<float>::AudioRingBufferTemplate(int numFrameSamples, int numFramesCapacity) :
    _numFrameSamples(numFrameSamples),
    _frameCapacity(numFramesCapacity),
    _sampleCapacity(numFrameSamples * numFramesCapacity),
    _bufferLength(numFrameSamples * (numFramesCapacity + 1))
{
    if (numFrameSamples) {
        _buffer = new Sample[_bufferLength];
        memset(_buffer, 0, _bufferLength * SampleSize);
        _nextOutput = _buffer;
        _endOfLastWrite = _buffer;
    }

    static QString repeatedOverflowMessage = LogHandler::getInstance().addRepeatedMessageRegex(RING_BUFFER_OVERFLOW_DEBUG);
    static QString repeatedDroppedMessage = LogHandler::getInstance().addRepeatedMessageRegex(DROPPED_SILENT_DEBUG);
}

template<>
AudioRingBufferTemplate<float>::~AudioRingBufferTemplate() {
    delete[] _buffer;
}

template<>
void AudioRingBufferTemplate<float>::clear() {
    _endOfLastWrite = _buffer;
    _nextOutput = _buffer;
}

template<>
void AudioRingBufferTemplate<float>::reset() {
    clear();
    _overflowCount = 0;
}

template<>
void AudioRingBufferTemplate<float>::resizeForFrameSize(int numFrameSamples) {
    delete[] _buffer;
    _numFrameSamples = numFrameSamples;
    _sampleCapacity = numFrameSamples * _frameCapacity;
    _bufferLength = numFrameSamples * (_frameCapacity + 1);

    if (numFrameSamples) {
        _buffer = new Sample[_bufferLength];
        memset(_buffer, 0, _bufferLength * SampleSize);
    } else {
        _buffer = nullptr;
    }

    reset();
}

template<>
int AudioRingBufferTemplate<float>::readSamples(Sample* destination, int maxSamples) {
    return readData((char*)destination, maxSamples * SampleSize) / SampleSize;
}

template<>
int AudioRingBufferTemplate<float>::appendSamples(Sample* destination, int maxSamples, bool append) {
    if (append) {
        return appendData((char*)destination, maxSamples * SampleSize) / SampleSize;
    } else {
        return readData((char*)destination, maxSamples * SampleSize) / SampleSize;
    }
}

template<>
int AudioRingBufferTemplate<float>::writeSamples(const Sample* source, int maxSamples) {
    return writeData((char*)source, maxSamples * SampleSize) / SampleSize;
}

template<>
int AudioRingBufferTemplate<float>::readData(char *data, int maxSize) {
    // only copy up to the number of samples we have available
    int maxSamples = maxSize / SampleSize;
    int numReadSamples = std::min(maxSamples, samplesAvailable());

    if (_nextOutput + numReadSamples > _buffer + _bufferLength) {
        // we're going to need to do two reads to get this data, it wraps around the edge
        int numSamplesToEnd = (_buffer + _bufferLength) - _nextOutput;

        // read to the end of the buffer
        memcpy(data, _nextOutput, numSamplesToEnd * SampleSize);

        // read the rest from the beginning of the buffer
        memcpy(data + (numSamplesToEnd * SampleSize), _buffer, (numReadSamples - numSamplesToEnd) * SampleSize);
    } else {
        memcpy(data, _nextOutput, numReadSamples * SampleSize);
    }

    shiftReadPosition(numReadSamples);

    return numReadSamples * SampleSize;
}

template<>
int AudioRingBufferTemplate<float>::appendData(char *data, int maxSize) {
    // only copy up to the number of samples we have available
    int maxSamples = maxSize / SampleSize;
    int numReadSamples = std::min(maxSamples, samplesAvailable());

    Sample* dest = reinterpret_cast<Sample*>(data);
    Sample* output = _nextOutput;
    if (_nextOutput + numReadSamples > _buffer + _bufferLength) {
        // we're going to need to do two reads to get this data, it wraps around the edge
        int numSamplesToEnd = (_buffer + _bufferLength) - _nextOutput;

        // read to the end of the buffer
        for (int i = 0; i < numSamplesToEnd; i++) {
            *dest++ = *output++;
        }

        // read the rest from the beginning of the buffer
        output = _buffer;
        for (int i = 0; i < (numReadSamples - numSamplesToEnd); i++) {
            *dest++ = *output++;
        }
    } else {
        for (int i = 0; i < numReadSamples; i++) {
            *dest++ = *output++;
        }
    }

    shiftReadPosition(numReadSamples);

    return numReadSamples * SampleSize;
}

template<>
int AudioRingBufferTemplate<float>::writeData(const char* data, int maxSize) {
    // only copy up to the number of samples we have capacity for
    int maxSamples = maxSize / SampleSize;
    int numWriteSamples = std::min(maxSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();

    if (numWriteSamples > samplesRoomFor) {
        // there's not enough room for this write. erase old data to make room for this new data
        int samplesToDelete = numWriteSamples - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;

        qCDebug(audio) << qPrintable(RING_BUFFER_OVERFLOW_DEBUG);
    }

    if (_endOfLastWrite + numWriteSamples > _buffer + _bufferLength) {
        // we're going to need to do two writes to set this data, it wraps around the edge
        int numSamplesToEnd = (_buffer + _bufferLength) - _endOfLastWrite;

        // write to the end of the buffer
        memcpy(_endOfLastWrite, data, numSamplesToEnd * SampleSize);

        // write the rest to the beginning of the buffer
        memcpy(_buffer, data + (numSamplesToEnd * SampleSize), (numWriteSamples - numSamplesToEnd) * SampleSize);
    } else {
        memcpy(_endOfLastWrite, data, numWriteSamples * SampleSize);
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, numWriteSamples);

    return numWriteSamples * SampleSize;
}

template<>
int AudioRingBufferTemplate<float>::samplesAvailable() const {
    if (!_endOfLastWrite) {
        return 0;
    }

    int sampleDifference = _endOfLastWrite - _nextOutput;
    if (sampleDifference < 0) {
        sampleDifference += _bufferLength;
    }
    return sampleDifference;
}

template<>
float* AudioRingBufferTemplate<float>::shiftedPositionAccomodatingWrap(Sample* position, int numSamplesShift) const {
    // NOTE: It is possible to shift out-of-bounds if (|numSamplesShift| > 2 * _bufferLength), but this should not occur
    if (numSamplesShift > 0 && position + numSamplesShift >= _buffer + _bufferLength) {
        // this shift will wrap the position around to the beginning of the ring
        return position + numSamplesShift - _bufferLength;
    } else if (numSamplesShift < 0 && position + numSamplesShift < _buffer) {
        // this shift will go around to the end of the ring
        return position + numSamplesShift + _bufferLength;
    } else {
        return position + numSamplesShift;
    }
}
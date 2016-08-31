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

AudioRingBuffer::AudioRingBuffer(int numFrameSamples, int numFramesCapacity) :
    _numFrameSamples(numFrameSamples),
    _frameCapacity(numFramesCapacity),
    _sampleCapacity(numFrameSamples * numFramesCapacity),
    _bufferLength(numFrameSamples * (numFramesCapacity + 1))
{
    if (numFrameSamples) {
        _buffer = new int16_t[_bufferLength];
        memset(_buffer, 0, _bufferLength * sizeof(int16_t));
        _nextOutput = _buffer;
        _endOfLastWrite = _buffer;
    }

    static QString repeatedOverflowMessage = LogHandler::getInstance().addRepeatedMessageRegex(RING_BUFFER_OVERFLOW_DEBUG);
    static QString repeatedDroppedMessage = LogHandler::getInstance().addRepeatedMessageRegex(DROPPED_SILENT_DEBUG);
};

AudioRingBuffer::~AudioRingBuffer() {
    delete[] _buffer;
}

void AudioRingBuffer::clear() {
    _endOfLastWrite = _buffer;
    _nextOutput = _buffer;
}

void AudioRingBuffer::reset() {
    clear();
    _overflowCount = 0;
}

void AudioRingBuffer::resizeForFrameSize(int numFrameSamples) {
    delete[] _buffer;
    _numFrameSamples = numFrameSamples;
    _sampleCapacity = numFrameSamples * _frameCapacity;
    _bufferLength = numFrameSamples * (_frameCapacity + 1);

    if (numFrameSamples) {
        _buffer = new int16_t[_bufferLength];
        memset(_buffer, 0, _bufferLength * sizeof(int16_t));
    } else {
        _buffer = nullptr;
    }

    reset();
}

int AudioRingBuffer::readSamples(int16_t* destination, int maxSamples) {
    return readData((char*)destination, maxSamples * sizeof(int16_t)) / sizeof(int16_t);
}

int AudioRingBuffer::writeSamples(const int16_t* source, int maxSamples) {
    return writeData((char*)source, maxSamples * sizeof(int16_t)) / sizeof(int16_t);
}

int AudioRingBuffer::readData(char *data, int maxSize) {
    // only copy up to the number of samples we have available
    int maxSamples = maxSize / sizeof(int16_t);
    int numReadSamples = std::min(maxSamples, samplesAvailable());

    if (_nextOutput + numReadSamples > _buffer + _bufferLength) {
        // we're going to need to do two reads to get this data, it wraps around the edge
        int numSamplesToEnd = (_buffer + _bufferLength) - _nextOutput;

        // read to the end of the buffer
        memcpy(data, _nextOutput, numSamplesToEnd * sizeof(int16_t));

        // read the rest from the beginning of the buffer
        memcpy(data + (numSamplesToEnd * sizeof(int16_t)), _buffer, (numReadSamples - numSamplesToEnd) * sizeof(int16_t));
    } else {
        memcpy(data, _nextOutput, numReadSamples * sizeof(int16_t));
    }

    shiftReadPosition(numReadSamples);

    return numReadSamples * sizeof(int16_t);
}

int AudioRingBuffer::writeData(const char* data, int maxSize) {
    // only copy up to the number of samples we have capacity for
    int maxSamples = maxSize / sizeof(int16_t);
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
        memcpy(_endOfLastWrite, data, numSamplesToEnd * sizeof(int16_t));

        // write the rest to the beginning of the buffer
        memcpy(_buffer, data + (numSamplesToEnd * sizeof(int16_t)), (numWriteSamples - numSamplesToEnd) * sizeof(int16_t));
    } else {
        memcpy(_endOfLastWrite, data, numWriteSamples * sizeof(int16_t));
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, numWriteSamples);

    return numWriteSamples * sizeof(int16_t);
}

inline int16_t& AudioRingBuffer::operator[](const int index) {
    return *shiftedPositionAccomodatingWrap(_nextOutput, index);
}

inline const int16_t& AudioRingBuffer::operator[] (const int index) const {
    return *shiftedPositionAccomodatingWrap(_nextOutput, index);
}

inline void AudioRingBuffer::shiftReadPosition(unsigned int numSamples) {
    _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, numSamples);
}

int AudioRingBuffer::samplesAvailable() const {
    if (!_endOfLastWrite) {
        return 0;
    }

    int sampleDifference = _endOfLastWrite - _nextOutput;
    if (sampleDifference < 0) {
        sampleDifference += _bufferLength;
    }
    return sampleDifference;
}

int AudioRingBuffer::addSilentSamples(int silentSamples) {
    // NOTE: This implementation is nearly identical to writeData save for s/memcpy/memset, refer to comments there
    int numWriteSamples = std::min(silentSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();

    if (numWriteSamples > samplesRoomFor) {
        numWriteSamples = samplesRoomFor;

        qCDebug(audio) << qPrintable(DROPPED_SILENT_DEBUG);
    }

    if (_endOfLastWrite + numWriteSamples > _buffer + _bufferLength) {
        int numSamplesToEnd = (_buffer + _bufferLength) - _endOfLastWrite;
        memset(_endOfLastWrite, 0, numSamplesToEnd * sizeof(int16_t));
        memset(_buffer, 0, (numWriteSamples - numSamplesToEnd) * sizeof(int16_t));
    } else {
        memset(_endOfLastWrite, 0, numWriteSamples * sizeof(int16_t));
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, numWriteSamples);

    return numWriteSamples;
}

int16_t* AudioRingBuffer::shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const {
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

float AudioRingBuffer::getFrameLoudness(const int16_t* frameStart) const {
    float loudness = 0.0f;
    const int16_t* sampleAt = frameStart;
    const int16_t* bufferLastAt = _buffer + _bufferLength - 1;

    for (int i = 0; i < _numFrameSamples; ++i) {
        loudness += (float) std::abs(*sampleAt);
        // wrap if necessary
        sampleAt = sampleAt == bufferLastAt ? _buffer : sampleAt + 1;
    }
    loudness /= _numFrameSamples;
    loudness /= AudioConstants::MAX_SAMPLE_VALUE;

    return loudness;
}

float AudioRingBuffer::getFrameLoudness(ConstIterator frameStart) const {
    if (frameStart.isNull()) {
        return 0.0f;
    }
    return getFrameLoudness(&(*frameStart));
}

int AudioRingBuffer::writeSamples(ConstIterator source, int maxSamples) {
    int samplesToCopy = std::min(maxSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (samplesToCopy > samplesRoomFor) {
        // there's not enough room for this write.  erase old data to make room for this new data
        int samplesToDelete = samplesToCopy - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;
        qCDebug(audio) << qPrintable(RING_BUFFER_OVERFLOW_DEBUG);
    }

    int16_t* bufferLast = _buffer + _bufferLength - 1;
    for (int i = 0; i < samplesToCopy; i++) {
        *_endOfLastWrite = *source;
        _endOfLastWrite = (_endOfLastWrite == bufferLast) ? _buffer : _endOfLastWrite + 1;
        ++source;
    }

    return samplesToCopy;
}

int AudioRingBuffer::writeSamplesWithFade(ConstIterator source, int maxSamples, float fade) {
    int samplesToCopy = std::min(maxSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (samplesToCopy > samplesRoomFor) {
        // there's not enough room for this write.  erase old data to make room for this new data
        int samplesToDelete = samplesToCopy - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;
        qCDebug(audio) << qPrintable(RING_BUFFER_OVERFLOW_DEBUG);
    }

    int16_t* bufferLast = _buffer + _bufferLength - 1;
    for (int i = 0; i < samplesToCopy; i++) {
        *_endOfLastWrite = (int16_t)((float)(*source) * fade);
        _endOfLastWrite = (_endOfLastWrite == bufferLast) ? _buffer : _endOfLastWrite + 1;
        ++source;
    }

    return samplesToCopy;
}

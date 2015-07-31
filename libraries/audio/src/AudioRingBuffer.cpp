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

#include "AudioLogging.h"

#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer(int numFrameSamples, bool randomAccessMode, int numFramesCapacity) :
_frameCapacity(numFramesCapacity),
_sampleCapacity(numFrameSamples * numFramesCapacity),
_bufferLength(numFrameSamples * (numFramesCapacity + 1)),
_numFrameSamples(numFrameSamples),
_randomAccessMode(randomAccessMode),
_overflowCount(0)
{
    if (numFrameSamples) {
        _buffer = new int16_t[_bufferLength];
        memset(_buffer, 0, _bufferLength * sizeof(int16_t));
        _nextOutput = _buffer;
        _endOfLastWrite = _buffer;
    } else {
        _buffer = NULL;
        _nextOutput = NULL;
        _endOfLastWrite = NULL;
    }
};

AudioRingBuffer::~AudioRingBuffer() {
    delete[] _buffer;
}

void AudioRingBuffer::reset() {
    clear();
    _overflowCount = 0;
}

void AudioRingBuffer::resizeForFrameSize(int numFrameSamples) {
    delete[] _buffer;
    _sampleCapacity = numFrameSamples * _frameCapacity;
    _bufferLength = numFrameSamples * (_frameCapacity + 1);
    _numFrameSamples = numFrameSamples;
    _buffer = new int16_t[_bufferLength];
    memset(_buffer, 0, _bufferLength * sizeof(int16_t));
    if (_randomAccessMode) {
        memset(_buffer, 0, _bufferLength * sizeof(int16_t));
    }
    reset();
}

void AudioRingBuffer::clear() {
    _endOfLastWrite = _buffer;
    _nextOutput = _buffer;
}

int AudioRingBuffer::readSamples(int16_t* destination, int maxSamples) {
    return readData((char*)destination, maxSamples * sizeof(int16_t)) / sizeof(int16_t);
}

int AudioRingBuffer::readData(char *data, int maxSize) {

    // only copy up to the number of samples we have available
    int numReadSamples = std::min((int)(maxSize / sizeof(int16_t)), samplesAvailable());

    // If we're in random access mode, then we consider our number of available read samples slightly
    // differently. Namely, if anything has been written, we say we have as many samples as they ask for
    // otherwise we say we have nothing available
    if (_randomAccessMode) {
        numReadSamples = _endOfLastWrite ? (maxSize / sizeof(int16_t)) : 0;
    }

    if (_nextOutput + numReadSamples > _buffer + _bufferLength) {
        // we're going to need to do two reads to get this data, it wraps around the edge

        // read to the end of the buffer
        int numSamplesToEnd = (_buffer + _bufferLength) - _nextOutput;
        memcpy(data, _nextOutput, numSamplesToEnd * sizeof(int16_t));
        if (_randomAccessMode) {
            memset(_nextOutput, 0, numSamplesToEnd * sizeof(int16_t)); // clear it
        }

        // read the rest from the beginning of the buffer
        memcpy(data + (numSamplesToEnd * sizeof(int16_t)), _buffer, (numReadSamples - numSamplesToEnd) * sizeof(int16_t));
        if (_randomAccessMode) {
            memset(_buffer, 0, (numReadSamples - numSamplesToEnd) * sizeof(int16_t)); // clear it
        }
    } else {
        // read the data
        memcpy(data, _nextOutput, numReadSamples * sizeof(int16_t));
        if (_randomAccessMode) {
            memset(_nextOutput, 0, numReadSamples * sizeof(int16_t)); // clear it
        }
    }

    // push the position of _nextOutput by the number of samples read
    _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, numReadSamples);

    return numReadSamples * sizeof(int16_t);
}

int AudioRingBuffer::writeSamples(const int16_t* source, int maxSamples) {
    return writeData((const char*)source, maxSamples * sizeof(int16_t)) / sizeof(int16_t);
}

int AudioRingBuffer::writeData(const char* data, int maxSize) {
    // make sure we have enough bytes left for this to be the right amount of audio
    // otherwise we should not copy that data, and leave the buffer pointers where they are
    int samplesToCopy = std::min((int)(maxSize / sizeof(int16_t)), _sampleCapacity);

    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (samplesToCopy > samplesRoomFor) {
        // there's not enough room for this write.  erase old data to make room for this new data
        int samplesToDelete = samplesToCopy - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;
        qCDebug(audio) << "Overflowed ring buffer! Overwriting old data";
    }

    if (_endOfLastWrite + samplesToCopy <= _buffer + _bufferLength) {
        memcpy(_endOfLastWrite, data, samplesToCopy * sizeof(int16_t));
    } else {
        int numSamplesToEnd = (_buffer + _bufferLength) - _endOfLastWrite;
        memcpy(_endOfLastWrite, data, numSamplesToEnd * sizeof(int16_t));
        memcpy(_buffer, data + (numSamplesToEnd * sizeof(int16_t)), (samplesToCopy - numSamplesToEnd) * sizeof(int16_t));
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, samplesToCopy);

    return samplesToCopy * sizeof(int16_t);
}

int16_t& AudioRingBuffer::operator[](const int index) {
    return *shiftedPositionAccomodatingWrap(_nextOutput, index);
}

const int16_t& AudioRingBuffer::operator[] (const int index) const {
    return *shiftedPositionAccomodatingWrap(_nextOutput, index);
}

void AudioRingBuffer::shiftReadPosition(unsigned int numSamples) {
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

    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (silentSamples > samplesRoomFor) {
        // there's not enough room for this write. write as many silent samples as we have room for
        silentSamples = samplesRoomFor;
        qCDebug(audio) << "Dropping some silent samples to prevent ring buffer overflow";
    }

    // memset zeroes into the buffer, accomodate a wrap around the end
    // push the _endOfLastWrite to the correct spot
    if (_endOfLastWrite + silentSamples <= _buffer + _bufferLength) {
        memset(_endOfLastWrite, 0, silentSamples * sizeof(int16_t));
    } else {
        int numSamplesToEnd = (_buffer + _bufferLength) - _endOfLastWrite;
        memset(_endOfLastWrite, 0, numSamplesToEnd * sizeof(int16_t));
        memset(_buffer, 0, (silentSamples - numSamplesToEnd) * sizeof(int16_t));
    }
    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, silentSamples);

    return silentSamples;
}

int16_t* AudioRingBuffer::shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const {

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
    const int16_t* _bufferLastAt = _buffer + _bufferLength - 1;

    for (int i = 0; i < _numFrameSamples; ++i) {
        loudness += (float) std::abs(*sampleAt);
        sampleAt = sampleAt == _bufferLastAt ? _buffer : sampleAt + 1;
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

float AudioRingBuffer::getNextOutputFrameLoudness() const {
    return getFrameLoudness(_nextOutput);
}

int AudioRingBuffer::writeSamples(ConstIterator source, int maxSamples) {
    int samplesToCopy = std::min(maxSamples, _sampleCapacity);
    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (samplesToCopy > samplesRoomFor) {
        // there's not enough room for this write.  erase old data to make room for this new data
        int samplesToDelete = samplesToCopy - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        _overflowCount++;
        qCDebug(audio) << "Overflowed ring buffer! Overwriting old data";
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
        qCDebug(audio) << "Overflowed ring buffer! Overwriting old data";
    }

    int16_t* bufferLast = _buffer + _bufferLength - 1;
    for (int i = 0; i < samplesToCopy; i++) {
        *_endOfLastWrite = (int16_t)((float)(*source) * fade);
        _endOfLastWrite = (_endOfLastWrite == bufferLast) ? _buffer : _endOfLastWrite + 1;
        ++source;
    }

    return samplesToCopy;
}

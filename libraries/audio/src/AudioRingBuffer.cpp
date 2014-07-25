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

#include <cstring>
#include <functional>
#include <math.h>

#include <QtCore/QDebug>

#include "PacketHeaders.h"
#include "AudioRingBuffer.h"


AudioRingBuffer::AudioRingBuffer(int numFrameSamples, bool randomAccessMode, int numFramesCapacity) :
    NodeData(),
    _frameCapacity(numFramesCapacity),
    _sampleCapacity(numFrameSamples * numFramesCapacity),
    _isFull(false),
    _numFrameSamples(numFrameSamples),
    _randomAccessMode(randomAccessMode),
    _overflowCount(0)
{
    if (numFrameSamples) {
        _buffer = new int16_t[_sampleCapacity];
        if (_randomAccessMode) {
            memset(_buffer, 0, _sampleCapacity * sizeof(int16_t));
        }
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
    _overflowCount = 0;
    _isFull = false;
    _endOfLastWrite = _buffer;
    _nextOutput = _buffer;
}

void AudioRingBuffer::resizeForFrameSize(int numFrameSamples) {
    delete[] _buffer;
    _sampleCapacity = numFrameSamples * _frameCapacity;
    _numFrameSamples = numFrameSamples;
    _buffer = new int16_t[_sampleCapacity];
    if (_randomAccessMode) {
        memset(_buffer, 0, _sampleCapacity * sizeof(int16_t));
    }
    reset();
}

int AudioRingBuffer::parseData(const QByteArray& packet) {
    // skip packet header and sequence number
    int numBytesBeforeAudioData = numBytesForPacketHeader(packet) + sizeof(quint16);
    return writeData(packet.data() + numBytesBeforeAudioData, packet.size() - numBytesBeforeAudioData);
}

int AudioRingBuffer::readSamples(int16_t* destination, int maxSamples) {
    return readData((char*) destination, maxSamples * sizeof(int16_t));
}

int AudioRingBuffer::readData(char *data, int maxSize) {

    // only copy up to the number of samples we have available
    int numReadSamples = std::min((int) (maxSize / sizeof(int16_t)), samplesAvailable());

    // If we're in random access mode, then we consider our number of available read samples slightly
    // differently. Namely, if anything has been written, we say we have as many samples as they ask for
    // otherwise we say we have nothing available
    if (_randomAccessMode) {
        numReadSamples = _endOfLastWrite ? (maxSize / sizeof(int16_t)) : 0;
    }

    if (_nextOutput + numReadSamples > _buffer + _sampleCapacity) {
        // we're going to need to do two reads to get this data, it wraps around the edge

        // read to the end of the buffer
        int numSamplesToEnd = (_buffer + _sampleCapacity) - _nextOutput;
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
    if (numReadSamples > 0) {
        _isFull = false;
    }

    return numReadSamples * sizeof(int16_t);
}

int AudioRingBuffer::writeSamples(const int16_t* source, int maxSamples) {    
    return writeData((const char*) source, maxSamples * sizeof(int16_t));
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
        qDebug() << "Overflowed ring buffer! Overwriting old data";
    }
    
    if (_endOfLastWrite + samplesToCopy <= _buffer + _sampleCapacity) {
        memcpy(_endOfLastWrite, data, samplesToCopy * sizeof(int16_t));
    } else {
        int numSamplesToEnd = (_buffer + _sampleCapacity) - _endOfLastWrite;
        memcpy(_endOfLastWrite, data, numSamplesToEnd * sizeof(int16_t));
        memcpy(_buffer, data + (numSamplesToEnd * sizeof(int16_t)), (samplesToCopy - numSamplesToEnd) * sizeof(int16_t));
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, samplesToCopy);
    if (samplesToCopy > 0 && _endOfLastWrite == _nextOutput) {
        _isFull = true;
    }

    return samplesToCopy * sizeof(int16_t);
}

int16_t& AudioRingBuffer::operator[](const int index) {
    return *shiftedPositionAccomodatingWrap(_nextOutput, index);
}

const int16_t& AudioRingBuffer::operator[] (const int index) const {
    return *shiftedPositionAccomodatingWrap(_nextOutput, index);
}

void AudioRingBuffer::shiftReadPosition(unsigned int numSamples) {
    if (numSamples > 0) {
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, numSamples);
        _isFull = false;
    }
}

int AudioRingBuffer::samplesAvailable() const {
    if (!_endOfLastWrite) {
        return 0;
    }
    if (_isFull) {
        return _sampleCapacity;
    }

    int sampleDifference = _endOfLastWrite - _nextOutput;
    if (sampleDifference < 0) {
        sampleDifference += _sampleCapacity;
    }
    return sampleDifference;
}

int AudioRingBuffer::addSilentFrame(int numSilentSamples) {

    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (numSilentSamples > samplesRoomFor) {
        // there's not enough room for this write. write as many silent samples as we have room for
        numSilentSamples = samplesRoomFor;
        qDebug() << "Dropping some silent samples to prevent ring buffer overflow";
    }

    // memset zeroes into the buffer, accomodate a wrap around the end
    // push the _endOfLastWrite to the correct spot
    if (_endOfLastWrite + numSilentSamples <= _buffer + _sampleCapacity) {
        memset(_endOfLastWrite, 0, numSilentSamples * sizeof(int16_t));
    } else {
        int numSamplesToEnd = (_buffer + _sampleCapacity) - _endOfLastWrite;
        memset(_endOfLastWrite, 0, numSamplesToEnd * sizeof(int16_t));
        memset(_buffer, 0, (numSilentSamples - numSamplesToEnd) * sizeof(int16_t));
    }
    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, numSilentSamples);
    if (numSilentSamples > 0 && _nextOutput == _endOfLastWrite) {
        _isFull = true;
    }

    return numSilentSamples * sizeof(int16_t);
}

int16_t* AudioRingBuffer::shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const {

    if (numSamplesShift > 0 && position + numSamplesShift >= _buffer + _sampleCapacity) {
        // this shift will wrap the position around to the beginning of the ring
        return position + numSamplesShift - _sampleCapacity;
    } else if (numSamplesShift < 0 && position + numSamplesShift < _buffer) {
        // this shift will go around to the end of the ring
        return position + numSamplesShift + _sampleCapacity;
    } else {
        return position + numSamplesShift;
    }
}

float AudioRingBuffer::getNextOutputFrameLoudness() const {
    float loudness = 0.0f;
    int16_t* sampleAt = _nextOutput;
    int16_t* _bufferLastAt = _buffer + _sampleCapacity - 1;
    if (samplesAvailable() >= _numFrameSamples) {
        for (int i = 0; i < _numFrameSamples; ++i) {
            loudness += fabsf(*sampleAt);
            sampleAt = sampleAt == _bufferLastAt ? _buffer : sampleAt + 1;
        }
        loudness /= _numFrameSamples;
        loudness /= MAX_SAMPLE_VALUE;
    }
    return loudness;
}

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
#include "../../../assignment-client/src/audio/AudioMixerClientData.h"

#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer(int numFrameSamples, bool randomAccessMode) :
    NodeData(),
    _sampleCapacity(numFrameSamples * RING_BUFFER_LENGTH_FRAMES),
    _numFrameSamples(numFrameSamples),
    _isStarved(true),
    _hasStarted(false),
    _randomAccessMode(randomAccessMode)
{
    _arrayLength = _sampleCapacity + 1;

    if (numFrameSamples) {
        _buffer = new int16_t[_arrayLength];
        if (_randomAccessMode) {
            memset(_buffer, 0, _arrayLength * sizeof(int16_t));
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
    _endOfLastWrite = _buffer;
    _nextOutput = _buffer;
    _isStarved = true;
}

void AudioRingBuffer::resizeForFrameSize(qint64 numFrameSamples) {
    delete[] _buffer;
    _sampleCapacity = numFrameSamples * RING_BUFFER_LENGTH_FRAMES;
    _arrayLength = _sampleCapacity + 1;
    _buffer = new int16_t[_arrayLength];
    if (_randomAccessMode) {
        memset(_buffer, 0, _arrayLength * sizeof(int16_t));
    }
    _nextOutput = _buffer;
    _endOfLastWrite = _buffer;
}

int AudioRingBuffer::parseData(const QByteArray& packet) {
    int numBytesBeforeAudioData = numBytesForPacketHeader(packet);
    if (packetTypeForPacket(packet) == PacketTypeMixedAudio) {
        numBytesBeforeAudioData += sizeof(AudioMixerJitterBuffersStats);
    }
    return writeData(packet.data() + numBytesBeforeAudioData, packet.size() - numBytesBeforeAudioData);
}

qint64 AudioRingBuffer::readSamples(int16_t* destination, qint64 maxSamples) {
    return readData((char*) destination, maxSamples * sizeof(int16_t));
}

qint64 AudioRingBuffer::readData(char *data, qint64 maxSize) {

    // only copy up to the number of samples we have available
    int numReadSamples = std::min((unsigned) (maxSize / sizeof(int16_t)), samplesAvailable());

    // If we're in random access mode, then we consider our number of available read samples slightly
    // differently. Namely, if anything has been written, we say we have as many samples as they ask for
    // otherwise we say we have nothing available
    if (_randomAccessMode) {
        numReadSamples = _endOfLastWrite ? (maxSize / sizeof(int16_t)) : 0;
    }

    if (_nextOutput + numReadSamples > _buffer + _arrayLength) {
        // we're going to need to do two reads to get this data, it wraps around the edge

        // read to the end of the buffer
        int numSamplesToEnd = (_buffer + _arrayLength) - _nextOutput;
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

qint64 AudioRingBuffer::writeSamples(const int16_t* source, qint64 maxSamples) {    
    return writeData((const char*) source, maxSamples * sizeof(int16_t));
}

qint64 AudioRingBuffer::writeData(const char* data, qint64 maxSize) {
    // make sure we have enough bytes left for this to be the right amount of audio
    // otherwise we should not copy that data, and leave the buffer pointers where they are

    int samplesToCopy = std::min((quint64)(maxSize / sizeof(int16_t)), (quint64)_sampleCapacity);
    
    int samplesRoomFor = _sampleCapacity - samplesAvailable();
    if (samplesToCopy > samplesRoomFor) {
        // there's not enough room for this write.  erase old data to make room for this new data
        int samplesToDelete = samplesToCopy - samplesRoomFor;
        _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, samplesToDelete);
        qDebug() << "Overflowed ring buffer! Overwriting old data";
    }
    
    if (_endOfLastWrite + samplesToCopy <= _buffer + _arrayLength) {
        memcpy(_endOfLastWrite, data, samplesToCopy * sizeof(int16_t));
    } else {
        int numSamplesToEnd = (_buffer + _arrayLength) - _endOfLastWrite;
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

unsigned int AudioRingBuffer::samplesAvailable() const {
    if (!_endOfLastWrite) {
        return 0;
    } else {
        int sampleDifference = _endOfLastWrite - _nextOutput;

        if (sampleDifference < 0) {
            sampleDifference += _arrayLength;
        }

        return sampleDifference;
    }
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
    if (_endOfLastWrite + numSilentSamples <= _buffer + _arrayLength) {
        memset(_endOfLastWrite, 0, numSilentSamples * sizeof(int16_t));
    } else {
        int numSamplesToEnd = (_buffer + _arrayLength) - _endOfLastWrite;
        memset(_endOfLastWrite, 0, numSamplesToEnd * sizeof(int16_t));
        memset(_buffer, 0, (numSilentSamples - numSamplesToEnd) * sizeof(int16_t));
    }
    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, numSilentSamples);

    return numSilentSamples * sizeof(int16_t);
}

bool AudioRingBuffer::isNotStarvedOrHasMinimumSamples(unsigned int numRequiredSamples) const {
    if (!_isStarved) {
        return true;
    } else {
        return samplesAvailable() >= numRequiredSamples;
    }
}

int16_t* AudioRingBuffer::shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const {

    if (numSamplesShift > 0 && position + numSamplesShift >= _buffer + _arrayLength) {
        // this shift will wrap the position around to the beginning of the ring
        return position + numSamplesShift - _arrayLength;
    } else if (numSamplesShift < 0 && position + numSamplesShift < _buffer) {
        // this shift will go around to the end of the ring
        return position + numSamplesShift + _arrayLength;
    } else {
        return position + numSamplesShift;
    }
}

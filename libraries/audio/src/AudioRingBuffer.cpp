//
//  AudioRingBuffer.cpp
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>
#include <math.h>

#include <QtCore/QDebug>

#include "PacketHeaders.h"

#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer(int numFrameSamples) :
    NodeData(),
    _sampleCapacity(numFrameSamples * RING_BUFFER_LENGTH_FRAMES),
    _isStarved(true),
    _hasStarted(false),
    _averageLoudness(0)
{
    if (numFrameSamples) {
        _buffer = new int16_t[_sampleCapacity];
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
    _buffer = new int16_t[_sampleCapacity];
    _nextOutput = _buffer;
    _endOfLastWrite = _buffer;
}

int AudioRingBuffer::parseData(const QByteArray& packet) {
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    return writeData(packet.data() + numBytesPacketHeader, packet.size() - numBytesPacketHeader);
}

void AudioRingBuffer::updateAverageLoudnessForBoundarySamples(int numSamples) {
    // ForBoundarySamples means that we expect the number of samples not to roll of the end of the ring buffer
    float nextLoudness = 0;
    
    for (int i = 0; i < numSamples; ++i) {
        nextLoudness += fabsf(_nextOutput[i]);
    }
    
    nextLoudness /= numSamples;
    nextLoudness /= MAX_SAMPLE_VALUE;
    
    const int TRAILING_AVERAGE_FRAMES = 100;
    const float CURRENT_FRAME_RATIO = 1 / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1 - CURRENT_FRAME_RATIO;
    const float LOUDNESS_EPSILON = 0.001;
    
    if (nextLoudness >= _averageLoudness) {
        _averageLoudness = nextLoudness;
    } else {
        _averageLoudness = (_averageLoudness * PREVIOUS_FRAMES_RATIO) + (CURRENT_FRAME_RATIO * nextLoudness);
        
        if (_averageLoudness < LOUDNESS_EPSILON) {
            _averageLoudness = 0;
        }
    }
}

qint64 AudioRingBuffer::readSamples(int16_t* destination, qint64 maxSamples) {
    return readData((char*) destination, maxSamples * sizeof(int16_t));
}

qint64 AudioRingBuffer::readData(char *data, qint64 maxSize) {

    // only copy up to the number of samples we have available
    int numReadSamples = std::min((unsigned) (maxSize / sizeof(int16_t)), samplesAvailable());

    if (_nextOutput + numReadSamples > _buffer + _sampleCapacity) {
        // we're going to need to do two reads to get this data, it wraps around the edge

        // read to the end of the buffer
        int numSamplesToEnd = (_buffer + _sampleCapacity) - _nextOutput;
        memcpy(data, _nextOutput, numSamplesToEnd * sizeof(int16_t));
        
        // read the rest from the beginning of the buffer
        memcpy(data + (numSamplesToEnd * sizeof(int16_t)), _buffer, (numReadSamples - numSamplesToEnd) * sizeof(int16_t));
    } else {
        // read the data
        memcpy(data, _nextOutput, numReadSamples * sizeof(int16_t));
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

    std::less<int16_t*> less;
    std::less_equal<int16_t*> lessEqual;

    if (_hasStarted
        && (less(_endOfLastWrite, _nextOutput)
            && lessEqual(_nextOutput, shiftedPositionAccomodatingWrap(_endOfLastWrite, samplesToCopy)))) {
        // this read will cross the next output, so call us starved and reset the buffer
        qDebug() << "Filled the ring buffer. Resetting.";
        _endOfLastWrite = _buffer;
        _nextOutput = _buffer;
        _isStarved = true;
    }

    if (_endOfLastWrite + samplesToCopy <= _buffer + _sampleCapacity) {
        memcpy(_endOfLastWrite, data, samplesToCopy * sizeof(int16_t));
    } else {
        int numSamplesToEnd = (_buffer + _sampleCapacity) - _endOfLastWrite;
        memcpy(_endOfLastWrite, data, numSamplesToEnd * sizeof(int16_t));
        memcpy(_buffer, data + (numSamplesToEnd * sizeof(int16_t)), (samplesToCopy - numSamplesToEnd) * sizeof(int16_t));
    }

    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, samplesToCopy);

    return samplesToCopy * sizeof(int16_t);
}

int16_t& AudioRingBuffer::operator[](const int index) {
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
            sampleDifference += _sampleCapacity;
        }

        return sampleDifference;
    }
}

void AudioRingBuffer::addSilentFrame(int numSilentSamples) {
    // memset zeroes into the buffer, accomodate a wrap around the end
    // push the _endOfLastWrite to the correct spot
    if (_endOfLastWrite + numSilentSamples <= _buffer + _sampleCapacity) {
        memset(_endOfLastWrite, 0, numSilentSamples * sizeof(int16_t));
        _endOfLastWrite += numSilentSamples;
    } else {
        int numSamplesToEnd = (_buffer + _sampleCapacity) - _endOfLastWrite;
        memset(_endOfLastWrite, 0, numSamplesToEnd * sizeof(int16_t));
        memset(_buffer, 0, (numSilentSamples - numSamplesToEnd) * sizeof(int16_t));
        
        _endOfLastWrite = _buffer + (numSilentSamples - numSamplesToEnd);
    }
}

bool AudioRingBuffer::isNotStarvedOrHasMinimumSamples(unsigned int numRequiredSamples) const {
    if (!_isStarved) {
        return true;
    } else {
        return samplesAvailable() >= numRequiredSamples;
    }
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

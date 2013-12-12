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

AudioRingBuffer::AudioRingBuffer(bool isStereo) :
    NodeData(NULL),
    _endOfLastWrite(NULL),
    _isStarved(true)
{
    _buffer = new int16_t[RING_BUFFER_LENGTH_SAMPLES];
    _nextOutput = _buffer;
};

AudioRingBuffer::~AudioRingBuffer() {
    delete[] _buffer;
}

void AudioRingBuffer::reset() {
    _endOfLastWrite = _buffer;
    _nextOutput = _buffer;
    _isStarved = true;
}

int AudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    int numBytesPacketHeader = numBytesForPacketHeader(sourceBuffer);
    return parseAudioSamples(sourceBuffer + numBytesPacketHeader, numBytes - numBytesPacketHeader);
}

int AudioRingBuffer::parseAudioSamples(unsigned char* sourceBuffer, int numBytes) {
    // make sure we have enough bytes left for this to be the right amount of audio
    // otherwise we should not copy that data, and leave the buffer pointers where they are
    
    int samplesToCopy = numBytes / sizeof(int16_t);
    
    if (!_endOfLastWrite) {
        _endOfLastWrite = _buffer;
    } else if (samplesToCopy > RING_BUFFER_LENGTH_SAMPLES - samplesAvailable()) {
        // this read will cross the next output, so call us starved and reset the buffer
        qDebug() << "Filled the ring buffer. Resetting.\n";
        _endOfLastWrite = _buffer;
        _nextOutput = _buffer;
        _isStarved = true;
    }
    
    if (_endOfLastWrite + samplesToCopy <= _buffer + RING_BUFFER_LENGTH_SAMPLES) {
        memcpy(_endOfLastWrite, sourceBuffer, numBytes);
    } else {
        int numSamplesToEnd = (_buffer + RING_BUFFER_LENGTH_SAMPLES) - _endOfLastWrite;
        memcpy(_endOfLastWrite, sourceBuffer, numSamplesToEnd * sizeof(int16_t));
        memcpy(_buffer, sourceBuffer + (numSamplesToEnd * sizeof(int16_t)), (samplesToCopy - numSamplesToEnd) * sizeof(int16_t));
    }
    
    _endOfLastWrite = shiftedPositionAccomodatingWrap(_endOfLastWrite, samplesToCopy);
    
    return numBytes;
}

int16_t& AudioRingBuffer::operator[](const int index) {
    // make sure this is a valid index
    assert(index > -RING_BUFFER_LENGTH_SAMPLES && index < RING_BUFFER_LENGTH_SAMPLES);
    
    return *shiftedPositionAccomodatingWrap(_nextOutput, index);
}

void AudioRingBuffer::read(int16_t* destination, unsigned int maxSamples) {
    
    // only copy up to the number of samples we have available
    int numReadSamples = std::min(maxSamples, samplesAvailable());
    
    if (_nextOutput + numReadSamples > _buffer + RING_BUFFER_LENGTH_SAMPLES) {
        // we're going to need to do two reads to get this data, it wraps around the edge
        
        // read to the end of the buffer
        int numSamplesToEnd = (_buffer + RING_BUFFER_LENGTH_SAMPLES) - _nextOutput;
        memcpy(destination, _nextOutput, numSamplesToEnd * sizeof(int16_t));
        
        // read the rest from the beginning of the buffer
        memcpy(destination + numSamplesToEnd, _buffer, (numReadSamples - numSamplesToEnd) * sizeof(int16_t));
    } else {
        // read the data
        memcpy(destination, _nextOutput, numReadSamples * sizeof(int16_t));
    }
    
    // push the position of _nextOutput by the number of samples read
    _nextOutput = shiftedPositionAccomodatingWrap(_nextOutput, numReadSamples);
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
            sampleDifference += RING_BUFFER_LENGTH_SAMPLES;
        }
        
        return sampleDifference;
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
    
    if (numSamplesShift > 0 && position + numSamplesShift >= _buffer + RING_BUFFER_LENGTH_SAMPLES) {
        // this shift will wrap the position around to the beginning of the ring
        return position + numSamplesShift - RING_BUFFER_LENGTH_SAMPLES;
    } else if (numSamplesShift < 0 && position + numSamplesShift < _buffer) {
        // this shift will go around to the end of the ring
        return position + numSamplesShift - RING_BUFFER_LENGTH_SAMPLES;
    } else {
        return position + numSamplesShift;
    }
}

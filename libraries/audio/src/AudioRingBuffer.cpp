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
    _isStarved(true),
    _hasStarted(false),
    _isStereo(isStereo)
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
    _hasStarted = false;
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
    } else if (diffLastWriteNextOutput() > RING_BUFFER_LENGTH_SAMPLES - samplesToCopy) {
        _endOfLastWrite = _buffer;
        _nextOutput = _buffer;
        _isStarved = true;
    }
    
    memcpy(_endOfLastWrite, sourceBuffer, numBytes);
    
    _endOfLastWrite += samplesToCopy;
    
    if (_endOfLastWrite >= _buffer + RING_BUFFER_LENGTH_SAMPLES) {
        _endOfLastWrite = _buffer;
    }
    
    return numBytes;
}

int AudioRingBuffer::diffLastWriteNextOutput() const {
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

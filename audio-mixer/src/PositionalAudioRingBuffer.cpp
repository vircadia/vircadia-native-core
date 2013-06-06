//
//  PositionalAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PacketHeaders.h>

#include "PositionalAudioRingBuffer.h"

PositionalAudioRingBuffer::PositionalAudioRingBuffer() :
    AudioRingBuffer(false),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(0.0f, 0.0f, 0.0f, 0.0f),
    _shouldLoopbackForAgent(false),
    _willBeAddedToMix(false)
{
    
}

int PositionalAudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    unsigned char* currentBuffer = sourceBuffer + sizeof(PACKET_HEADER);
    currentBuffer += parsePositionalData(currentBuffer, numBytes - (currentBuffer - sourceBuffer));
    currentBuffer += parseAudioSamples(currentBuffer, numBytes - (currentBuffer - sourceBuffer));
    
    return currentBuffer - sourceBuffer;
}

int PositionalAudioRingBuffer::parsePositionalData(unsigned char* sourceBuffer, int numBytes) {
    unsigned char* currentBuffer = sourceBuffer;
    
    memcpy(&_position, currentBuffer, sizeof(_position));
    currentBuffer += sizeof(_position);
    
    memcpy(&_orientation, currentBuffer, sizeof(_orientation));
    currentBuffer += sizeof(_orientation);
    
    // if this agent sent us a NaN for first float in orientation then don't consider this good audio and bail
    if (std::isnan(_orientation.x)) {
        _endOfLastWrite = _nextOutput = _buffer;
        _isStarted = false;
        return 0;
    }
    
    return currentBuffer - sourceBuffer;
}

bool PositionalAudioRingBuffer::shouldBeAddedToMix(int numJitterBufferSamples) {
    if (_endOfLastWrite) {
        if (!_isStarted && diffLastWriteNextOutput() <= BUFFER_LENGTH_SAMPLES_PER_CHANNEL + numJitterBufferSamples) {
            printf("Buffer held back\n");
            return false;
        } else if (diffLastWriteNextOutput() < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
            printf("Buffer starved.\n");
            _isStarted = false;
            return false;
        } else {
            // good buffer, add this to the mix
            _isStarted = true;
            return true;
        }
    }
    
    return false;
}
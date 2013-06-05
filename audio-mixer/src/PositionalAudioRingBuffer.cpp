//
//  PositionalAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PositionalAudioRingBuffer.h"

PositionalAudioRingBuffer::PositionalAudioRingBuffer() :
    AudioRingBuffer(false),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(0.0f, 0.0f, 0.0f, 0.0f),
    _shouldLoopbackForAgent(false),
    _wasAddedToMix(false)
{
    
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
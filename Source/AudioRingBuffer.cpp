//
//  AudioRingBuffer.cpp
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer(short ringBufferSamples) {
    ringBufferLengthSamples = ringBufferSamples;
    started = false;
    
    endOfLastWrite = NULL;
    
    buffer = new int16_t[ringBufferLengthSamples];
    nextOutput = buffer;
};

AudioRingBuffer::~AudioRingBuffer() {
    delete[] buffer;
};

short AudioRingBuffer::diffLastWriteNextOutput()
{
    if (endOfLastWrite == NULL) {
        return 0;
    } else {
        short sampleDifference = endOfLastWrite - nextOutput;
        
        if (sampleDifference < 0) {
            sampleDifference += ringBufferLengthSamples;
        }
        
        return sampleDifference;
    }
}

short AudioRingBuffer::bufferOverlap(int16_t *pointer, short addedDistance)
{
    short samplesLeft = (buffer + ringBufferLengthSamples) - pointer;
    
    if (samplesLeft < addedDistance) {
        return addedDistance - samplesLeft;
    } else {
        return 0;
    }
}

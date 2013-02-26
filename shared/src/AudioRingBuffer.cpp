//
//  AudioRingBuffer.cpp
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>
#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer() {
    started = false;
    addedToMix = false;
    
    endOfLastWrite = NULL;
    
    buffer = new int16_t[RING_BUFFER_SAMPLES];
    nextOutput = buffer;
};

AudioRingBuffer::AudioRingBuffer(const AudioRingBuffer &otherRingBuffer) {
    started = otherRingBuffer.started;
    addedToMix = otherRingBuffer.addedToMix;
    
    buffer = new int16_t[RING_BUFFER_SAMPLES];
    memcpy(buffer, otherRingBuffer.buffer, sizeof(int16_t) * RING_BUFFER_SAMPLES);
    
    nextOutput = buffer + (otherRingBuffer.nextOutput - otherRingBuffer.buffer);
    endOfLastWrite = buffer + (otherRingBuffer.endOfLastWrite - otherRingBuffer.buffer);
}

AudioRingBuffer::~AudioRingBuffer() {
    delete[] buffer;
};

AudioRingBuffer* AudioRingBuffer::clone() const {
    return new AudioRingBuffer(*this);
}

int16_t* AudioRingBuffer::getNextOutput() {
    return nextOutput;
}

void AudioRingBuffer::setNextOutput(int16_t *newPointer) {
    nextOutput = newPointer;
}

int16_t* AudioRingBuffer::getEndOfLastWrite() {
    return endOfLastWrite;
}

void AudioRingBuffer::setEndOfLastWrite(int16_t *newPointer) {
    endOfLastWrite = newPointer;
}

int16_t* AudioRingBuffer::getBuffer() {
    return buffer;
}

bool AudioRingBuffer::isStarted() {
    return started;
}

void AudioRingBuffer::setStarted(bool status) {
    started = status;
}

bool AudioRingBuffer::wasAddedToMix() {
    return addedToMix;
}

void AudioRingBuffer::setAddedToMix(bool added) {
    addedToMix = added;
}

void AudioRingBuffer::parseData(void *data, int size) {
    int16_t *audioData = (int16_t *)data;
    
    if (endOfLastWrite == NULL) {
        endOfLastWrite = buffer;
    } else if (diffLastWriteNextOutput() > RING_BUFFER_SAMPLES - BUFFER_LENGTH_SAMPLES) {
        endOfLastWrite = buffer;
        nextOutput = buffer;
        started = false;
    }
    
    memcpy(endOfLastWrite, audioData, BUFFER_LENGTH_BYTES);
    endOfLastWrite += BUFFER_LENGTH_SAMPLES;
    
    if (endOfLastWrite >= buffer + RING_BUFFER_SAMPLES) {
        endOfLastWrite = buffer;
    }    
}

short AudioRingBuffer::diffLastWriteNextOutput()
{
    if (endOfLastWrite == NULL) {
        return 0;
    } else {
        short sampleDifference = endOfLastWrite - nextOutput;
        
        if (sampleDifference < 0) {
            sampleDifference += RING_BUFFER_SAMPLES;
        }
        
        return sampleDifference;
    }
}

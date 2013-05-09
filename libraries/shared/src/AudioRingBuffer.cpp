//
//  AudioRingBuffer.cpp
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>
#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer(int ringSamples, int bufferSamples) :
    _ringBufferLengthSamples(ringSamples),
    _bufferLengthSamples(bufferSamples),
    endOfLastWrite(NULL),
    started(false),
    _shouldBeAddedToMix(false),
    _shouldLoopbackForAgent(false) {
    buffer = new int16_t[_ringBufferLengthSamples];
    nextOutput = buffer;
};

AudioRingBuffer::AudioRingBuffer(const AudioRingBuffer &otherRingBuffer) {
    _ringBufferLengthSamples = otherRingBuffer._ringBufferLengthSamples;
    _bufferLengthSamples = otherRingBuffer._bufferLengthSamples;
    started = otherRingBuffer.started;
    _shouldBeAddedToMix = otherRingBuffer._shouldBeAddedToMix;
    _shouldLoopbackForAgent = otherRingBuffer._shouldLoopbackForAgent;
    
    buffer = new int16_t[_ringBufferLengthSamples];
    memcpy(buffer, otherRingBuffer.buffer, sizeof(int16_t) * _ringBufferLengthSamples);
    
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

float* AudioRingBuffer::getPosition() {
    return position;
}

void AudioRingBuffer::setPosition(float *newPosition) {
    position[0] = newPosition[0];
    position[1] = newPosition[1];
    position[2] = newPosition[2];
}

float AudioRingBuffer::getAttenuationRatio() {
    return attenuationRatio;
}

void AudioRingBuffer::setAttenuationRatio(float newAttenuation) {
    attenuationRatio = newAttenuation;
}

float AudioRingBuffer::getBearing() {
    return bearing;
}

void AudioRingBuffer::setBearing(float newBearing) {
    bearing = newBearing;
}

const int AGENT_LOOPBACK_MODIFIER = 307;

int AudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    if (numBytes > (_bufferLengthSamples * sizeof(int16_t))) {
        
        unsigned char *dataPtr = sourceBuffer + 1;
        
        for (int p = 0; p < 3; p ++) {
            memcpy(&position[p], dataPtr, sizeof(float));
            dataPtr += sizeof(float);
        }
        
        unsigned int attenuationByte = *(dataPtr++);
        attenuationRatio = attenuationByte / 255.0f;
        
        memcpy(&bearing, dataPtr, sizeof(float));
        dataPtr += sizeof(bearing);
        
        if (bearing > 180 || bearing < -180) {
            // we were passed an invalid bearing because this agent wants loopback (pressed the H key)
            _shouldLoopbackForAgent = true;
            
            // correct the bearing
            bearing = bearing  > 0
                ? bearing - AGENT_LOOPBACK_MODIFIER
                : bearing + AGENT_LOOPBACK_MODIFIER;
        } else {
            _shouldLoopbackForAgent = false;
        }        
        
        sourceBuffer = dataPtr;
    }

    if (endOfLastWrite == NULL) {
        endOfLastWrite = buffer;
    } else if (diffLastWriteNextOutput() > _ringBufferLengthSamples - _bufferLengthSamples) {
        endOfLastWrite = buffer;
        nextOutput = buffer;
        started = false;
    }
    
    memcpy(endOfLastWrite, sourceBuffer, _bufferLengthSamples * sizeof(int16_t));
    
    endOfLastWrite += _bufferLengthSamples;
    
    if (endOfLastWrite >= buffer + _ringBufferLengthSamples) {
        endOfLastWrite = buffer;
    }
    
    return numBytes;
}

short AudioRingBuffer::diffLastWriteNextOutput() {
    if (endOfLastWrite == NULL) {
        return 0;
    } else {
        short sampleDifference = endOfLastWrite - nextOutput;
        
        if (sampleDifference < 0) {
            sampleDifference += _ringBufferLengthSamples;
        }
        
        return sampleDifference;
    }
}

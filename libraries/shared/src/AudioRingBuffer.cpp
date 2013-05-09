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
    _endOfLastWrite(NULL),
    _started(false),
    _shouldBeAddedToMix(false),
    _shouldLoopbackForAgent(false) {
        
    _buffer = new int16_t[_ringBufferLengthSamples];
    _nextOutput = _buffer;
};

AudioRingBuffer::AudioRingBuffer(const AudioRingBuffer &otherRingBuffer) {
    _ringBufferLengthSamples = otherRingBuffer._ringBufferLengthSamples;
    _bufferLengthSamples = otherRingBuffer._bufferLengthSamples;
    _started = otherRingBuffer._started;
    _shouldBeAddedToMix = otherRingBuffer._shouldBeAddedToMix;
    _shouldLoopbackForAgent = otherRingBuffer._shouldLoopbackForAgent;
    
    _buffer = new int16_t[_ringBufferLengthSamples];
    memcpy(_buffer, otherRingBuffer._buffer, sizeof(int16_t) * _ringBufferLengthSamples);
    
    _nextOutput = _buffer + (otherRingBuffer._nextOutput - otherRingBuffer._buffer);
    _endOfLastWrite = _buffer + (otherRingBuffer._endOfLastWrite - otherRingBuffer._buffer);
}

AudioRingBuffer::~AudioRingBuffer() {
    delete[] _buffer;
};

AudioRingBuffer* AudioRingBuffer::clone() const {
    return new AudioRingBuffer(*this);
}

const int AGENT_LOOPBACK_MODIFIER = 307;

int AudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    if (numBytes > (_bufferLengthSamples * sizeof(int16_t))) {
        
        unsigned char *dataPtr = sourceBuffer + 1;
        
        memcpy(&_position, dataPtr, sizeof(_position.x) * 3);
        dataPtr += (sizeof(_position.x) * 3);
        
        unsigned int attenuationByte = *(dataPtr++);
        _attenuationRatio = attenuationByte / 255.0f;
        
        memcpy(&_bearing, dataPtr, sizeof(float));
        dataPtr += sizeof(_bearing);
        
        if (_bearing > 180 || _bearing < -180) {
            // we were passed an invalid bearing because this agent wants loopback (pressed the H key)
            _shouldLoopbackForAgent = true;
            
            // correct the bearing
            _bearing = _bearing  > 0
                ? _bearing - AGENT_LOOPBACK_MODIFIER
                : _bearing + AGENT_LOOPBACK_MODIFIER;
        } else {
            _shouldLoopbackForAgent = false;
        }        
        
        sourceBuffer = dataPtr;
    }

    if (_endOfLastWrite == NULL) {
        _endOfLastWrite = _buffer;
    } else if (diffLastWriteNextOutput() > _ringBufferLengthSamples - _bufferLengthSamples) {
        _endOfLastWrite = _buffer;
        _nextOutput = _buffer;
        _started = false;
    }
    
    memcpy(_endOfLastWrite, sourceBuffer, _bufferLengthSamples * sizeof(int16_t));
    
    _endOfLastWrite += _bufferLengthSamples;
    
    if (_endOfLastWrite >= _buffer + _ringBufferLengthSamples) {
        _endOfLastWrite = _buffer;
    }
    
    return numBytes;
}

short AudioRingBuffer::diffLastWriteNextOutput() {
    if (_endOfLastWrite == NULL) {
        return 0;
    } else {
        short sampleDifference = _endOfLastWrite - _nextOutput;
        
        if (sampleDifference < 0) {
            sampleDifference += _ringBufferLengthSamples;
        }
        
        return sampleDifference;
    }
}

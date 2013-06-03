//
//  AudioRingBuffer.cpp
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>
#include <math.h>

#include "PacketHeaders.h"

#include "AudioRingBuffer.h"

AudioRingBuffer::AudioRingBuffer(int ringSamples, int bufferSamples) :
    AgentData(NULL),
    _ringBufferLengthSamples(ringSamples),
    _bufferLengthSamples(bufferSamples),
    _cubeSideLength(0.0f),
    _endOfLastWrite(NULL),
    _started(false),
    _shouldBeAddedToMix(false),
    _shouldLoopbackForAgent(false),
    _streamIdentifier()
{
    _buffer = new int16_t[_ringBufferLengthSamples];
    _nextOutput = _buffer;
};

AudioRingBuffer::~AudioRingBuffer() {
    delete[] _buffer;
};

const int AGENT_LOOPBACK_MODIFIER = 307;

int AudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    
    unsigned char* dataBuffer = sourceBuffer + 1;
    
    if (sourceBuffer[0] == PACKET_HEADER_INJECT_AUDIO ||
        sourceBuffer[0] == PACKET_HEADER_MICROPHONE_AUDIO) {
        // if this came from an injector or interface client
        // there's data required for spatialization to pull out
        
        if (sourceBuffer[0] == PACKET_HEADER_INJECT_AUDIO) {
            // we've got a stream identifier to pull from the packet
            memcpy(&_streamIdentifier, dataBuffer, sizeof(_streamIdentifier));
            dataBuffer += sizeof(_streamIdentifier);
            
            // push past the injection command
            dataBuffer += sizeof(INJECT_AUDIO_AT_POINT_COMMAND);
        }
        
        memcpy(&_position, dataBuffer, sizeof(_position));
        dataBuffer += (sizeof(_position));
        
        if (sourceBuffer[0] == PACKET_HEADER_INJECT_AUDIO && sourceBuffer[1] == INJECT_AUDIO_AT_CUBE_COMMAND) {
            // this is audio that needs to be injected as a volume (cube)
            // parse out the cubeHalfHeight sent by the client
            memcpy(&_cubeSideLength, dataBuffer, sizeof(_cubeSideLength));
            dataBuffer += (sizeof(_cubeSideLength));
        }
        
        unsigned int attenuationByte = *(dataBuffer++);
        _attenuationRatio = attenuationByte / 255.0f;
        
        memcpy(&_bearing, dataBuffer, sizeof(float));
        dataBuffer += sizeof(_bearing);
        
        // if this agent sent us a NaN bearing then don't consider this good audio and bail
        if (std::isnan(_bearing)) {
            _endOfLastWrite = _nextOutput = _buffer;
            _started = false;
            return 0;
        } else if (_bearing > 180 || _bearing < -180) {
            // we were passed an invalid bearing because this agent wants loopback (pressed the H key)
            _shouldLoopbackForAgent = true;
            
            // correct the bearing
            _bearing = _bearing  > 0
                ? _bearing - AGENT_LOOPBACK_MODIFIER
                : _bearing + AGENT_LOOPBACK_MODIFIER;
        } else {
            _shouldLoopbackForAgent = false;
        }
    }
    
    // make sure we have enough bytes left for this to be the right amount of audio
    // otherwise we should not copy that data, and leave the buffer pointers where they are
    if (numBytes - (dataBuffer - sourceBuffer) == _bufferLengthSamples * sizeof(int16_t)) {
        if (!_endOfLastWrite) {
            _endOfLastWrite = _buffer;
        } else if (diffLastWriteNextOutput() > _ringBufferLengthSamples - _bufferLengthSamples) {
            _endOfLastWrite = _buffer;
            _nextOutput = _buffer;
            _started = false;
        }
        
        memcpy(_endOfLastWrite, dataBuffer, _bufferLengthSamples * sizeof(int16_t));
        
        _endOfLastWrite += _bufferLengthSamples;
        
        if (_endOfLastWrite >= _buffer + _ringBufferLengthSamples) {
            _endOfLastWrite = _buffer;
        }
    }
        
    return numBytes;
}

short AudioRingBuffer::diffLastWriteNextOutput() {
    if (!_endOfLastWrite) {
        return 0;
    } else {
        short sampleDifference = _endOfLastWrite - _nextOutput;
        
        if (sampleDifference < 0) {
            sampleDifference += _ringBufferLengthSamples;
        }
        
        return sampleDifference;
    }
}

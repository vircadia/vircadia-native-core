//
//  InjectedAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>

#include <PacketHeaders.h>

#include "InjectedAudioRingBuffer.h"

InjectedAudioRingBuffer::InjectedAudioRingBuffer() :
    _radius(0.0f),
    _attenuationRatio(0),
    _streamIdentifier()
{
    
}

int InjectedAudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    unsigned char* currentBuffer =  sourceBuffer + numBytesForPacketHeader(sourceBuffer);
    
    // pull stream identifier from the packet
    memcpy(&_streamIdentifier, currentBuffer, sizeof(_streamIdentifier));
    currentBuffer += sizeof(_streamIdentifier);
    
    // use parsePositionalData in parent PostionalAudioRingBuffer class to pull common positional data
    currentBuffer += parsePositionalData(currentBuffer, numBytes - (currentBuffer - sourceBuffer));
    
    // pull out the radius for this injected source - if it's zero this is a point source
    memcpy(&_radius, currentBuffer, sizeof(_radius));
    currentBuffer += sizeof(_radius);
    
    unsigned int attenuationByte = *(currentBuffer++);
    _attenuationRatio = attenuationByte / (float) MAX_INJECTOR_VOLUME;
    
    currentBuffer += parseAudioSamples(currentBuffer, numBytes - (currentBuffer - sourceBuffer));
    
    return currentBuffer - sourceBuffer;
}

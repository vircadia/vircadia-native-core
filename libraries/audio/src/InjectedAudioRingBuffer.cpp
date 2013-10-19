//
//  InjectedAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>

#include <PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioRingBuffer.h"

InjectedAudioRingBuffer::InjectedAudioRingBuffer(const QUuid& streamIdentifier) :
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Microphone),
    _streamIdentifier(streamIdentifier),
    _radius(0.0f),
    _attenuationRatio(0)
{
    
}

int InjectedAudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    unsigned char* currentBuffer =  sourceBuffer + numBytesForPacketHeader(sourceBuffer);
    
    // push past the UUID for this injector
    currentBuffer += NUM_BYTES_RFC4122_UUID;
    
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

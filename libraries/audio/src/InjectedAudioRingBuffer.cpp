//
//  InjectedAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>

#include <QtCore/qdebug.h>

#include <PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioRingBuffer.h"

InjectedAudioRingBuffer::InjectedAudioRingBuffer(const QUuid& streamIdentifier) :
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Injector),
    _streamIdentifier(streamIdentifier),
    _radius(0.0f),
    _attenuationRatio(0)
{
    
}

const uchar MAX_INJECTOR_VOLUME = 255;

int InjectedAudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    unsigned char* currentBuffer =  sourceBuffer + numBytesForPacketHeader(sourceBuffer);
    
    // push past the UUID for this node and the stream identifier
    currentBuffer += (NUM_BYTES_RFC4122_UUID * 2);
    
    // pull the loopback flag and set our boolean
    uchar shouldLoopback;
    memcpy(&shouldLoopback, currentBuffer, sizeof(shouldLoopback));
    currentBuffer += sizeof(shouldLoopback);
    _shouldLoopbackForNode = (shouldLoopback == 1);
    
    // use parsePositionalData in parent PostionalAudioRingBuffer class to pull common positional data
    currentBuffer += parsePositionalData(currentBuffer, numBytes - (currentBuffer - sourceBuffer));
    
    // pull out the radius for this injected source - if it's zero this is a point source
    memcpy(&_radius, currentBuffer, sizeof(_radius));
    currentBuffer += sizeof(_radius);
    
    unsigned int attenuationByte = *(currentBuffer++);
    _attenuationRatio = attenuationByte / (float) MAX_INJECTOR_VOLUME;
    
    currentBuffer += writeData((char*) currentBuffer, numBytes - (currentBuffer - sourceBuffer));
    
    return currentBuffer - sourceBuffer;
}

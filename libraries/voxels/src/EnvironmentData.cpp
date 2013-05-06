//
//  EnvironmentData.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cstring>

#include "EnvironmentData.h"
#include "PacketHeaders.h"

int EnvironmentData::getBroadcastData(unsigned char* destinationBuffer) const {
    unsigned char* bufferStart = destinationBuffer;
    
    memcpy(destinationBuffer, &_atmosphereCenter, sizeof(_atmosphereCenter));
    destinationBuffer += sizeof(_atmosphereCenter);

    memcpy(destinationBuffer, &_atmosphereInnerRadius, sizeof(_atmosphereInnerRadius));
    destinationBuffer += sizeof(_atmosphereInnerRadius);
    
    memcpy(destinationBuffer, &_atmosphereOuterRadius, sizeof(_atmosphereOuterRadius));
    destinationBuffer += sizeof(_atmosphereOuterRadius);
    
    memcpy(destinationBuffer, &_rayleighScattering, sizeof(_rayleighScattering));
    destinationBuffer += sizeof(_rayleighScattering);
    
    memcpy(destinationBuffer, &_mieScattering, sizeof(_mieScattering));
    destinationBuffer += sizeof(_mieScattering);
    
    memcpy(destinationBuffer, &_sunLocation, sizeof(_sunLocation));
    destinationBuffer += sizeof(_sunLocation);
    
    memcpy(destinationBuffer, &_sunBrightness, sizeof(_sunBrightness));
    destinationBuffer += sizeof(_sunBrightness);
    
    return destinationBuffer - bufferStart;
}

int EnvironmentData::parseData(unsigned char* sourceBuffer, int numBytes) {
    // increment to push past the packet header
    sourceBuffer++;
    
    unsigned char* startPosition = sourceBuffer;
    
    memcpy(&_atmosphereCenter, sourceBuffer, sizeof(_atmosphereCenter));
    sourceBuffer += sizeof(_atmosphereCenter);
    
    memcpy(&_atmosphereInnerRadius, sourceBuffer, sizeof(_atmosphereInnerRadius));
    sourceBuffer += sizeof(_atmosphereInnerRadius);
    
    memcpy(&_atmosphereOuterRadius, sourceBuffer, sizeof(_atmosphereOuterRadius));
    sourceBuffer += sizeof(_atmosphereOuterRadius);
    
    memcpy(&_rayleighScattering, sourceBuffer, sizeof(_rayleighScattering));
    sourceBuffer += sizeof(_rayleighScattering);
    
    memcpy(&_mieScattering, sourceBuffer, sizeof(_mieScattering));
    sourceBuffer += sizeof(_mieScattering);
    
    memcpy(&_sunLocation, sourceBuffer, sizeof(_sunLocation));
    sourceBuffer += sizeof(_sunLocation);
    
    memcpy(&_sunBrightness, sourceBuffer, sizeof(_sunBrightness));
    sourceBuffer += sizeof(_sunBrightness);
    
    return sourceBuffer - startPosition;
}


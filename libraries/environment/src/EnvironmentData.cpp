//
//  EnvironmentData.cpp
//  libraries/environment/src
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>

#include "EnvironmentData.h"

// initial values from Sean O'Neil's GPU Gems entry (http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html),
// GameEngine.cpp
EnvironmentData::EnvironmentData(int id) :
    _id(id),
    _flat(true),
    _gravity(0.0f),
    _atmosphereCenter(0, -1000, 0),
    _atmosphereInnerRadius(1000.0),
    _atmosphereOuterRadius(1025.0),
    _rayleighScattering(0.0025f),
    _mieScattering(0.0010f),
    _scatteringWavelengths(0.650f, 0.570f, 0.475f),
    _sunLocation(1000, 900, 1000),
    _sunBrightness(20.0f),
    _hasStars(true) {
}

glm::vec3 EnvironmentData::getAtmosphereCenter(const glm::vec3& cameraPosition) const {
    return _atmosphereCenter + (_flat ? glm::vec3(cameraPosition.x, 0.0f, cameraPosition.z) : glm::vec3());
}

glm::vec3 EnvironmentData::getSunLocation(const glm::vec3& cameraPosition) const {
    return _sunLocation;
}

int EnvironmentData::getBroadcastData(unsigned char* destinationBuffer) const {
    unsigned char* bufferStart = destinationBuffer;
    
    memcpy(destinationBuffer, &_id, sizeof(_id));
    destinationBuffer += sizeof(_id);
    
    memcpy(destinationBuffer, &_flat, sizeof(_flat));
    destinationBuffer += sizeof(_flat);
    
    memcpy(destinationBuffer, &_gravity, sizeof(_gravity));
    destinationBuffer += sizeof(_gravity);
    
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
    
    memcpy(destinationBuffer, &_scatteringWavelengths, sizeof(_scatteringWavelengths));
    destinationBuffer += sizeof(_scatteringWavelengths);
    
    memcpy(destinationBuffer, &_sunLocation, sizeof(_sunLocation));
    destinationBuffer += sizeof(_sunLocation);
    
    memcpy(destinationBuffer, &_sunBrightness, sizeof(_sunBrightness));
    destinationBuffer += sizeof(_sunBrightness);
    
    return destinationBuffer - bufferStart;
}

int EnvironmentData::parseData(const unsigned char* sourceBuffer, int numBytes) {
    const unsigned char* startPosition = sourceBuffer;
    
    memcpy(&_id, sourceBuffer, sizeof(_id));
    sourceBuffer += sizeof(_id);
    
    memcpy(&_flat, sourceBuffer, sizeof(_flat));
    sourceBuffer += sizeof(_flat);
    
    memcpy(&_gravity, sourceBuffer, sizeof(_gravity));
    sourceBuffer += sizeof(_gravity);
    
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
    
    memcpy(&_scatteringWavelengths, sourceBuffer, sizeof(_scatteringWavelengths));
    sourceBuffer += sizeof(_scatteringWavelengths);
    
    memcpy(&_sunLocation, sourceBuffer, sizeof(_sunLocation));
    sourceBuffer += sizeof(_sunLocation);
    
    memcpy(&_sunBrightness, sourceBuffer, sizeof(_sunBrightness));
    sourceBuffer += sizeof(_sunBrightness);
    
    return sourceBuffer - startPosition;
}


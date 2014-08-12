//
//  Referential.cpp
//
//
//  Created by Clement on 7/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <GLMHelpers.h>

#include "AvatarData.h"
#include "Referential.h"

Referential::Referential(Type type, AvatarData* avatar) :
    _type(type),
    _version(0),
    _isValid(true),
    _avatar(avatar)
{
    if (_avatar == NULL) {
        _isValid = false;
        return;
    }
    if (_avatar->hasReferential()) {
        _version = _avatar->getReferential()->version() + 1;
    }
}

Referential::Referential(const unsigned char*& sourceBuffer, AvatarData* avatar) :
    _isValid(false),
    _avatar(avatar)
{
    // Since we can't return how many byte have been read
    // We take a reference to the pointer as argument and increment the pointer ouself.
    sourceBuffer += unpackReferential(sourceBuffer);
    // The actual unpacking to the right referential type happens in Avatar::simulate()
    // If subclassed, make sure to add a case there to unpack the new referential type correctly
}

Referential::~Referential() {
}

int Referential::packReferential(unsigned char* destinationBuffer) const {
    const unsigned char* startPosition = destinationBuffer;
    destinationBuffer += pack(destinationBuffer);
    
    unsigned char* sizePosition = destinationBuffer++; // Save a spot for the extra data size
    char size = packExtraData(destinationBuffer);
    *sizePosition = size; // write extra data size in saved spot here
    destinationBuffer += size;
    
    return destinationBuffer - startPosition;
}

int Referential::unpackReferential(const unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;
    sourceBuffer += unpack(sourceBuffer);
    
    char expectedSize = *sourceBuffer++;
    char bytesRead = unpackExtraData(sourceBuffer, expectedSize);
    _isValid = (bytesRead == expectedSize);
    if (!_isValid) {
        // Will occur if the new instance unpacking is of the wrong type
        qDebug() << "[ERROR] Referential extra data overflow";
    }
    sourceBuffer += expectedSize;
    
    return sourceBuffer - startPosition;
}

int Referential::pack(unsigned char* destinationBuffer) const {
    unsigned char* startPosition = destinationBuffer;
    *destinationBuffer++ = (unsigned char)_type;
    memcpy(destinationBuffer, &_version, sizeof(_version));
    destinationBuffer += sizeof(_version);
    
    destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, _translation, 0);
    destinationBuffer += packOrientationQuatToBytes(destinationBuffer, _rotation);
    destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, _scale, 0);
    return destinationBuffer - startPosition;
}

int Referential::unpack(const unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;
    _type = (Type)*sourceBuffer++;
    if (_type < 0 || _type >= NUM_TYPE) {
        _type = UNKNOWN;
    }
    memcpy(&_version, sourceBuffer, sizeof(_version));
    sourceBuffer += sizeof(_version);
    
    sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, _translation, 0);
    sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, _rotation);
    sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((const int16_t*) sourceBuffer, &_scale, 0);
    return sourceBuffer - startPosition;
}

int Referential::packExtraData(unsigned char *destinationBuffer) const {
    // Since we can't interpret that data, just store it in a buffer for later use.
    memcpy(destinationBuffer, _extraDataBuffer.data(), _extraDataBuffer.size());
    return _extraDataBuffer.size();
}


int Referential::unpackExtraData(const unsigned char* sourceBuffer, int size) {
    _extraDataBuffer.clear();
    _extraDataBuffer.setRawData(reinterpret_cast<const char*>(sourceBuffer), size);
    return size;
}


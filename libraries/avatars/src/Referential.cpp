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

#include <SharedUtil.h>

#include "Referential.h"

Referential::Referential(Type type, AvatarData* avatar) :
    _type(type),
    _createdAt(usecTimestampNow()),
    _isValid(true),
    _avatar(avatar)
{
    if (_avatar == NULL) {
        _isValid = false;
        return;
    }
    qDebug() << "[DEBUG] New Referential";
}

Referential::Referential(const unsigned char*& sourceBuffer, AvatarData* avatar) :
    _isValid(false),
    _avatar(avatar)
{
    sourceBuffer += unpack(sourceBuffer);
}

Referential::~Referential() {
}

int Referential::packReferential(unsigned char* destinationBuffer) {
    const unsigned char* startPosition = destinationBuffer;
    destinationBuffer += pack(destinationBuffer);
    
    unsigned char* sizePosition = destinationBuffer++;
    char size = packExtraData(destinationBuffer);
    *sizePosition = size;
    destinationBuffer += size;
    
    return destinationBuffer - startPosition;
}

int Referential::unpackReferential(const unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;
    sourceBuffer += unpack(sourceBuffer);
    
    char expectedSize = *sourceBuffer++;
    char bytesRead = unpackExtraData(sourceBuffer);
    _isValid = (bytesRead == expectedSize);
    sourceBuffer += expectedSize;
    
    return sourceBuffer - startPosition;
}

int Referential::pack(unsigned char* destinationBuffer) {
    unsigned char* startPosition = destinationBuffer;
    *destinationBuffer++ = (unsigned char)_type;
    memcpy(destinationBuffer, &_createdAt, sizeof(_createdAt));
    
    destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, _translation, 0);
    destinationBuffer += packOrientationQuatToBytes(destinationBuffer, _rotation);
    destinationBuffer += packFloatScalarToSignedTwoByteFixed(destinationBuffer, _scale, 0);
    return destinationBuffer - startPosition;
}

int Referential::unpack(const unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;
    _type = (Type)*sourceBuffer++;
    memcpy(&_createdAt, sourceBuffer, sizeof(_createdAt));
    sourceBuffer += sizeof(_createdAt);
    
    sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, _translation, 0);
    sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, _rotation);
    sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((const int16_t*) sourceBuffer, &_scale, 0);
    return sourceBuffer - startPosition;
}


int Referential::unpackExtraData(const unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;
    int size = *sourceBuffer;
    _extraDataBuffer.clear();
    _extraDataBuffer.setRawData(reinterpret_cast<const char*>(sourceBuffer), size + 1);
    sourceBuffer += size + 1;
    return sourceBuffer - startPosition;
}


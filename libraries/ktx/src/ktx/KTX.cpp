//
//  KTX.cpp
//  ktx/src/ktx
//
//  Created by Zach Pomerantz on 2/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "KTX.h"

#include <algorithm> //min max and more

using namespace ktx;

uint32_t Header::evalPadding(size_t byteSize) {
    //auto padding = byteSize % PACKING_SIZE;
 //   return (uint32_t) (padding ? PACKING_SIZE - padding : 0);
    return (uint32_t) (3 - (byteSize + 3) % PACKING_SIZE);// padding ? PACKING_SIZE - padding : 0);
}


const Header::Identifier ktx::Header::IDENTIFIER {{
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
}};

Header::Header() {
    memcpy(identifier, IDENTIFIER.data(), IDENTIFIER_LENGTH);
}

uint32_t Header::evalMaxDimension() const {
    return std::max(getPixelWidth(), std::max(getPixelHeight(), getPixelDepth()));
}

uint32_t Header::evalPixelWidth(uint32_t level) const {
    return std::max(getPixelWidth() >> level, 1U);
}
uint32_t Header::evalPixelHeight(uint32_t level) const {
    return std::max(getPixelHeight() >> level, 1U);
}
uint32_t Header::evalPixelDepth(uint32_t level) const {
    return std::max(getPixelDepth() >> level, 1U);
}

size_t Header::evalPixelSize() const {
    return glTypeSize; // Really we should generate the size from the FOrmat etc
}

size_t Header::evalRowSize(uint32_t level) const {
    auto pixWidth = evalPixelWidth(level);
    auto pixSize = evalPixelSize();
    auto netSize = pixWidth * pixSize;
    auto padding = evalPadding(netSize);
    return netSize + padding;
}
size_t Header::evalFaceSize(uint32_t level) const {
    auto pixHeight = evalPixelHeight(level);
    auto pixDepth = evalPixelDepth(level);
    auto rowSize = evalRowSize(level);
    return pixDepth * pixHeight * rowSize;
}
size_t Header::evalImageSize(uint32_t level) const {
    auto faceSize = evalFaceSize(level);
    if (numberOfFaces == 6 && numberOfArrayElements == 0) {
        return faceSize;
    } else {
        return (getNumberOfSlices() * numberOfFaces * faceSize);
    }
}


KTX::KTX() {
}

KTX::~KTX() {
}

void KTX::resetStorage(Storage* storage) {
    _storage.reset(storage);
}

const Header* KTX::getHeader() const {
    if (_storage) {
        return reinterpret_cast<const Header*> (_storage->_bytes);
    } else {
        return nullptr;
    }
}


size_t KTX::getKeyValueDataSize() const {
    if (_storage) {
        return getHeader()->bytesOfKeyValueData;
    } else {
        return 0;
    }
}

size_t KTX::getTexelsDataSize() const {
    if (_storage) {
        //return  _storage->size() - (sizeof(Header) + getKeyValueDataSize());
        return  (_storage->_bytes + _storage->_size) - getTexelsData();
    } else {
        return 0;
    }
}

const Byte* KTX::getKeyValueData() const {
    if (_storage) {
        return (_storage->_bytes + sizeof(Header));
    } else {
        return nullptr;
    }
}

const Byte* KTX::getTexelsData() const {
    if (_storage) {
        return (_storage->_bytes + sizeof(Header) + getKeyValueDataSize());
    } else {
        return nullptr;
    }
}

Byte* KTX::getTexelsData() {
    if (_storage) {
        return (_storage->_bytes + sizeof(Header) + getKeyValueDataSize());
    } else {
        return nullptr;
    }
}

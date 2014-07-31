//
//  OctreePacketData.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 11/19/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PerfStat.h>
#include "OctreePacketData.h"

bool OctreePacketData::_debug = false;
quint64 OctreePacketData::_totalBytesOfOctalCodes = 0;
quint64 OctreePacketData::_totalBytesOfBitMasks = 0;
quint64 OctreePacketData::_totalBytesOfColor = 0;
quint64 OctreePacketData::_totalBytesOfValues = 0;
quint64 OctreePacketData::_totalBytesOfPositions = 0;
quint64 OctreePacketData::_totalBytesOfRawData = 0;



OctreePacketData::OctreePacketData(bool enableCompression, int targetSize) {
    changeSettings(enableCompression, targetSize); // does reset...
}

void OctreePacketData::changeSettings(bool enableCompression, unsigned int targetSize) {
    _enableCompression = enableCompression;
    _targetSize = std::min(MAX_OCTREE_UNCOMRESSED_PACKET_SIZE, targetSize);
    reset();
}

void OctreePacketData::reset() {
    _bytesInUse = 0;
    _bytesAvailable = _targetSize;
    _subTreeAt = 0;
    _compressedBytes = 0;
    _bytesInUseLastCheck = 0;
    _dirty = false;

    _bytesOfOctalCodes = 0;
    _bytesOfBitMasks = 0;
    _bytesOfColor = 0;
    _bytesOfOctalCodesCurrentSubTree = 0;
}

OctreePacketData::~OctreePacketData() {
}

bool OctreePacketData::append(const unsigned char* data, int length) {
    bool success = false;

    if (length <= _bytesAvailable) {
        memcpy(&_uncompressed[_bytesInUse], data, length);
        _bytesInUse += length;
        _bytesAvailable -= length;
        success = true;
        _dirty = true;
    }
    return success;
}

bool OctreePacketData::append(unsigned char byte) {
    bool success = false;
    if (_bytesAvailable > 0) {
        _uncompressed[_bytesInUse] = byte;
        _bytesInUse++;
        _bytesAvailable--; 
        success = true;
        _dirty = true;
    }
    return success;
}

bool OctreePacketData::updatePriorBitMask(int offset, unsigned char bitmask) {
    bool success = false;
    if (offset >= 0 && offset < _bytesInUse) {
        _uncompressed[offset] = bitmask;
        success = true;
        _dirty = true;
    }
    return success;
}

bool OctreePacketData::updatePriorBytes(int offset, const unsigned char* replacementBytes, int length) {
    bool success = false;
    if (length >= 0 && offset >= 0 && ((offset + length) <= _bytesInUse)) {
        memcpy(&_uncompressed[offset], replacementBytes, length); // copy new content
        success = true;
        _dirty = true;
    }
    return success;
}

bool OctreePacketData::startSubTree(const unsigned char* octcode) {
    _bytesOfOctalCodesCurrentSubTree = _bytesOfOctalCodes;
    bool success = false;
    int possibleStartAt = _bytesInUse;
    int length = 0;
    if (octcode) {
        length = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octcode));
        success = append(octcode, length); // handles checking compression
    } else {
        // NULL case, means root node, which is 0
        unsigned char byte = 0;
        length = 1;
        success = append(byte); // handles checking compression
    }
    if (success) {
        _subTreeAt = possibleStartAt;
    }
    if (success) {
        _bytesOfOctalCodes += length;
        _totalBytesOfOctalCodes += length;
    }
    return success;
}

const unsigned char* OctreePacketData::getFinalizedData() {
    if (!_enableCompression) {
        return &_uncompressed[0]; 
    }

    if (_dirty) {
        if (_debug) {
            qDebug("getFinalizedData() _compressedBytes=%d _bytesInUse=%d",_compressedBytes, _bytesInUse);
        }
        compressContent();
    }
    return &_compressed[0]; 
}

int OctreePacketData::getFinalizedSize() {
    if (!_enableCompression) {
        return _bytesInUse; 
    }

    if (_dirty) {
        if (_debug) {
            qDebug("getFinalizedSize() _compressedBytes=%d _bytesInUse=%d",_compressedBytes, _bytesInUse);
        }
        compressContent(); 
    }

    return _compressedBytes; 
}


void OctreePacketData::endSubTree() {
    _subTreeAt = _bytesInUse;
}

void OctreePacketData::discardSubTree() {
    int bytesInSubTree = _bytesInUse - _subTreeAt;
    _bytesInUse -= bytesInSubTree;
    _bytesAvailable += bytesInSubTree; 
    _subTreeAt = _bytesInUse; // should be the same actually...
    _dirty = true;

    // rewind to start of this subtree, other items rewound by endLevel()
    int reduceBytesOfOctalCodes = _bytesOfOctalCodes - _bytesOfOctalCodesCurrentSubTree;
    _bytesOfOctalCodes = _bytesOfOctalCodesCurrentSubTree;
    _totalBytesOfOctalCodes -= reduceBytesOfOctalCodes;
}

LevelDetails OctreePacketData::startLevel() {
    LevelDetails key(_bytesInUse, _bytesOfOctalCodes, _bytesOfBitMasks, _bytesOfColor);
    return key;
}

void OctreePacketData::discardLevel(LevelDetails key) {
    int bytesInLevel = _bytesInUse - key._startIndex;
    
    // reset statistics...
    int reduceBytesOfOctalCodes = _bytesOfOctalCodes - key._bytesOfOctalCodes;
    int reduceBytesOfBitMasks = _bytesOfBitMasks - key._bytesOfBitmasks;
    int reduceBytesOfColor = _bytesOfColor - key._bytesOfColor;

    _bytesOfOctalCodes = key._bytesOfOctalCodes;
    _bytesOfBitMasks = key._bytesOfBitmasks;
    _bytesOfColor = key._bytesOfColor;

    _totalBytesOfOctalCodes -= reduceBytesOfOctalCodes;
    _totalBytesOfBitMasks -= reduceBytesOfBitMasks;
    _totalBytesOfColor -= reduceBytesOfColor;

    if (_debug) {
        qDebug("discardLevel() BEFORE _dirty=%s bytesInLevel=%d _compressedBytes=%d _bytesInUse=%d",
            debug::valueOf(_dirty), bytesInLevel, _compressedBytes, _bytesInUse);
    }
            
    _bytesInUse -= bytesInLevel;
    _bytesAvailable += bytesInLevel; 
    _dirty = true;

    if (_debug) {
        qDebug("discardLevel() AFTER _dirty=%s bytesInLevel=%d _compressedBytes=%d _bytesInUse=%d",
            debug::valueOf(_dirty), bytesInLevel, _compressedBytes, _bytesInUse);
    }
}

bool OctreePacketData::endLevel(LevelDetails key) {
    bool success = true;
    return success;
}

bool OctreePacketData::appendBitMask(unsigned char bitmask) {
    bool success = append(bitmask); // handles checking compression
    if (success) {
        _bytesOfBitMasks++;
        _totalBytesOfBitMasks++;
    }
    return success;
}

bool OctreePacketData::appendColor(const nodeColor& color) {
    return appendColor(color[RED_INDEX], color[GREEN_INDEX], color[BLUE_INDEX]);
}

bool OctreePacketData::appendColor(const rgbColor& color) {
    return appendColor(color[RED_INDEX], color[GREEN_INDEX], color[BLUE_INDEX]);
}

bool OctreePacketData::appendColor(colorPart red, colorPart green, colorPart blue) {
    // eventually we can make this use a dictionary...
    bool success = false;
    const int BYTES_PER_COLOR = 3;
    if (_bytesAvailable > BYTES_PER_COLOR) {
        // handles checking compression...
        if (append(red)) {
            if (append(green)) {
                if (append(blue)) {
                    success = true;
                }
            }
        }
    }
    if (success) {
        _bytesOfColor += BYTES_PER_COLOR;
        _totalBytesOfColor += BYTES_PER_COLOR;
    }
    return success;
}

bool OctreePacketData::appendValue(uint8_t value) {
    bool success = append(value); // used unsigned char version
    if (success) {
        _bytesOfValues++;
        _totalBytesOfValues++;
    }
    return success;
}

bool OctreePacketData::appendValue(uint16_t value) {
    const unsigned char* data = (const unsigned char*)&value;
    int length = sizeof(value);
    bool success = append(data, length);
    if (success) {
        _bytesOfValues += length;
        _totalBytesOfValues += length;
    }
    return success;
}

bool OctreePacketData::appendValue(uint32_t value) {
    const unsigned char* data = (const unsigned char*)&value;
    int length = sizeof(value);
    bool success = append(data, length);
    if (success) {
        _bytesOfValues += length;
        _totalBytesOfValues += length;
    }
    return success;
}

bool OctreePacketData::appendValue(quint64 value) {
    const unsigned char* data = (const unsigned char*)&value;
    int length = sizeof(value);
    bool success = append(data, length);

    if (success) {
        _bytesOfValues += length;
        _totalBytesOfValues += length;
        qDebug() << "OctreePacketData::appendValue(quint64 value) SUCCESS length=" << length;
    } else {
        qDebug() << "OctreePacketData::appendValue(quint64 value) FAILED length=" << length;
    }
    return success;
}

bool OctreePacketData::appendValue(float value) {
    const unsigned char* data = (const unsigned char*)&value;
    int length = sizeof(value);
    bool success = append(data, length);
    if (success) {
        _bytesOfValues += length;
        _totalBytesOfValues += length;
    }
    return success;
}

bool OctreePacketData::appendValue(const glm::vec3& value) {
    const unsigned char* data = (const unsigned char*)&value;
    int length = sizeof(value);
    bool success = append(data, length);
    if (success) {
        _bytesOfValues += length;
        _totalBytesOfValues += length;
    }
    return success;
}

bool OctreePacketData::appendValue(const glm::quat& value) {
    const size_t VALUES_PER_QUAT = 4;
    const size_t PACKED_QUAT_SIZE = sizeof(uint16_t) * VALUES_PER_QUAT;
    unsigned char data[PACKED_QUAT_SIZE];
    int length = packOrientationQuatToBytes(data, value);
    bool success = append(data, length);
    if (success) {
        _bytesOfValues += length;
        _totalBytesOfValues += length;
    }
    return success;
}

bool OctreePacketData::appendValue(bool value) {
    bool success = append((uint8_t)value); // used unsigned char version
    if (success) {
        _bytesOfValues++;
        _totalBytesOfValues++;
    }
    return success;
}

bool OctreePacketData::appendValue(const QString& string) {
    // TODO: make this a ByteCountCoded leading byte
    uint16_t length = string.size() + 1; // include NULL
    bool success = appendValue(length);
    if (success) {
        success = appendRawData((const unsigned char*)qPrintable(string), length);
    }
    return success;
}

bool OctreePacketData::appendValue(const QByteArray& bytes) {
    bool success = appendRawData((const unsigned char*)bytes.constData(), bytes.size());
    return success;
}


bool OctreePacketData::appendPosition(const glm::vec3& value) {
    const unsigned char* data = (const unsigned char*)&value;
    int length = sizeof(value);
    bool success = append(data, length);
    if (success) {
        _bytesOfPositions += length;
        _totalBytesOfPositions += length;
    }
    return success;
}

bool OctreePacketData::appendRawData(const unsigned char* data, int length) {
    bool success = append(data, length);
    if (success) {
        _bytesOfRawData += length;
        _totalBytesOfRawData += length;
    }
    return success;
}

quint64 OctreePacketData::_compressContentTime = 0;
quint64 OctreePacketData::_compressContentCalls = 0;

bool OctreePacketData::compressContent() { 
    PerformanceWarning warn(false, "OctreePacketData::compressContent()", false, &_compressContentTime, &_compressContentCalls);
    
    // without compression, we always pass...
    if (!_enableCompression) {
        return true;
    }

    _bytesInUseLastCheck = _bytesInUse;

    bool success = false;
    const int MAX_COMPRESSION = 9;

    // we only want to compress the data payload, not the message header
    const uchar* uncompressedData = &_uncompressed[0];
    int uncompressedSize = _bytesInUse;

    QByteArray compressedData = qCompress(uncompressedData, uncompressedSize, MAX_COMPRESSION);

    if (compressedData.size() < (int)MAX_OCTREE_PACKET_DATA_SIZE) {
        _compressedBytes = compressedData.size();
        for (int i = 0; i < _compressedBytes; i++) {
            _compressed[i] = compressedData[i];
        }
        _dirty = false;
        success = true;
    }
    return success;
}


void OctreePacketData::loadFinalizedContent(const unsigned char* data, int length) {
    reset();

    if (data && length > 0) {

        if (_enableCompression) {
            QByteArray compressedData;
            for (int i = 0; i < length; i++) {
                compressedData[i] = data[i];
                _compressed[i] = compressedData[i];
            }
            _compressedBytes = length;
            QByteArray uncompressedData = qUncompress(compressedData);
            if (uncompressedData.size() <= _bytesAvailable) {
                _bytesInUse = uncompressedData.size();
                _bytesAvailable -= uncompressedData.size();

                for (int i = 0; i < _bytesInUse; i++) {
                    _uncompressed[i] = uncompressedData[i];
                }
            }
        } else {
            for (int i = 0; i < length; i++) {
                _uncompressed[i] = _compressed[i] = data[i];
            }
            _bytesInUse = _compressedBytes = length;
        }
    } else {
        if (_debug) {
            qDebug("OctreePacketData::loadCompressedContent()... length = 0, nothing to do...");
        }
    }
}

void OctreePacketData::debugContent() {
    qDebug("OctreePacketData::debugContent()... COMPRESSED DATA.... size=%d",_compressedBytes);
    int perline=0;
    for (int i = 0; i < _compressedBytes; i++) {
        printf("%.2x ",_compressed[i]);
        perline++;
        if (perline >= 30) {
            printf("\n");
            perline=0;
        }
    }
    printf("\n");
    
    qDebug("OctreePacketData::debugContent()... UNCOMPRESSED DATA.... size=%d",_bytesInUse);
    perline=0;
    for (int i = 0; i < _bytesInUse; i++) {
        printf("%.2x ",_uncompressed[i]);
        perline++;
        if (perline >= 30) {
            printf("\n");
            perline=0;
        }
    }
    printf("\n");
}

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

#include <GLMHelpers.h>
#include <PerfStat.h>

#include "OctreeLogging.h"
#include "OctreePacketData.h"
#include "NumericalConstants.h"

bool OctreePacketData::_debug = false;
AtomicUIntStat OctreePacketData::_totalBytesOfOctalCodes { 0 };
AtomicUIntStat OctreePacketData::_totalBytesOfBitMasks { 0 };
AtomicUIntStat OctreePacketData::_totalBytesOfColor { 0 };
AtomicUIntStat OctreePacketData::_totalBytesOfValues { 0 };
AtomicUIntStat OctreePacketData::_totalBytesOfPositions { 0 };
AtomicUIntStat OctreePacketData::_totalBytesOfRawData { 0 };

struct aaCubeData {
    glm::vec3 corner;
    float scale;
};

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
    _bytesReserved = 0;
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
    
    const bool wantDebug = false;
    if (wantDebug && !success) {
        qCDebug(octree) << "OctreePacketData::append(const unsigned char* data, int length) FAILING....";
        qCDebug(octree) << "    length=" << length;
        qCDebug(octree) << "    _bytesAvailable=" << _bytesAvailable;
        qCDebug(octree) << "    _bytesInUse=" << _bytesInUse;
        qCDebug(octree) << "    _targetSize=" << _targetSize;
        qCDebug(octree) << "    _bytesReserved=" << _bytesReserved;
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

bool OctreePacketData::reserveBitMask() {
    return reserveBytes(sizeof(unsigned char));
}

bool OctreePacketData::reserveBytes(int numberOfBytes) {
    bool success = false;
    
    if (_bytesAvailable >= numberOfBytes) {
        _bytesReserved += numberOfBytes;
        _bytesAvailable -= numberOfBytes;
        success = true;
    }
    
    return success;
}

bool OctreePacketData::releaseReservedBitMask() {
    return releaseReservedBytes(sizeof(unsigned char));
}

bool OctreePacketData::releaseReservedBytes(int numberOfBytes) {
    bool success = false;

    if (_bytesReserved >= numberOfBytes) {
        _bytesReserved -= numberOfBytes;
        _bytesAvailable += numberOfBytes;
        success = true;
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
        if (replacementBytes >= &_uncompressed[offset] && replacementBytes <= &_uncompressed[offset + length]) {
            memmove(&_uncompressed[offset], replacementBytes, length); // copy new content with overlap safety
        } else {
            memcpy(&_uncompressed[offset], replacementBytes, length); // copy new content
        }
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
        length = (int)bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octcode));
        success = append(octcode, length); // handles checking compression
    } else {
        // NULL case, means root node, which is 0
        unsigned char byte = 0;
        length = 1;
        success = append(byte); // handles checking compression
    }
    if (success) {
        _subTreeAt = possibleStartAt;
        _subTreeBytesReserved = _bytesReserved;
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
            qCDebug(octree, "getFinalizedData() _compressedBytes=%d _bytesInUse=%d",_compressedBytes, _bytesInUse);
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
            qCDebug(octree, "getFinalizedSize() _compressedBytes=%d _bytesInUse=%d",_compressedBytes, _bytesInUse);
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
    
    // if we discard the subtree then reset reserved bytes to the value when we started the subtree
    _bytesReserved = _subTreeBytesReserved;
}

LevelDetails OctreePacketData::startLevel() {
    LevelDetails key(_bytesInUse, _bytesOfOctalCodes, _bytesOfBitMasks, _bytesOfColor, _bytesReserved);
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
        qCDebug(octree, "discardLevel() BEFORE _dirty=%s bytesInLevel=%d _compressedBytes=%d _bytesInUse=%d",
            debug::valueOf(_dirty), bytesInLevel, _compressedBytes, _bytesInUse);
    }
            
    _bytesInUse -= bytesInLevel;
    _bytesAvailable += bytesInLevel; 
    _dirty = true;
    
    // reserved bytes are reset to the value when the level started
    _bytesReserved = key._bytesReservedAtStart;

    if (_debug) {
        qCDebug(octree, "discardLevel() AFTER _dirty=%s bytesInLevel=%d _compressedBytes=%d _bytesInUse=%d",
            debug::valueOf(_dirty), bytesInLevel, _compressedBytes, _bytesInUse);
    }
}

bool OctreePacketData::endLevel(LevelDetails key) {
    bool success = true;

    // reserved bytes should be the same value as when the level started
    if (_bytesReserved != key._bytesReservedAtStart) {
        qCDebug(octree) << "WARNING: endLevel() called but some reserved bytes not used.";
        qCDebug(octree) << "       current bytesReserved:" << _bytesReserved;
        qCDebug(octree) << "   start level bytesReserved:" << key._bytesReservedAtStart;
    }

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

bool OctreePacketData::appendValue(const nodeColor& color) {
    return appendColor(color[RED_INDEX], color[GREEN_INDEX], color[BLUE_INDEX]);
}

bool OctreePacketData::appendValue(const xColor& color) {
    return appendColor(color.red, color.green, color.blue);
}

bool OctreePacketData::appendValue(const rgbColor& color) {
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

bool OctreePacketData::appendValue(const QVector<glm::vec3>& value) {
    uint16_t qVecSize = value.size();
    bool success = appendValue(qVecSize);
    if (success) {
        success = append((const unsigned char*)value.constData(), qVecSize * sizeof(glm::vec3));
        if (success) {
            _bytesOfValues += qVecSize * sizeof(glm::vec3);
            _totalBytesOfValues += qVecSize * sizeof(glm::vec3);
        }
    }
    return success;
}

bool OctreePacketData::appendValue(const QVector<glm::quat>& value) {
    uint16_t qVecSize = value.size();
    bool success = appendValue(qVecSize);

    if (success) {
        QByteArray dataByteArray(udt::MAX_PACKET_SIZE, 0);
        unsigned char* start = reinterpret_cast<unsigned char*>(dataByteArray.data());
        unsigned char* destinationBuffer = start;
        for (int index = 0; index < value.size(); index++) {
            destinationBuffer += packOrientationQuatToBytes(destinationBuffer, value[index]);
        }
        int quatsSize = destinationBuffer - start;
        success = append(start, quatsSize);
        if (success) {
            _bytesOfValues += quatsSize;
            _totalBytesOfValues += quatsSize;
        }
    }

    return success;
}

bool OctreePacketData::appendValue(const QVector<float>& value) {
    uint16_t qVecSize = value.size();
    bool success = appendValue(qVecSize);
    if (success) {
        success = append((const unsigned char*)value.constData(), qVecSize * sizeof(float));
        if (success) {
            _bytesOfValues += qVecSize * sizeof(float);
            _totalBytesOfValues += qVecSize * sizeof(float);
        }
    }
    return success;
}

bool OctreePacketData::appendValue(const QVector<bool>& value) {
    uint16_t qVecSize = value.size();
    bool success = appendValue(qVecSize);

    if (success) {
        QByteArray dataByteArray(udt::MAX_PACKET_SIZE, 0);
        unsigned char* start = reinterpret_cast<unsigned char*>(dataByteArray.data());
        unsigned char* destinationBuffer = start;
        int bit = 0;
        for (int index = 0; index < value.size(); index++) {
            if (value[index]) {
                (*destinationBuffer) |= (1 << bit);
            }
            if (++bit == BITS_IN_BYTE) {
                destinationBuffer++;
                bit = 0;
            }
        }
        if (bit != 0) {
            destinationBuffer++;
        }
        int boolsSize = destinationBuffer - start;
        success = append(start, boolsSize);
        if (success) {
            _bytesOfValues += boolsSize;
            _totalBytesOfValues += boolsSize;
        }
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

bool OctreePacketData::appendValue(const QUuid& uuid) {
    QByteArray bytes = uuid.toRfc4122();
    if (uuid.isNull()) {
        return appendValue((uint16_t)0); // zero length for null uuid
    } else {
        uint16_t length = bytes.size();
        bool success = appendValue(length);
        if (success) {
            success = appendRawData((const unsigned char*)bytes.constData(), bytes.size());
        }
        return success;
    }
}

bool OctreePacketData::appendValue(const QByteArray& bytes) {
    // TODO: make this a ByteCountCoded leading byte
    uint16_t length = bytes.size();
    bool success = appendValue(length);
    if (success) {
        success = appendRawData((const unsigned char*)bytes.constData(), bytes.size());
    }
    return success;
}

bool OctreePacketData::appendValue(const AACube& aaCube) {
    aaCubeData cube { aaCube.getCorner(), aaCube.getScale() };
    const unsigned char* data = (const unsigned char*)&cube;
    int length = sizeof(aaCubeData);
    bool success = append(data, length);
    if (success) {
        _bytesOfValues += length;
        _totalBytesOfValues += length;
    }
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


bool OctreePacketData::appendRawData(QByteArray data) {
    return appendRawData((unsigned char *)data.data(), data.size());
}


AtomicUIntStat OctreePacketData::_compressContentTime { 0 };
AtomicUIntStat OctreePacketData::_compressContentCalls { 0 };

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
            qCDebug(octree, "OctreePacketData::loadCompressedContent()... length = 0, nothing to do...");
        }
    }
}

void OctreePacketData::debugContent() {
    qCDebug(octree, "OctreePacketData::debugContent()... COMPRESSED DATA.... size=%d",_compressedBytes);
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
    
    qCDebug(octree, "OctreePacketData::debugContent()... UNCOMPRESSED DATA.... size=%d",_bytesInUse);
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

int OctreePacketData::unpackDataFromBytes(const unsigned char* dataBytes, QString& result) { 
    uint16_t length;
    memcpy(&length, dataBytes, sizeof(length));
    dataBytes += sizeof(length);
    QString value((const char*)dataBytes);
    result = value;
    return sizeof(length) + length;
}

int OctreePacketData::unpackDataFromBytes(const unsigned char* dataBytes, QUuid& result) { 
    uint16_t length;
    memcpy(&length, dataBytes, sizeof(length));
    dataBytes += sizeof(length);
    if (length == 0) {
        result = QUuid();
    } else {
        QByteArray ba((const char*)dataBytes, length);
        result = QUuid::fromRfc4122(ba);
    }
    return sizeof(length) + length;
}

int OctreePacketData::unpackDataFromBytes(const unsigned char* dataBytes, xColor& result) { 
    result.red = dataBytes[RED_INDEX];
    result.green = dataBytes[GREEN_INDEX];
    result.blue = dataBytes[BLUE_INDEX];
    return sizeof(rgbColor);
}


int OctreePacketData::unpackDataFromBytes(const unsigned char *dataBytes, QVector<glm::vec3>& result) {
    uint16_t length;
    memcpy(&length, dataBytes, sizeof(uint16_t));
    dataBytes += sizeof(length);
    if (length * sizeof(glm::vec3) > MAX_OCTREE_UNCOMRESSED_PACKET_SIZE) {
        result.resize(0);
        return sizeof(uint16_t);
    }
    result.resize(length);
    memcpy(result.data(), dataBytes, length * sizeof(glm::vec3));
    return sizeof(uint16_t) + length * sizeof(glm::vec3);
}

int OctreePacketData::unpackDataFromBytes(const unsigned char *dataBytes, QVector<glm::quat>& result) {
    uint16_t length;
    memcpy(&length, dataBytes, sizeof(uint16_t));
    dataBytes += sizeof(length);
    if (length * sizeof(glm::quat) > MAX_OCTREE_UNCOMRESSED_PACKET_SIZE) {
        result.resize(0);
        return sizeof(uint16_t);
    }
    result.resize(length);

    const unsigned char *start = dataBytes;
    for (int i = 0; i < length; i++) {
        dataBytes += unpackOrientationQuatFromBytes(dataBytes, result[i]);
    }

    return (dataBytes - start) + (int)sizeof(uint16_t);
}

int OctreePacketData::unpackDataFromBytes(const unsigned char* dataBytes, QVector<float>& result) {
    uint16_t length;
    memcpy(&length, dataBytes, sizeof(uint16_t));
    dataBytes += sizeof(length);
    if (length * sizeof(float) > MAX_OCTREE_UNCOMRESSED_PACKET_SIZE) {
        result.resize(0);
        return sizeof(uint16_t);
    }
    result.resize(length);
    memcpy(result.data(), dataBytes, length * sizeof(float));
    return sizeof(uint16_t) + length * sizeof(float);
}

int OctreePacketData::unpackDataFromBytes(const unsigned char* dataBytes, QVector<bool>& result) {
    uint16_t length;
    memcpy(&length, dataBytes, sizeof(uint16_t));
    dataBytes += sizeof(length);
    if (length * sizeof(bool) > MAX_OCTREE_UNCOMRESSED_PACKET_SIZE) {
        result.resize(0);
        return sizeof(uint16_t);
    }
    result.resize(length);

    int bit = 0;
    unsigned char current = 0;
    const unsigned char *start = dataBytes;
    for (int i = 0; i < length; i ++) {
        if (bit == 0) {
            current = *dataBytes++;
        }
        result[i] = (bool)(current & (1 << bit));
        bit = (bit + 1) % BITS_IN_BYTE;
    }

    return (dataBytes - start) + (int)sizeof(uint16_t);
}

int OctreePacketData::unpackDataFromBytes(const unsigned char* dataBytes, QByteArray& result) {
    uint16_t length;
    memcpy(&length, dataBytes, sizeof(length));
    dataBytes += sizeof(length);
    QByteArray value((const char*)dataBytes, length);
    result = value;
    return sizeof(length) + length;
}

int OctreePacketData::unpackDataFromBytes(const unsigned char* dataBytes, AACube& result) {
    aaCubeData cube;
    memcpy(&cube, dataBytes, sizeof(aaCubeData));
    result = AACube(cube.corner, cube.scale);
    return sizeof(aaCubeData);
}

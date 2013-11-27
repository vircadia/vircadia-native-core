//
//  VoxelPacketData.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PerfStat.h>
#include "VoxelPacketData.h"

bool VoxelPacketData::_debug = false;
uint64_t VoxelPacketData::_totalBytesOfOctalCodes = 0;
uint64_t VoxelPacketData::_totalBytesOfBitMasks = 0;
uint64_t VoxelPacketData::_totalBytesOfColor = 0;



VoxelPacketData::VoxelPacketData(bool enableCompression, int targetSize) {
    changeSettings(enableCompression, targetSize); // does reset...
}

void VoxelPacketData::changeSettings(bool enableCompression, int targetSize) {
    _enableCompression = enableCompression;
    _targetSize = std::min(MAX_VOXEL_UNCOMRESSED_PACKET_SIZE, targetSize);
    reset();
}

void VoxelPacketData::reset() {
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

VoxelPacketData::~VoxelPacketData() {
}

bool VoxelPacketData::append(const unsigned char* data, int length) {
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

bool VoxelPacketData::append(unsigned char byte) {
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

bool VoxelPacketData::updatePriorBitMask(int offset, unsigned char bitmask) {
    bool success = false;
    if (offset >= 0 && offset < _bytesInUse) {
        _uncompressed[offset] = bitmask;
        success = true;
        _dirty = true;
    }
    return success;
}

bool VoxelPacketData::updatePriorBytes(int offset, const unsigned char* replacementBytes, int length) {
    bool success = false;
    if (length >= 0 && offset >= 0 && ((offset + length) <= _bytesInUse)) {
        memcpy(&_uncompressed[offset], replacementBytes, length); // copy new content
        success = true;
        _dirty = true;
    }
    return success;
}

bool VoxelPacketData::startSubTree(const unsigned char* octcode) {
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

const unsigned char* VoxelPacketData::getFinalizedData() {
    if (!_enableCompression) {
        return &_uncompressed[0]; 
    }

    if (_dirty) {
        if (_debug) {
            printf("getFinalizedData() _compressedBytes=%d _bytesInUse=%d\n",_compressedBytes, _bytesInUse);
        }
        compressContent();
    }
    return &_compressed[0]; 
}

int VoxelPacketData::getFinalizedSize() {
    if (!_enableCompression) {
        return _bytesInUse; 
    }

    if (_dirty) {
        if (_debug) {
            printf("getFinalizedSize() _compressedBytes=%d _bytesInUse=%d\n",_compressedBytes, _bytesInUse);
        }
        compressContent(); 
    }

    return _compressedBytes; 
}


void VoxelPacketData::endSubTree() {
    _subTreeAt = _bytesInUse;
}

void VoxelPacketData::discardSubTree() {
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

LevelDetails VoxelPacketData::startLevel() {
    LevelDetails key(_bytesInUse, _bytesOfOctalCodes, _bytesOfBitMasks, _bytesOfColor);
    return key;
}

void VoxelPacketData::discardLevel(LevelDetails key) {
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
        printf("discardLevel() BEFORE _dirty=%s bytesInLevel=%d _compressedBytes=%d _bytesInUse=%d\n",
            debug::valueOf(_dirty), bytesInLevel, _compressedBytes, _bytesInUse);
    }
            
    _bytesInUse -= bytesInLevel;
    _bytesAvailable += bytesInLevel; 
    _dirty = true;

    if (_debug) {
        printf("discardLevel() AFTER _dirty=%s bytesInLevel=%d _compressedBytes=%d _bytesInUse=%d\n",
            debug::valueOf(_dirty), bytesInLevel, _compressedBytes, _bytesInUse);
    }
}

bool VoxelPacketData::endLevel(LevelDetails key) {
    bool success = true;
    return success;
}

bool VoxelPacketData::appendBitMask(unsigned char bitmask) {
    bool success = append(bitmask); // handles checking compression
    if (success) {
        _bytesOfBitMasks++;
        _totalBytesOfBitMasks++;
    }
    return success;
}

bool VoxelPacketData::appendColor(const nodeColor& color) {
    // eventually we can make this use a dictionary...
    bool success = false;
    const int BYTES_PER_COLOR = 3;
    if (_bytesAvailable > BYTES_PER_COLOR) {
        // handles checking compression...
        if (append(color[RED_INDEX])) {
            if (append(color[GREEN_INDEX])) {
                if (append(color[BLUE_INDEX])) {
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

uint64_t VoxelPacketData::_compressContentTime = 0;
uint64_t VoxelPacketData::_compressContentCalls = 0;

bool VoxelPacketData::compressContent() { 
    PerformanceWarning warn(false, "VoxelPacketData::compressContent()", false, &_compressContentTime, &_compressContentCalls);
    
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

    if (compressedData.size() < MAX_VOXEL_PACKET_DATA_SIZE) {
        _compressedBytes = compressedData.size();
        for (int i = 0; i < _compressedBytes; i++) {
            _compressed[i] = compressedData[i];
        }
        _dirty = false;
        success = true;
    }
    return success;
}


void VoxelPacketData::loadFinalizedContent(const unsigned char* data, int length) {
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
            printf("VoxelPacketData::loadCompressedContent()... length = 0, nothing to do...\n");
        }
    }
}

void VoxelPacketData::debugContent() {
    printf("VoxelPacketData::debugContent()... COMPRESSED DATA.... size=%d\n",_compressedBytes);
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
    
    printf("VoxelPacketData::debugContent()... UNCOMPRESSED DATA.... size=%d\n",_bytesInUse);
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

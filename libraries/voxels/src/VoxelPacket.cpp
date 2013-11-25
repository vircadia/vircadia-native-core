//
//  VoxelPacket.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PerfStat.h>
#include "VoxelPacket.h"

bool VoxelPacket::_debug = false;
uint64_t VoxelPacket::_bytesOfOctalCodes = 0;
uint64_t VoxelPacket::_bytesOfBitMasks = 0;
uint64_t VoxelPacket::_bytesOfColor = 0;



VoxelPacket::VoxelPacket(bool enableCompression, int maxFinalizedSize) {
    _enableCompression = enableCompression;
    _maxFinalizedSize = maxFinalizedSize;
    reset();
}

void VoxelPacket::reset() {
    _bytesInUse = 0;
    if (_enableCompression) {
        _bytesAvailable = MAX_VOXEL_UNCOMRESSED_PACKET_SIZE;
    } else {
        _bytesAvailable = _maxFinalizedSize;
    }
    _subTreeAt = 0;
    _compressedBytes = 0;
    _bytesInUseLastCheck = 0;
    _dirty = false;
}

VoxelPacket::~VoxelPacket() {
}

bool VoxelPacket::append(const unsigned char* data, int length) {
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

bool VoxelPacket::append(unsigned char byte) {
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

bool VoxelPacket::updatePriorBitMask(int offset, unsigned char bitmask) {
    bool success = false;
    if (offset >= 0 && offset < _bytesInUse) {
        _uncompressed[offset] = bitmask;
        success = true;
        _dirty = true;
    }
    return success;
}

bool VoxelPacket::updatePriorBytes(int offset, const unsigned char* replacementBytes, int length) {
    bool success = false;
    if (length >= 0 && offset >= 0 && ((offset + length) <= _bytesInUse)) {
        memcpy(&_uncompressed[offset], replacementBytes, length); // copy new content
        success = true;
        _dirty = true;
    }
    return success;
}

bool VoxelPacket::startSubTree(const unsigned char* octcode) {
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
    }
    return success;
}

const unsigned char* VoxelPacket::getFinalizedData() {
    if (!_enableCompression) {
        return &_uncompressed[0]; 
    }
    
    if (_dirty) {
        if (_debug) {
            printf("getFinalizedData() _compressedBytes=%d _bytesInUse=%d\n",_compressedBytes, _bytesInUse);
        }
        checkCompress(); 
    }

    return &_compressed[0]; 
}

int VoxelPacket::getFinalizedSize() { 
    if (!_enableCompression) {
        return _bytesInUse; 
    }

    if (_dirty) {
        if (_debug) {
            printf("getFinalizedSize() _compressedBytes=%d _bytesInUse=%d\n",_compressedBytes, _bytesInUse);
        }
        checkCompress(); 
    }

    return _compressedBytes; 
}


void VoxelPacket::endSubTree() {
    _subTreeAt = _bytesInUse;
}

void VoxelPacket::discardSubTree() {
    int bytesInSubTree = _bytesInUse - _subTreeAt;
    _bytesInUse -= bytesInSubTree;
    _bytesAvailable += bytesInSubTree; 
    _subTreeAt = _bytesInUse; // should be the same actually...
    _dirty = true;
}

LevelDetails VoxelPacket::startLevel() {
    LevelDetails key(_bytesInUse,0,0,0);
    return key;
}

void VoxelPacket::discardLevel(LevelDetails key) {
    int bytesInLevel = _bytesInUse - key._startIndex;

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

bool VoxelPacket::endLevel(LevelDetails key) {
    bool success = true;

    // if we are dirty (something has changed) then try a compression test in the following cases...
    //    1) If we've previously compressed and our _compressedBytes are "close enough to our limit" that we want to keep
    //       testing to make sure we don't overflow... VOXEL_PACKET_ALWAYS_TEST_COMPRESSION_THRESHOLD
    //    2) If we've passed the uncompressed size where we believe we might pass the compressed threshold, and we've added
    //       a sufficient number of uncompressed bytes
    if (_dirty && (
            (_compressedBytes > VOXEL_PACKET_ALWAYS_TEST_COMPRESSED_THRESHOLD) || 
            (   (_bytesInUse > VOXEL_PACKET_TEST_UNCOMPRESSED_THRESHOLD) && 
                (_bytesInUse - _bytesInUseLastCheck > VOXEL_PACKET_TEST_UNCOMPRESSED_CHANGE_THRESHOLD)
            )
        )) {
        if (_debug) {
            printf("endLevel() _dirty=%s _compressedBytes=%d _bytesInUse=%d\n",
                debug::valueOf(_dirty), _compressedBytes, _bytesInUse);
        }
        success = checkCompress();
    }
    if (!success) {
        discardLevel(key);
    }
    return success;
}

bool VoxelPacket::appendBitMask(unsigned char bitmask) {
    bool success = append(bitmask); // handles checking compression
    if (success) {
        _bytesOfBitMasks++;
    }
    return success;
}

bool VoxelPacket::appendColor(const nodeColor& color) {
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
    }
    return success;
}

uint64_t VoxelPacket::_checkCompressTime = 0;
uint64_t VoxelPacket::_checkCompressCalls = 0;

bool VoxelPacket::checkCompress() { 
    PerformanceWarning warn(false,"VoxelPacket::checkCompress()",false,&_checkCompressTime,&_checkCompressCalls);
    
    // without compression, we always pass...
    if (!_enableCompression) {
        return true;
    }

    _bytesInUseLastCheck = _bytesInUse;

    bool success = false;
    const int MAX_COMPRESSION = 2;

    // we only want to compress the data payload, not the message header
    QByteArray compressedData = qCompress(_uncompressed, _bytesInUse, MAX_COMPRESSION);
    if (compressedData.size() < MAX_VOXEL_PACKET_SIZE) {
        _compressedBytes = compressedData.size();
        for (int i = 0; i < _compressedBytes; i++) {
            _compressed[i] = compressedData[i];
        }
        _dirty = false;
        success = true;
    }
    return success;
}


void VoxelPacket::loadFinalizedContent(const unsigned char* data, int length) {
    reset(); // by definition we reset upon loading compressed content
    
    if (length > 0) {
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
        if (_debug) printf("VoxelPacket::loadCompressedContent()... length = 0, nothing to do...\n");
    }
}

void VoxelPacket::debugContent() {

    printf("VoxelPacket::debugContent()... COMPRESSED DATA.... size=%d\n",_compressedBytes);
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
    
    printf("VoxelPacket::debugContent()... UNCOMPRESSED DATA.... size=%d\n",_bytesInUse);
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

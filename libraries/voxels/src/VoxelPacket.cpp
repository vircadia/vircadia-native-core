//
//  VoxelPacket.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "VoxelPacket.h"

VoxelPacket::VoxelPacket() {
    reset();
}

void VoxelPacket::reset() {
    _bytesInUse = 0;
    _bytesAvailable = MAX_VOXEL_PACKET_SIZE;
    _subTreeAt = 0;
    _levelAt = 0;
}

VoxelPacket::~VoxelPacket() {
}

bool VoxelPacket::append(const unsigned char* data, int length) {
    bool success = false;

    if (length <= _bytesAvailable) {
        memcpy(&_buffer[_bytesInUse], data, length);
        _bytesInUse += length;
        _bytesAvailable -= length;
        success = true;
    }
    return success;
}

bool VoxelPacket::setByte(int offset, unsigned char byte) {
    bool success = false;
    if (offset >= 0 && offset < _bytesInUse) {
        _buffer[offset] = byte;
        success = true;
    }
    return success;
}



void VoxelPacket::startSubTree(const unsigned char* octcode) {
    _subTreeAt = _bytesInUse;
    if (octcode) {
        int length = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octcode));
        append(octcode, length);
    } else {
        // NULL case, means root node, which is 0
        _buffer[_bytesInUse] = 0;
        _bytesInUse += 1;
        _bytesAvailable -= 1;
    }
}

void VoxelPacket::endSubTree() {
    _subTreeAt = _bytesInUse;
}

void VoxelPacket::discardSubTree() {
    int bytesInSubTree = _bytesInUse - _subTreeAt;
    _bytesInUse -= bytesInSubTree;
    _bytesAvailable += bytesInSubTree; 
}

void VoxelPacket::startLevel() {
    _levelAt = _bytesInUse;
}

bool VoxelPacket::appendBitMask(unsigned char bitmask) {
    bool success = false;
    if (_bytesAvailable > 0) {
        _buffer[_bytesInUse] = bitmask;
        success = true;
    }
    return success;
}

bool VoxelPacket::appendColor(rgbColor color) {
    // eventually we can make this use a dictionary...
    bool success = false;
    if (_bytesAvailable > sizeof(rgbColor)) {
        success = append((const unsigned char*)&color, sizeof(rgbColor));
    }
    return success;
}

void VoxelPacket::endLevel() {
    _levelAt = _bytesInUse;
}

void VoxelPacket::discardLevel() {
    int bytesInLevel = _bytesInUse - _levelAt;
    _bytesInUse -= bytesInLevel;
    _bytesAvailable += bytesInLevel; 
}

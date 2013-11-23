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

bool VoxelPacket::append(unsigned char byte) {
    bool success = false;
    if (_bytesAvailable > 0) {
        _buffer[_bytesInUse] = byte;
        _bytesInUse++;
        _bytesAvailable--; 
        success = true;
    }
    return success;
}

bool VoxelPacket::updatePriorBitMask(int offset, unsigned char bitmask) {
    bool success = false;
    if (offset >= 0 && offset < _bytesInUse) {
        _buffer[offset] = bitmask;
        success = true;
    }
    return success;
}

bool VoxelPacket::updatePriorBytes(int offset, const unsigned char* replacementBytes, int length) {
    bool success = false;
    if (length >= 0 && offset >= 0 && ((offset + length) <= _bytesInUse)) {
        memcpy(&_buffer[offset], replacementBytes, length);
        success = true;
    }
    return success;
}

bool VoxelPacket::startSubTree(const unsigned char* octcode) {
    bool success = false;
    int possibleStartAt = _bytesInUse;
    if (octcode) {
        int length = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octcode));
        success = append(octcode, length);
    } else {
        // NULL case, means root node, which is 0
        unsigned char byte = 0;
        success = append(byte);
    }
    if (success) {
        _subTreeAt = possibleStartAt;
    }
    return success;
}

void VoxelPacket::endSubTree() {
    _subTreeAt = _bytesInUse;
}

void VoxelPacket::discardSubTree() {
    int bytesInSubTree = _bytesInUse - _subTreeAt;
    _bytesInUse -= bytesInSubTree;
    _bytesAvailable += bytesInSubTree; 
    _subTreeAt = _bytesInUse; // should be the same actually...
}

int VoxelPacket::startLevel() {
    int key = _bytesInUse;
    return key;
}

void VoxelPacket::discardLevel(int key) {
    int bytesInLevel = _bytesInUse - key;
    _bytesInUse -= bytesInLevel;
    _bytesAvailable += bytesInLevel; 
}

void VoxelPacket::endLevel() {
    // nothing to do
}

bool VoxelPacket::appendBitMask(unsigned char bitmask) {
    bool success = false;
    if (_bytesAvailable > 0) {
        _buffer[_bytesInUse] = bitmask;
        _bytesInUse++;
        _bytesAvailable--; 
        success = true;
    }
    return success;
}

bool VoxelPacket::appendColor(rgbColor color) {
    // eventually we can make this use a dictionary...
    bool success = false;
    const int BYTES_PER_COLOR = 3;
    if (_bytesAvailable > BYTES_PER_COLOR) {
        append(color[0]);
        append(color[1]);
        append(color[2]);
        success = true;
    }
    return success;
}


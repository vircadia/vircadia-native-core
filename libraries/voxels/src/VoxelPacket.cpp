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
    _uncompressedData.clear();
    
    //printf("VoxelPacket::reset()... ");
    //debugCompareBuffers();
}

VoxelPacket::~VoxelPacket() {
}

bool VoxelPacket::append(const unsigned char* data, int length) {
    bool success = false;

    if (length <= _bytesAvailable) {
        int priorBytesInUse = _bytesInUse;

        memcpy(&_buffer[_bytesInUse], data, length);
        //QByteArray newData((const char*)data, length);
        //_uncompressedData.append(newData);
        
        _uncompressedData.resize(_bytesInUse+length);
        char* writeable = _uncompressedData.data();
        memcpy(&writeable[_bytesInUse], data, length);

        _bytesInUse += length;
        _bytesAvailable -= length;
        success = true;


        //printf("VoxelPacket::append(data, length=%d... priorBytesInUse=%d)... ", length, priorBytesInUse);
        debugCompareBuffers();

    }
    return success;
}

bool VoxelPacket::append(unsigned char byte) {
    bool success = false;
    if (_bytesAvailable > 0) {
        _buffer[_bytesInUse] = byte;
        //_uncompressedData.append(byte);
        _uncompressedData[_bytesInUse] = byte;
        
        _bytesInUse++;
        _bytesAvailable--; 
        success = true;

        //printf("VoxelPacket::append(byte)... ");
        debugCompareBuffers();

    }
    return success;
}

bool VoxelPacket::updatePriorBitMask(int offset, unsigned char bitmask) {
    bool success = false;
    if (offset >= 0 && offset < _bytesInUse) {
        _buffer[offset] = bitmask;
        _uncompressedData.replace(offset, sizeof(bitmask), (const char*)&bitmask, sizeof(bitmask));
        success = true;

        //printf("VoxelPacket::updatePriorBitMask()... ");
        debugCompareBuffers();

    }
    return success;
}

bool VoxelPacket::updatePriorBytes(int offset, const unsigned char* replacementBytes, int length) {
    bool success = false;
    if (length >= 0 && offset >= 0 && ((offset + length) <= _bytesInUse)) {
        memcpy(&_buffer[offset], replacementBytes, length);
        _uncompressedData.replace(offset, length, (const char*)replacementBytes, length);
        success = true;

        //printf("VoxelPacket::updatePriorBytes(offset=%d length=%d)...", offset, length);
        debugCompareBuffers();

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
    printf("VoxelPacket::appendBitMask()...\n");
    return append(bitmask);
}

bool VoxelPacket::appendColor(const nodeColor& color) {
    printf("VoxelPacket::appendColor()...\n");
    // eventually we can make this use a dictionary...
    bool success = false;
    const int BYTES_PER_COLOR = 3;
    if (_bytesAvailable > BYTES_PER_COLOR) {
        append(color[RED_INDEX]);
        append(color[GREEN_INDEX]);
        append(color[BLUE_INDEX]);
        success = true;
    }
    return success;
}

/***
void VoxelPacket::compressPacket() { 
    int uncompressedLength = getPacketLengthUncompressed();
    const int MAX_COMPRESSION = 9;
    // we only want to compress the data payload, not the message header
    int numBytesPacketHeader = numBytesForPacketHeader(_voxelPacket);
    QByteArray compressedData = qCompress(_voxelPacket+numBytesPacketHeader, 
                                    uncompressedLength-numBytesPacketHeader, MAX_COMPRESSION);
    _compressedPacket.clear();
    _compressedPacket.append((const char*)_voxelPacket, numBytesPacketHeader);
    _compressedPacket.append(compressedData);
}

void VoxelPacket::uncompressPacket() { 
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    QByteArray compressedData((const char*)packetData + numBytesPacketHeader, 
                            messageLength - numBytesPacketHeader);
    QByteArray uncompressedData = qUncompress(compressedData);
    QByteArray uncompressedPacket((const char*)packetData, numBytesPacketHeader);
    uncompressedPacket.append(uncompressedData);
    //app->_voxels.parseData((unsigned char*)uncompressedPacket.data(), uncompressedPacket.size());
}
***/

void VoxelPacket::debugCompareBuffers() const {
    const unsigned char* buffer = &_buffer[0];
    const unsigned char* byteArray = (const unsigned char*)_uncompressedData.constData();
    
    if (memcmp(buffer, byteArray, _bytesInUse) == 0) {
        //printf("VoxelPacket::debugCompareBuffers()... they match...\n");
        //printf("\n");
    } else {
        printf("VoxelPacket::debugCompareBuffers()... THEY DON'T MATCH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        printf("_buffer=");
        outputBufferBits(buffer, _bytesInUse);
        printf("_uncompressedData=");
        outputBufferBits(byteArray, _bytesInUse);
        printf("VoxelPacket::debugCompareBuffers()... THEY DON'T MATCH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    }
}




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
    _bytesAvailable = MAX_VOXEL_UNCOMRESSED_PACKET_SIZE;
    _subTreeAt = 0;
    _compressedBytes = 0;
}

VoxelPacket::~VoxelPacket() {
}

bool VoxelPacket::append(const unsigned char* data, int length) {
    bool success = false;

    if (length <= _bytesAvailable) {
        memcpy(&_uncompressed[_bytesInUse], data, length);
        _bytesInUse += length;
        _bytesAvailable -= length;
        
        // Now, check for compression, if we fit, then proceed, otherwise, rollback.
        if (checkCompress()) {
            success = true;
        } else {
            // rollback is easy, we just reset _bytesInUse and _bytesAvailable
            _bytesInUse -= length;
            _bytesAvailable += length;
        }
    }
    return success;
}

bool VoxelPacket::append(unsigned char byte) {
    bool success = false;
    if (_bytesAvailable > 0) {
        _uncompressed[_bytesInUse] = byte;
        _bytesInUse++;
        _bytesAvailable--; 

        // Now, check for compression, if we fit, then proceed, otherwise, rollback.
        if (checkCompress()) {
            success = true;
        } else {
            // rollback is easy, we just reset _bytesInUse and _bytesAvailable
            _bytesInUse--;
            _bytesAvailable++;
        }
    }
    return success;
}

bool VoxelPacket::updatePriorBitMask(int offset, unsigned char bitmask) {
    bool success = false;
    if (offset >= 0 && offset < _bytesInUse) {
        unsigned char oldValue = _uncompressed[offset];
        _uncompressed[offset] = bitmask;
        // Now, check for compression, if we fit, then proceed, otherwise, rollback.
        if (checkCompress()) {
            success = true;
        } else {
            // rollback is easy, the length didn't change, but we need to restore the previous value
            _uncompressed[offset] = oldValue;
        }
    }
    return success;
}

bool VoxelPacket::updatePriorBytes(int offset, const unsigned char* replacementBytes, int length) {
    bool success = false;
    if (length >= 0 && offset >= 0 && ((offset + length) <= _bytesInUse)) {
        unsigned char oldValues[length];
        memcpy(&oldValues[0], &_uncompressed[offset], length); // save the old values for restore
        memcpy(&_uncompressed[offset], replacementBytes, length); // copy new content

        // Now, check for compression, if we fit, then proceed, otherwise, rollback.
        if (checkCompress()) {
            success = true;
        } else {
            // rollback is easy, the length didn't change, but we need to restore the previous values
            memcpy(&_uncompressed[offset], &oldValues[0], length); // restore the old values
        }
    }
    return success;
}

bool VoxelPacket::startSubTree(const unsigned char* octcode) {
    bool success = false;
    int possibleStartAt = _bytesInUse;
    if (octcode) {
        int length = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octcode));
        success = append(octcode, length); // handles checking compression
    } else {
        // NULL case, means root node, which is 0
        unsigned char byte = 0;
        success = append(byte); // handles checking compression
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

    // we can be confident that it will compress, because we can't get here without previously being able to compress
    // the content up to this point in the uncompressed buffer. But we still call this because it cleans up the compressed
    // buffer with the correct content
    checkCompress(); 
}

int VoxelPacket::startLevel() {
    int key = _bytesInUse;
    return key;
}

void VoxelPacket::discardLevel(int key) {
    int bytesInLevel = _bytesInUse - key;
    _bytesInUse -= bytesInLevel;
    _bytesAvailable += bytesInLevel; 

    // we can be confident that it will compress, because we can't get here without previously being able to compress
    // the content up to this point in the uncompressed buffer. But we still call this because it cleans up the compressed
    // buffer with the correct content
    checkCompress(); 
}

void VoxelPacket::endLevel() {
    // nothing to do
}

bool VoxelPacket::appendBitMask(unsigned char bitmask) {
    return append(bitmask); // handles checking compression
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
    return success;
}

bool VoxelPacket::checkCompress() { 
    bool success = false;
    const int MAX_COMPRESSION = 9;

    // we only want to compress the data payload, not the message header
    QByteArray compressedData = qCompress(_uncompressed,_bytesInUse, MAX_COMPRESSION);
    if (compressedData.size() < MAX_VOXEL_PACKET_SIZE) {
        //memcpy(&_compressed[0], compressedData.constData(), compressedData.size());

        _compressedBytes = compressedData.size();
        for (int i = 0; i < _compressedBytes; i++) {
            _compressed[i] = compressedData[i];
        }
        
//printf("compressed %d bytes from %d original bytes\n", _compressedBytes, _bytesInUse);
        success = true;
    }
    return success;
}


void VoxelPacket::loadCompressedContent(const unsigned char* data, int length) {
    reset(); // by definition we reset upon loading compressed content

    if (length > 0) {
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
        printf("VoxelPacket::loadCompressedContent()... length = 0, nothing to do...\n");
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




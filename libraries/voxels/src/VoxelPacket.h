//
//  VoxelPacket.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013
//
//

#ifndef __hifi__VoxelPacket__
#define __hifi__VoxelPacket__

#include <SharedUtil.h>
#include "VoxelConstants.h"
#include "VoxelNode.h"

class VoxelPacket {
public:
    VoxelPacket();
    ~VoxelPacket();

    void reset();
    bool startSubTree(const unsigned char* octcode = NULL);
    void endSubTree();
    void discardSubTree();

    void startLevel();
    bool appendBitMask(unsigned char bitmask);
    bool appendColor(rgbColor color);
    void discardLevel();
    void endLevel();
    
    bool append(const unsigned char* data, int length); /// appends raw bytes
    bool append(unsigned char byte); /// append a single byte
    bool setByte(int offset, unsigned char byte); /// sets a single raw byte from previously appended portion of the stream

    //int getBytesInUse() const { return _bytesInUse; }
    //int getBytesAvailable() const { return _bytesAvailable; }

    unsigned char* getCompressedData() { return &_buffer[0]; } /// get pointer to start of compressed stream
    int getCompressedSize() const { return _bytesInUse; } /// the size of the packet in compressed form

    /// returns the offset of the next uncompressed byte to be written 
    int getNextByteUncompressed() const { return _bytesInUse; } 
    
    /// has some content been written to the packet
    bool hasContent() const { return (_bytesInUse > 0); }
    
    /// returns a byte offset from beginning of stream based on offset from end. 
    /// Positive offsetFromEnd returns that many bytes before the end
    int getByteOffset(int offsetFromEnd) const {
        return _bytesInUse - offsetFromEnd;
    }
    
    unsigned char* getStartOfBuffer() { return &_buffer[0]; } /// get pointer to the start of stream buffer
    unsigned char* getEndOfBuffer() { return &_buffer[_bytesInUse]; } /// get pointer to current end of stream buffer
    
    
private:
    unsigned char _buffer[MAX_VOXEL_PACKET_SIZE];
    int _bytesInUse;
    int _bytesAvailable;
    int _subTreeAt;
    int _levelAt;
};

#endif /* defined(__hifi__VoxelPacket__) */
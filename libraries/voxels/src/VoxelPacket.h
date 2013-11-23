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

    /// starts a level marker. returns an opaque key which can be used to discard the level
    int startLevel();

    /// discards all content back to a previous marker key
    void discardLevel(int key);
    
    /// ends a level without discarding it
    void endLevel();

    bool appendBitMask(unsigned char bitmask);

    /// updates the value of a bitmask from a previously appended portion of the uncompressed stream, might fail if the new 
    /// bitmask would cause packet to be less compressed, or if offset was out of range.
    bool updatePriorBitMask(int offset, unsigned char bitmask); 

    bool appendColor(rgbColor color);

    /// returns a byte offset from beginning of the uncompressed stream based on offset from end. 
    /// Positive offsetFromEnd returns that many bytes before the end of uncompressed stream
    int getUncompressedByteOffset(int offsetFromEnd = 0) const { return _bytesInUse - offsetFromEnd; }

    /// get access to the finalized data (it may be compressed or rewritten into optimal form)
    unsigned char* getFinalizedData() { return &_buffer[0]; }
    /// get size of the finalized data (it may be compressed or rewritten into optimal form)
    int getFinalizedSize() const { return _bytesInUse; }

    /// get pointer to the start of uncompressed stream buffer
    unsigned char* getUncompressedData() { return &_buffer[0]; }
    /// the size of the packet in uncompressed form
    int getUncompressedSize() { return _bytesInUse; }

    /// has some content been written to the packet
    bool hasContent() const { return (_bytesInUse > 0); }
    
    ////////////////////////////////////
    // XXXBHG: Questions...
    //  Slice Reshuffle...
    //
    //  1) getEndOfBuffer() is used by recursive slice shuffling... is there a safer API for that? This usage would probably
    //     break badly with compression... Especially since we do a memcpy into the uncompressed buffer, we'd need to
    //     add an "updateBytes()" method... which could definitely fail on compression.... It would also break any RLE we
    //     might implement, since the order of the colors would clearly change.
    //
    //  2) add stats tracking for number of bytes of octal code, bitmasks, and colors in a packet.
    unsigned char* getEndOfBuffer() { return &_buffer[_bytesInUse]; } /// get pointer to current end of stream buffer
    
private:
    /// appends raw bytes, might fail if byte would cause packet to be too large
    bool append(const unsigned char* data, int length);
    
    /// append a single byte, might fail if byte would cause packet to be too large
    bool append(unsigned char byte);

    unsigned char _buffer[MAX_VOXEL_PACKET_SIZE];
    int _bytesInUse;
    int _bytesAvailable;
    int _subTreeAt;
    int _levelAt;
};

#endif /* defined(__hifi__VoxelPacket__) */
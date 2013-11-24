//
//  VoxelPacket.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013
//
//  TO DO:
//
//    *  add stats tracking for number of bytes of octal code, bitmasks, and colors in a packet.
//    *  add compression
//    *  improve semantics for "reshuffle" - current approach will work for now and with compression
//       but wouldn't work with RLE because the colors in the levels would get reordered and RLE would need
//       to be recalculated
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

    /// reset completely, all data is discarded
    void reset();
    
    /// call to begin encoding a subtree starting at this point, this will append the octcode to the uncompressed stream
    /// at this point. May fail if new datastream is too long. In failure case the stream remains in it's previous state.
    bool startSubTree(const unsigned char* octcode = NULL);
    
    // call to indicate that the current subtree is complete and changes should be committed.
    void endSubTree();

    // call rollback the current subtree and restore the stream to the state prior to starting the subtree encoding
    void discardSubTree();

    /// starts a level marker. returns an opaque key which can be used to discard the level
    int startLevel();

    /// discards all content back to a previous marker key
    void discardLevel(int key);
    
    /// ends a level without discarding it
    void endLevel();

    /// appends a bitmask to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendBitMask(unsigned char bitmask);

    /// updates the value of a bitmask from a previously appended portion of the uncompressed stream, might fail if the new 
    /// bitmask would cause packet to be less compressed, or if offset was out of range.
    bool updatePriorBitMask(int offset, unsigned char bitmask); 

    /// updates the uncompressed content of the stream starting at byte offset with replacementBytes for length.
    /// Might fail if the new bytes would cause packet to be less compressed, or if offset and length was out of range.
    bool updatePriorBytes(int offset, const unsigned char* replacementBytes, int length);

    /// appends a color to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendColor(const nodeColor& color);

    /// returns a byte offset from beginning of the uncompressed stream based on offset from end. 
    /// Positive offsetFromEnd returns that many bytes before the end of uncompressed stream
    int getUncompressedByteOffset(int offsetFromEnd = 0) const { return _bytesInUse - offsetFromEnd; }

    /// get access to the finalized data (it may be compressed or rewritten into optimal form)
    unsigned char* getFinalizedData() { return &_buffer[0]; }
    /// get size of the finalized data (it may be compressed or rewritten into optimal form)
    int getFinalizedSize() const { return _bytesInUse; }

    /// get pointer to the start of uncompressed stream buffer
    const unsigned char* getUncompressedData() { return &_buffer[0]; }
    /// the size of the packet in uncompressed form
    int getUncompressedSize() { return _bytesInUse; }

    /// has some content been written to the packet
    bool hasContent() const { return (_bytesInUse > 0); }
    
private:
    /// appends raw bytes, might fail if byte would cause packet to be too large
    bool append(const unsigned char* data, int length);
    
    /// append a single byte, might fail if byte would cause packet to be too large
    bool append(unsigned char byte);

    unsigned char _buffer[MAX_VOXEL_PACKET_SIZE];
    int _bytesInUse;
    int _bytesAvailable;
    int _subTreeAt;
};

#endif /* defined(__hifi__VoxelPacket__) */
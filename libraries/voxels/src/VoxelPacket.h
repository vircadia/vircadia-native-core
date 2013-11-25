//
//  VoxelPacket.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013
//
//  TO DO:
//
//    *  add stats tracking for number of bytes of octal code, bitmasks, and colors in a packet.
//
//    *  determine why we sometimes don't fill packets very well (rarely) mid-scene... sometimes it appears as if
//       the "next node" would encode with more bytes than can fit in the remainder of the packet. this might be
//       several tens or hundreds of bytes, but theoretically other voxels would have fit. This happens in the 0100
//       scene a couple times.  
//          this is happening because of nodes that are not recursed for good reason like:
//              - being occluded
//              - being previously in view
//              - being out of view, etc.
//          in these cases, the node is not re-added to the bag... so, we can probably just keep going...
//
//    *  further testing of compression to determine optimal configuration for performance and compression
//    *  improve semantics for "reshuffle" - current approach will work for now and with compression
//       but wouldn't work with RLE because the colors in the levels would get reordered and RLE would need
//       to be recalculated
//

#ifndef __hifi__VoxelPacket__
#define __hifi__VoxelPacket__

#include <SharedUtil.h>
#include "VoxelConstants.h"
#include "VoxelNode.h"

const int MAX_VOXEL_PACKET_SIZE = MAX_PACKET_SIZE - (sizeof(PACKET_TYPE) + sizeof(PACKET_VERSION));
const int MAX_VOXEL_UNCOMRESSED_PACKET_SIZE = 4500;
const int VOXEL_PACKET_ALWAYS_TEST_COMPRESSED_THRESHOLD = 1400;
const int VOXEL_PACKET_TEST_UNCOMPRESSED_THRESHOLD = 4000;
const int VOXEL_PACKET_TEST_UNCOMPRESSED_CHANGE_THRESHOLD = 20;
const int VOXEL_PACKET_COMPRESSION_DEFAULT = false;

class VoxelPacket {
public:
    VoxelPacket(bool enableCompression = false, int maxFinalizedSize = MAX_VOXEL_PACKET_SIZE);
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
    
    /// ends a level, and performs any expensive finalization. may fail if finalization creates a stream which is too large
    /// if the finalization would fail, the packet will automatically discard the previous level.
    bool endLevel(int key);

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
    const unsigned char* getFinalizedData();
    /// get size of the finalized data (it may be compressed or rewritten into optimal form)
    int getFinalizedSize();

    /// get pointer to the start of uncompressed stream buffer
    const unsigned char* getUncompressedData() { return &_uncompressed[0]; }
    /// the size of the packet in uncompressed form
    int getUncompressedSize() { return _bytesInUse; }

    /// has some content been written to the packet
    bool hasContent() const { return (_bytesInUse > 0); }

    /// load finalized content to allow access to decoded content for parsing
    void loadFinalizedContent(const unsigned char* data, int length);

    void debugContent();

    static uint64_t _checkCompressTime;
    static uint64_t _checkCompressCalls;
     
private:
    /// appends raw bytes, might fail if byte would cause packet to be too large
    bool append(const unsigned char* data, int length);
    
    /// append a single byte, might fail if byte would cause packet to be too large
    bool append(unsigned char byte);

    bool _enableCompression;
    int _maxFinalizedSize;

    unsigned char _uncompressed[MAX_VOXEL_UNCOMRESSED_PACKET_SIZE];
    int _bytesInUse;
    int _bytesAvailable;
    int _subTreeAt;

    bool checkCompress();
    
    unsigned char _compressed[MAX_VOXEL_UNCOMRESSED_PACKET_SIZE];
    int _compressedBytes;
    int _bytesInUseLastCheck;
    bool _dirty;
    
    static bool _debug;
};

#endif /* defined(__hifi__VoxelPacket__) */
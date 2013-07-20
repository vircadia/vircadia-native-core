//
//  VoxelSceneStats.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//
//

#ifndef __hifi__VoxelSceneStats__
#define __hifi__VoxelSceneStats__

#include <stdint.h>
#include <NodeList.h>

class VoxelNode;

class VoxelSceneStats {
public:
    VoxelSceneStats();
    ~VoxelSceneStats();
    void reset();
    void sceneStarted(bool fullScene, bool moving);
    void sceneCompleted();

    void printDebugDetails();
    void packetSent(int bytes);

    void encodeStarted();
    void encodeStopped();
    
    void traversed(const VoxelNode* node);
    void skippedDistance(const VoxelNode* node);
    void skippedOutOfView(const VoxelNode* node);
    void skippedWasInView(const VoxelNode* node);
    void skippedNoChange(const VoxelNode* node);
    void skippedOccluded(const VoxelNode* node);
    void colorSent(const VoxelNode* node);
    void didntFit(const VoxelNode* node);
    void colorBitsWritten();
    void existsBitsWritten();
    void existsInPacketBitsWritten();
    void childBitsRemoved(bool includesExistsBits, bool includesColors);

    int packIntoMessage(unsigned char* destinationBuffer, int availableBytes);
    int unpackFromMessage(unsigned char* sourceBuffer, int availableBytes);

    bool readyToSend() const { return _readyToSend; }
    void markAsSent() { _readyToSend = false; }
    unsigned char* getStatsMessage() { return &_statsMessage[0]; }
    int getStatsMessageLength() const { return _statsMessageLength; }

    enum {
        ITEM_ELAPSED,
        ITEM_ENCODE,
        ITEM_PACKETS,
        ITEM_VOXELS,
        ITEM_COLORS,
        ITEM_BITS,
        ITEM_TRAVERSED,
        ITEM_SKIPPED,
        ITEM_SKIPPED_DISTANCE,
        ITEM_SKIPPED_OUT_OF_VIEW,
        ITEM_SKIPPED_WAS_IN_VIEW,
        ITEM_SKIPPED_NO_CHANGE,
        ITEM_SKIPPED_OCCLUDED,
        ITEM_DIDNT_FIT,
        ITEM_MODE,
        ITEM_COUNT
    };

    // Meta information about each stats item
    struct ItemInfo {
        char const* const   caption;
        unsigned            colorRGBA;
    };
    
    ItemInfo& getItemInfo(int item) { return _ITEMS[item]; };
    char* getItemValue(int item);
    
private:
    bool _readyToSend;
    unsigned char _statsMessage[MAX_PACKET_SIZE];
    int _statsMessageLength;

    // scene timing data in usecs
    uint64_t _start;
    uint64_t _end;
    uint64_t _elapsed;

    uint64_t _totalEncodeTime;
    uint64_t _encodeStart;
    
    // scene voxel related data
    unsigned long _traversed;
    unsigned long _internal;
    unsigned long _leaves;
    
    unsigned long _skippedDistance;
    unsigned long _internalSkippedDistance;
    unsigned long _leavesSkippedDistance;

    unsigned long _skippedOutOfView;
    unsigned long _internalSkippedOutOfView;
    unsigned long _leavesSkippedOutOfView;

    unsigned long _skippedWasInView;
    unsigned long _internalSkippedWasInView;
    unsigned long _leavesSkippedWasInView;

    unsigned long _skippedNoChange;
    unsigned long _internalSkippedNoChange;
    unsigned long _leavesSkippedNoChange;

    unsigned long _skippedOccluded;
    unsigned long _internalSkippedOccluded;
    unsigned long _leavesSkippedOccluded;

    unsigned long _colorSent;
    unsigned long _internalColorSent;
    unsigned long _leavesColorSent;

    unsigned long _didntFit;
    unsigned long _internalDidntFit;
    unsigned long _leavesDidntFit;

    unsigned long _colorBitsWritten;
    unsigned long _existsBitsWritten;
    unsigned long _existsInPacketBitsWritten;
    unsigned long _treesRemoved;

    // Accounting Notes:
    //
    // 1) number of voxels sent can be calculated as _colorSent + _colorBitsWritten. This works because each internal 
    //    node in a packet will have a _colorBitsWritten included for it and each "leaf" in the packet will have a
    //    _colorSent written for it. Note that these "leaf" nodes in the packets may not be actual leaves in the full
    //    tree, because LOD may cause us to send an average color for an internal node instead of recursing deeper to
    //    the leaves.
    //
    // 2) the stats balance if: (working assumption) 
    //     if _colorSent > 0 
    //          _traversed = all skipped + _colorSent + _colorBitsWritten
    //     else
    //          _traversed = all skipped + _colorSent + _colorBitsWritten  + _treesRemoved
    //
    
    // scene network related data
    unsigned int  _packets;
    unsigned long _bytes;
    unsigned int  _passes;
    
    // features related items
    bool _moving;
    bool _fullSceneDraw;


    static ItemInfo _ITEMS[];
    static int const MAX_ITEM_VALUE_LENGTH = 128;
    char _itemValueBuffer[MAX_ITEM_VALUE_LENGTH];
};

#endif /* defined(__hifi__VoxelSceneStats__) */

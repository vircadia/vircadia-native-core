//
//  VoxelSceneStats.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__VoxelSceneStats__
#define __hifi__VoxelSceneStats__

#include <stdint.h>
#include <NodeList.h>
#include "JurisdictionMap.h"

class VoxelNode;

/// Collects statistics for calculating and sending a scene from a voxel server to an interface client
class VoxelSceneStats {
public:
    VoxelSceneStats();
    ~VoxelSceneStats();
    void reset();
    
    /// Call when beginning the computation of a scene. Initializes internal structures
    void sceneStarted(bool fullScene, bool moving, VoxelNode* root, JurisdictionMap* jurisdictionMap);

    /// Call when the computation of a scene is completed. Finalizes internal structures
    void sceneCompleted();

    void printDebugDetails();
    
    /// Track that a packet was sent as part of the scene.
    void packetSent(int bytes);

    /// Tracks the beginning of an encode pass during scene calculation.
    void encodeStarted();

    /// Tracks the ending of an encode pass during scene calculation.
    void encodeStopped();
    
    /// Track that a node was traversed as part of computation of a scene.
    void traversed(const VoxelNode* node);

    /// Track that a node was skipped as part of computation of a scene due to being beyond the LOD distance.
    void skippedDistance(const VoxelNode* node);

    /// Track that a node was skipped as part of computation of a scene due to being out of view.
    void skippedOutOfView(const VoxelNode* node);

    /// Track that a node was skipped as part of computation of a scene due to previously being in view while in delta sending
    void skippedWasInView(const VoxelNode* node);

    /// Track that a node was skipped as part of computation of a scene due to not having changed since last full scene sent
    void skippedNoChange(const VoxelNode* node);

    /// Track that a node was skipped as part of computation of a scene due to being occluded
    void skippedOccluded(const VoxelNode* node);

    /// Track that a node's color was was sent as part of computation of a scene
    void colorSent(const VoxelNode* node);

    /// Track that a node was due to be sent, but didn't fit in the packet and was moved to next packet
    void didntFit(const VoxelNode* node);

    /// Track that the color bitmask was was sent as part of computation of a scene
    void colorBitsWritten();

    /// Track that the exists in tree bitmask was was sent as part of computation of a scene
    void existsBitsWritten();

    /// Track that the exists in packet bitmask was was sent as part of computation of a scene
    void existsInPacketBitsWritten();

    /// Fix up tracking statistics in case where bitmasks were removed for some reason
    void childBitsRemoved(bool includesExistsBits, bool includesColors);

    /// Pack the details of the statistics into a buffer for sending as a network packet
    int packIntoMessage(unsigned char* destinationBuffer, int availableBytes);

    /// Unpack the details of the statistics from a buffer typically received as a network packet
    int unpackFromMessage(unsigned char* sourceBuffer, int availableBytes);

    /// Indicates that a scene has been completed and the statistics are ready to be sent
    bool isReadyToSend() const { return _isReadyToSend; }

    /// Mark that the scene statistics have been sent
    void markAsSent() { _isReadyToSend = false; }

    unsigned char* getStatsMessage() { return &_statsMessage[0]; }
    int getStatsMessageLength() const { return _statsMessageLength; }

    /// List of various items tracked by VoxelSceneStats which can be accessed via getItemInfo() and getItemValue()
    enum Item {
        ITEM_ELAPSED,
        ITEM_ENCODE,
        ITEM_PACKETS,
        ITEM_VOXELS_SERVER,
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

    /// Meta information about each stats item
    struct ItemInfo {
        char const* const   caption;
        unsigned            colorRGBA;
    };
    
    /// Returns details about items tracked by VoxelSceneStats
    /// \param Item item The item from the stats you're interested in.
    ItemInfo& getItemInfo(Item item) { return _ITEMS[item]; }

    /// Returns a UI formatted value of an item tracked by VoxelSceneStats
    /// \param Item item The item from the stats you're interested in.
    char* getItemValue(Item item);
    
    /// Returns OctCode for root node of the jurisdiction of this particular voxel server
    unsigned char* getJurisdictionRoot() const { return _jurisdictionRoot; }

    /// Returns list of OctCodes for end nodes of the jurisdiction of this particular voxel server
    const std::vector<unsigned char*>& getJurisdictionEndNodes() const { return _jurisdictionEndNodes; }
    
private:
    bool _isReadyToSend;
    unsigned char _statsMessage[MAX_PACKET_SIZE];
    int _statsMessageLength;

    // scene timing data in usecs
    bool     _isStarted;
    uint64_t _start;
    uint64_t _end;
    uint64_t _elapsed;
    
    SimpleMovingAverage _elapsedAverage;
    SimpleMovingAverage _bitsPerVoxelAverage;

    uint64_t _totalEncodeTime;
    uint64_t _encodeStart;
    
    // scene voxel related data
    unsigned long _totalVoxels;
    unsigned long _totalInternal;
    unsigned long _totalLeaves;

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
    bool _isMoving;
    bool _isFullScene;


    static ItemInfo _ITEMS[];
    static int const MAX_ITEM_VALUE_LENGTH = 128;
    char _itemValueBuffer[MAX_ITEM_VALUE_LENGTH];
    
    unsigned char* _jurisdictionRoot;
    std::vector<unsigned char*> _jurisdictionEndNodes;
};

#endif /* defined(__hifi__VoxelSceneStats__) */

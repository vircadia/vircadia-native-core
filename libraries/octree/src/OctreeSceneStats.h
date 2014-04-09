//
//  OctreeSceneStats.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 7/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__OctreeSceneStats__
#define __hifi__OctreeSceneStats__

#include <stdint.h>
#include <NodeList.h>
#include <SharedUtil.h>
#include "JurisdictionMap.h"

#define GREENISH  0x40ff40d0
#define YELLOWISH 0xffef40c0
#define GREYISH 0xd0d0d0a0

class OctreeElement;

/// Collects statistics for calculating and sending a scene from a octree server to an interface client
class OctreeSceneStats {
public:
    OctreeSceneStats();
    ~OctreeSceneStats();
    void reset();
    
    OctreeSceneStats(const OctreeSceneStats& other); // copy constructor
    OctreeSceneStats& operator= (const OctreeSceneStats& other); // copy assignment
    
    /// Call when beginning the computation of a scene. Initializes internal structures
    void sceneStarted(bool fullScene, bool moving, OctreeElement* root, JurisdictionMap* jurisdictionMap);
    bool getIsSceneStarted() const { return _isStarted; }

    /// Call when the computation of a scene is completed. Finalizes internal structures
    void sceneCompleted();

    void printDebugDetails();
    
    /// Track that a packet was sent as part of the scene.
    void packetSent(int bytes);

    /// Tracks the beginning of an encode pass during scene calculation.
    void encodeStarted();

    /// Tracks the ending of an encode pass during scene calculation.
    void encodeStopped();
    
    /// Track that a element was traversed as part of computation of a scene.
    void traversed(const OctreeElement* element);

    /// Track that a element was skipped as part of computation of a scene due to being beyond the LOD distance.
    void skippedDistance(const OctreeElement* element);

    /// Track that a element was skipped as part of computation of a scene due to being out of view.
    void skippedOutOfView(const OctreeElement* element);

    /// Track that a element was skipped as part of computation of a scene due to previously being in view while in delta sending
    void skippedWasInView(const OctreeElement* element);

    /// Track that a element was skipped as part of computation of a scene due to not having changed since last full scene sent
    void skippedNoChange(const OctreeElement* element);

    /// Track that a element was skipped as part of computation of a scene due to being occluded
    void skippedOccluded(const OctreeElement* element);

    /// Track that a element's color was was sent as part of computation of a scene
    void colorSent(const OctreeElement* element);

    /// Track that a element was due to be sent, but didn't fit in the packet and was moved to next packet
    void didntFit(const OctreeElement* element);

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
    int unpackFromMessage(const unsigned char* sourceBuffer, int availableBytes);

    /// Indicates that a scene has been completed and the statistics are ready to be sent
    bool isReadyToSend() const { return _isReadyToSend; }

    /// Mark that the scene statistics have been sent
    void markAsSent() { _isReadyToSend = false; }

    unsigned char* getStatsMessage() { return &_statsMessage[0]; }
    int getStatsMessageLength() const { return _statsMessageLength; }

    /// List of various items tracked by OctreeSceneStats which can be accessed via getItemInfo() and getItemValue()
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
        char const* const caption;
        unsigned colorRGBA;
        int detailsCount;
        const char* detailsLabels;
    };
    
    /// Returns details about items tracked by OctreeSceneStats
    /// \param Item item The item from the stats you're interested in.
    ItemInfo& getItemInfo(Item item) { return _ITEMS[item]; }

    /// Returns a UI formatted value of an item tracked by OctreeSceneStats
    /// \param Item item The item from the stats you're interested in.
    const char* getItemValue(Item item);

    /// Returns OctCode for root element of the jurisdiction of this particular octree server
    unsigned char* getJurisdictionRoot() const { return _jurisdictionRoot; }

    /// Returns list of OctCodes for end elements of the jurisdiction of this particular octree server
    const std::vector<unsigned char*>& getJurisdictionEndNodes() const { return _jurisdictionEndNodes; }
    
    bool isMoving() const { return _isMoving; };
    unsigned long getTotalElements() const { return _totalElements; }
    unsigned long getTotalInternal() const { return _totalInternal; }
    unsigned long getTotalLeaves() const { return _totalLeaves; }
    unsigned long getTotalEncodeTime() const { return _totalEncodeTime; }
    unsigned long getElapsedTime() const { return _elapsed; }

    unsigned long getLastFullElapsedTime() const { return _lastFullElapsed; }
    unsigned long getLastFullTotalEncodeTime() const { return _lastFullTotalEncodeTime; }
    unsigned int getLastFullTotalPackets() const { return _lastFullTotalPackets; }
    unsigned long getLastFullTotalBytes() const { return _lastFullTotalBytes; }

    // Used in client implementations to track individual octree packets
    void trackIncomingOctreePacket(const QByteArray& packet, bool wasStatsPacket, int nodeClockSkewUsec);

    unsigned int getIncomingPackets() const { return _incomingPacket; }
    unsigned long getIncomingBytes() const { return _incomingBytes; } 
    unsigned long getIncomingWastedBytes() const { return _incomingWastedBytes; }
    unsigned int getIncomingOutOfOrder() const { return _incomingLate + _incomingEarly; }
    unsigned int getIncomingLikelyLost() const { return _incomingLikelyLost; }
    unsigned int getIncomingRecovered() const { return _incomingRecovered; }
    unsigned int getIncomingEarly() const { return _incomingEarly; }
    unsigned int getIncomingLate() const { return _incomingLate; }
    unsigned int getIncomingReallyLate() const { return _incomingReallyLate; }
    unsigned int getIncomingPossibleDuplicate() const { return _incomingPossibleDuplicate; }
    float getIncomingFlightTimeAverage() { return _incomingFlightTimeAverage.getAverage(); }

private:

    void copyFromOther(const OctreeSceneStats& other);

    bool _isReadyToSend;
    unsigned char _statsMessage[MAX_PACKET_SIZE];
    int _statsMessageLength;

    // scene timing data in usecs
    bool _isStarted;
    quint64 _start;
    quint64 _end;
    quint64 _elapsed;

    quint64 _lastFullElapsed;
    quint64 _lastFullTotalEncodeTime;
    unsigned int  _lastFullTotalPackets;
    unsigned long _lastFullTotalBytes;
    
    SimpleMovingAverage _elapsedAverage;
    SimpleMovingAverage _bitsPerOctreeAverage;

    quint64 _totalEncodeTime;
    quint64 _encodeStart;
    
    // scene octree related data
    unsigned long _totalElements;
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
    // 1) number of octrees sent can be calculated as _colorSent + _colorBitsWritten. This works because each internal 
    //    element in a packet will have a _colorBitsWritten included for it and each "leaf" in the packet will have a
    //    _colorSent written for it. Note that these "leaf" elements in the packets may not be actual leaves in the full
    //    tree, because LOD may cause us to send an average color for an internal element instead of recursing deeper to
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
    
    // incoming packets stats
    unsigned int _incomingPacket;
    unsigned long _incomingBytes;
    unsigned long _incomingWastedBytes;

    uint16_t _incomingLastSequence; /// last incoming sequence number
    unsigned int _incomingLikelyLost; /// count of packets likely lost, may be off by _incomingReallyLate count
    unsigned int _incomingRecovered; /// packets that were late, and we had in our missing list, we consider recovered
    unsigned int _incomingEarly; /// out of order earlier than expected
    unsigned int _incomingLate; /// out of order later than expected
    unsigned int _incomingReallyLate; /// out of order and later than MAX_MISSING_SEQUENCE_OLD_AGE late
    unsigned int _incomingPossibleDuplicate; /// out of order possibly a duplicate
    QSet<uint16_t> _missingSequenceNumbers;
    SimpleMovingAverage _incomingFlightTimeAverage;
    
    // features related items
    bool _isMoving;
    bool _isFullScene;


    static ItemInfo _ITEMS[];
    static int const MAX_ITEM_VALUE_LENGTH = 128;
    char _itemValueBuffer[MAX_ITEM_VALUE_LENGTH];
    
    unsigned char* _jurisdictionRoot;
    std::vector<unsigned char*> _jurisdictionEndNodes;
};

/// Map between element IDs and their reported OctreeSceneStats. Typically used by classes that need to know which elements sent
/// which octree stats
typedef std::map<QUuid, OctreeSceneStats> NodeToOctreeSceneStats;
typedef std::map<QUuid, OctreeSceneStats>::iterator NodeToOctreeSceneStatsIterator;

#endif /* defined(__hifi__OctreeSceneStats__) */

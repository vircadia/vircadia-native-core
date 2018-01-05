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

#ifndef hifi_OctreeSceneStats_h
#define hifi_OctreeSceneStats_h

#include <stdint.h>

#include <NodeList.h>
#include <shared/ReadWriteLockable.h>

#include "JurisdictionMap.h"
#include "OctreePacketData.h"
#include "SequenceNumberStats.h"
#include "OctalCode.h"

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
    void sceneStarted(bool fullScene, bool moving, const OctreeElementPointer& root, JurisdictionMap* jurisdictionMap);
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
    void traversed(const OctreeElementPointer& element);

    /// Track that a element was skipped as part of computation of a scene due to being beyond the LOD distance.
    void skippedDistance(const OctreeElementPointer& element);

    /// Track that a element was skipped as part of computation of a scene due to being out of view.
    void skippedOutOfView(const OctreeElementPointer& element);

    /// Track that a element was skipped as part of computation of a scene due to previously being in view while in delta sending
    void skippedWasInView(const OctreeElementPointer& element);

    /// Track that a element was skipped as part of computation of a scene due to not having changed since last full scene sent
    void skippedNoChange(const OctreeElementPointer& element);

    /// Track that a element was skipped as part of computation of a scene due to being occluded
    void skippedOccluded(const OctreeElementPointer& element);

    /// Track that a element's color was was sent as part of computation of a scene
    void colorSent(const OctreeElementPointer& element);

    /// Track that a element was due to be sent, but didn't fit in the packet and was moved to next packet
    void didntFit(const OctreeElementPointer& element);

    /// Track that the color bitmask was was sent as part of computation of a scene
    void colorBitsWritten();

    /// Track that the exists in tree bitmask was was sent as part of computation of a scene
    void existsBitsWritten();

    /// Track that the exists in packet bitmask was was sent as part of computation of a scene
    void existsInPacketBitsWritten();

    /// Fix up tracking statistics in case where bitmasks were removed for some reason
    void childBitsRemoved(bool includesExistsBits);

    /// Pack the details of the statistics into a buffer for sending as a network packet
    int packIntoPacket();

    /// Unpack the details of the statistics from a network packet
    int unpackFromPacket(ReceivedMessage& packet);

    /// Indicates that a scene has been completed and the statistics are ready to be sent
    bool isReadyToSend() const { return _isReadyToSend; }

    /// Mark that the scene statistics have been sent
    void markAsSent() { _isReadyToSend = false; }

    NLPacket& getStatsMessage() { return *_statsPacket; }

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
    OctalCodePtr getJurisdictionRoot() const { return _jurisdictionRoot; }

    /// Returns list of OctCodes for end elements of the jurisdiction of this particular octree server
    const OctalCodePtrList& getJurisdictionEndNodes() const { return _jurisdictionEndNodes; }

    bool isMoving() const { return _isMoving; }
    bool isFullScene() const { return _isFullScene; }
    quint64 getTotalElements() const { return _totalElements; }
    quint64 getTotalInternal() const { return _totalInternal; }
    quint64 getTotalLeaves() const { return _totalLeaves; }
    quint64 getTotalEncodeTime() const { return _totalEncodeTime; }
    quint64 getElapsedTime() const { return _elapsed; }

    quint64 getLastFullElapsedTime() const { return _lastFullElapsed; }
    quint64 getLastFullTotalEncodeTime() const { return _lastFullTotalEncodeTime; }
    quint32 getLastFullTotalPackets() const { return _lastFullTotalPackets; }
    quint64 getLastFullTotalBytes() const { return _lastFullTotalBytes; }

    // Used in client implementations to track individual octree packets
    void trackIncomingOctreePacket(ReceivedMessage& message, bool wasStatsPacket, qint64 nodeClockSkewUsec);

    quint32 getIncomingPackets() const { return _incomingPacket; }
    quint64 getIncomingBytes() const { return _incomingBytes; }
    quint64 getIncomingWastedBytes() const { return _incomingWastedBytes; }
    float getIncomingFlightTimeAverage() { return _incomingFlightTimeAverage.getAverage(); }

    const SequenceNumberStats& getIncomingOctreeSequenceNumberStats() const { return _incomingOctreeSequenceNumberStats; }
    SequenceNumberStats& getIncomingOctreeSequenceNumberStats() { return _incomingOctreeSequenceNumberStats; }

private:

    void copyFromOther(const OctreeSceneStats& other);

    bool _isReadyToSend;

    std::unique_ptr<NLPacket> _statsPacket = NLPacket::create(PacketType::OctreeStats);

    // scene timing data in usecs
    bool _isStarted;
    quint64 _start;
    quint64 _end;
    quint64 _elapsed;

    quint64 _lastFullElapsed;
    quint64 _lastFullTotalEncodeTime;
    quint32  _lastFullTotalPackets;
    quint64 _lastFullTotalBytes;

    SimpleMovingAverage _elapsedAverage;
    SimpleMovingAverage _bitsPerOctreeAverage;

    quint64 _totalEncodeTime;
    quint64 _encodeStart;

    // scene octree related data
    quint64 _totalElements;
    quint64 _totalInternal;
    quint64 _totalLeaves;

    quint64 _traversed;
    quint64 _internal;
    quint64 _leaves;

    quint64 _skippedDistance;
    quint64 _internalSkippedDistance;
    quint64 _leavesSkippedDistance;

    quint64 _skippedOutOfView;
    quint64 _internalSkippedOutOfView;
    quint64 _leavesSkippedOutOfView;

    quint64 _skippedWasInView;
    quint64 _internalSkippedWasInView;
    quint64 _leavesSkippedWasInView;

    quint64 _skippedNoChange;
    quint64 _internalSkippedNoChange;
    quint64 _leavesSkippedNoChange;

    quint64 _skippedOccluded;
    quint64 _internalSkippedOccluded;
    quint64 _leavesSkippedOccluded;

    quint64 _colorSent;
    quint64 _internalColorSent;
    quint64 _leavesColorSent;

    quint64 _didntFit;
    quint64 _internalDidntFit;
    quint64 _leavesDidntFit;

    quint64 _colorBitsWritten;
    quint64 _existsBitsWritten;
    quint64 _existsInPacketBitsWritten;
    quint64 _treesRemoved;

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
    quint32  _packets;
    quint64 _bytes;
    quint32  _passes;

    // incoming packets stats
    quint32 _incomingPacket;
    quint64 _incomingBytes;
    quint64 _incomingWastedBytes;

    SequenceNumberStats _incomingOctreeSequenceNumberStats;

    SimpleMovingAverage _incomingFlightTimeAverage;

    // features related items
    bool _isMoving;
    bool _isFullScene;


    static ItemInfo _ITEMS[];
    static const int MAX_ITEM_VALUE_LENGTH = 128;
    char _itemValueBuffer[MAX_ITEM_VALUE_LENGTH];

    OctalCodePtr _jurisdictionRoot;
    std::vector<OctalCodePtr> _jurisdictionEndNodes;
};

/// Map between element IDs and their reported OctreeSceneStats. Typically used by classes that need to know which elements sent
/// which octree stats
class NodeToOctreeSceneStats : public std::map<QUuid, OctreeSceneStats>, public ReadWriteLockable {};
typedef NodeToOctreeSceneStats::iterator NodeToOctreeSceneStatsIterator;

#endif // hifi_OctreeSceneStats_h

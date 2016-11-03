//
//  OctreeQueryNode.h
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeQueryNode_h
#define hifi_OctreeQueryNode_h

#include <iostream>

#include <NodeData.h>
#include <OctreeConstants.h>
#include <OctreeElementBag.h>
#include <OctreePacketData.h>
#include <OctreeQuery.h>
#include <OctreeSceneStats.h>
#include "SentPacketHistory.h"
#include <qqueue.h>

class OctreeSendThread;
class OctreeServer;

class OctreeQueryNode : public OctreeQuery {
    Q_OBJECT
public:
    OctreeQueryNode() = default;
    virtual ~OctreeQueryNode() = default;

    void init(); // called after creation to set up some virtual items
    virtual PacketType getMyPacketType() const = 0;

    void resetOctreePacket();  // resets octree packet to after "V" header

    void writeToPacket(const unsigned char* buffer, unsigned int bytes); // writes to end of packet

    NLPacket& getPacket() const { return *_octreePacket; }
    bool isPacketWaiting() const { return _octreePacketWaiting; }

    bool packetIsDuplicate() const;
    bool shouldSuppressDuplicatePacket();

    unsigned int getAvailable() const { return _octreePacket->bytesAvailableForWrite(); }
    int getMaxSearchLevel() const { return _maxSearchLevel; }
    void resetMaxSearchLevel() { _maxSearchLevel = 1; }
    void incrementMaxSearchLevel() { _maxSearchLevel++; }

    int getMaxLevelReached() const { return _maxLevelReachedInLastSearch; }
    void setMaxLevelReached(int maxLevelReached) { _maxLevelReachedInLastSearch = maxLevelReached; }

    OctreeElementBag elementBag;
    OctreeElementExtraEncodeData extraEncodeData;

    void copyCurrentViewFrustum(ViewFrustum& viewOut) const;
    void copyLastKnownViewFrustum(ViewFrustum& viewOut) const;

    // These are not classic setters because they are calculating and maintaining state
    // which is set asynchronously through the network receive
    bool updateCurrentViewFrustum();
    void updateLastKnownViewFrustum();

    bool getViewSent() const { return _viewSent; }
    void setViewSent(bool viewSent);

    bool getViewFrustumChanging() const { return _viewFrustumChanging; }
    bool getViewFrustumJustStoppedChanging() const { return _viewFrustumJustStoppedChanging; }

    bool moveShouldDump() const;

    quint64 getLastTimeBagEmpty() const { return _lastTimeBagEmpty; }
    void setLastTimeBagEmpty() { _lastTimeBagEmpty = _sceneSendStartTime; }

    bool hasLodChanged() const { return _lodChanged; }

    OctreeSceneStats stats;

    void dumpOutOfView();

    quint64 getLastRootTimestamp() const { return _lastRootTimestamp; }
    void setLastRootTimestamp(quint64 timestamp) { _lastRootTimestamp = timestamp; }
    unsigned int getlastOctreePacketLength() const { return _lastOctreePacketLength; }
    int getDuplicatePacketCount() const { return _duplicatePacketCount; }

    void sceneStart(quint64 sceneSendStartTime) { _sceneSendStartTime = sceneSendStartTime; }

    void nodeKilled();
    bool isShuttingDown() const { return _isShuttingDown; }

    void octreePacketSent() { packetSent(*_octreePacket); }
    void packetSent(const NLPacket& packet);

    OCTREE_PACKET_SEQUENCE getSequenceNumber() const { return _sequenceNumber; }

    void parseNackPacket(ReceivedMessage& message);
    bool hasNextNackedPacket() const;
    const NLPacket* getNextNackedPacket();

private:
    OctreeQueryNode(const OctreeQueryNode &);
    OctreeQueryNode& operator= (const OctreeQueryNode&);

    bool _viewSent { false };
    std::unique_ptr<NLPacket> _octreePacket;
    bool _octreePacketWaiting;

    unsigned int _lastOctreePacketLength { 0 };
    int _duplicatePacketCount { 0 };
    quint64 _firstSuppressedPacket { usecTimestampNow() };

    int _maxSearchLevel { 1 };
    int _maxLevelReachedInLastSearch { 1 };

    mutable QMutex _viewMutex { QMutex::Recursive };
    ViewFrustum _currentViewFrustum;
    ViewFrustum _lastKnownViewFrustum;
    quint64 _lastTimeBagEmpty { 0 };
    bool _viewFrustumChanging { false };
    bool _viewFrustumJustStoppedChanging { true };

    OctreeSendThread* _octreeSendThread { nullptr };

    // watch for LOD changes
    int _lastClientBoundaryLevelAdjust { 0 };
    float _lastClientOctreeSizeScale { DEFAULT_OCTREE_SIZE_SCALE };
    bool _lodChanged { false };
    bool _lodInitialized { false };

    OCTREE_PACKET_SEQUENCE _sequenceNumber { 0 };

    quint64 _lastRootTimestamp { 0 };

    PacketType _myPacketType { PacketType::Unknown };
    bool _isShuttingDown { false };

    SentPacketHistory _sentPacketHistory;
    QQueue<OCTREE_PACKET_SEQUENCE> _nackedSequenceNumbers;

    quint64 _sceneSendStartTime = 0;

    std::array<char, udt::MAX_PACKET_SIZE> _lastOctreePayload;
};

#endif // hifi_OctreeQueryNode_h

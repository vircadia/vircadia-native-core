//
//  OctreeSendThread.h
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Threaded or non-threaded object for sending octree data packets to a client
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeSendThread_h
#define hifi_OctreeSendThread_h

#include <atomic>

#include <GenericThread.h>

class OctreeQueryNode;
class OctreeServer;

using AtomicUIntStat = std::atomic<uintmax_t>;

/// Threaded processor for sending octree packets to a single client
class OctreeSendThread : public GenericThread {
    Q_OBJECT
public:
    OctreeSendThread(OctreeServer* myServer, const SharedNodePointer& node);
    virtual ~OctreeSendThread();

    void setIsShuttingDown();
    bool isShuttingDown() { return _isShuttingDown; }
    
    QUuid getNodeUuid() const { return _nodeUuid; }

    static AtomicUIntStat _totalBytes;
    static AtomicUIntStat _totalWastedBytes;
    static AtomicUIntStat _totalPackets;

    static AtomicUIntStat _totalSpecialBytes;
    static AtomicUIntStat _totalSpecialPackets;

    static AtomicUIntStat _usleepTime;
    static AtomicUIntStat _usleepCalls;

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process() override;

private:
    int handlePacketSend(SharedNodePointer node, OctreeQueryNode* nodeData, int& trueBytesSent, int& truePacketsSent, bool dontSuppressDuplicate = false);
    int packetDistributor(SharedNodePointer node, OctreeQueryNode* nodeData, bool viewFrustumChanged);
    
    
    OctreeServer* _myServer { nullptr };
    QWeakPointer<Node> _node;
    QUuid _nodeUuid;

    OctreePacketData _packetData;

    int _nodeMissingCount { 0 };
    bool _isShuttingDown { false };
};

#endif // hifi_OctreeSendThread_h

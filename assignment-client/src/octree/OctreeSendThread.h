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
#include <Node.h>
#include <OctreePacketData.h>
#include "OctreeQueryNode.h"

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

    virtual void traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData,
            bool viewFrustumChanged, bool isFullScene);
    virtual bool traverseTreeAndBuildNextPacketPayload(EncodeBitstreamParams& params, const QJsonObject& jsonFilters);

    OctreePacketData _packetData;
    QWeakPointer<Node> _node;
    OctreeServer* _myServer { nullptr };

private:
    /// Called before a packetDistributor pass to allow for pre-distribution processing
    virtual void preDistributionProcessing() {};
    int handlePacketSend(SharedNodePointer node, OctreeQueryNode* nodeData, bool dontSuppressDuplicate = false);
    int packetDistributor(SharedNodePointer node, OctreeQueryNode* nodeData, bool viewFrustumChanged);

    virtual bool hasSomethingToSend(OctreeQueryNode* nodeData) { return !nodeData->elementBag.isEmpty(); }
    virtual bool shouldStartNewTraversal(OctreeQueryNode* nodeData, bool viewFrustumChanged) { return viewFrustumChanged || !hasSomethingToSend(nodeData); }
    virtual void preStartNewScene(OctreeQueryNode* nodeData, bool isFullScene);
    virtual bool shouldTraverseAndSend(OctreeQueryNode* nodeData) { return hasSomethingToSend(nodeData); }

    QUuid _nodeUuid;

    int _truePacketsSent { 0 }; // available for debug stats
    int _trueBytesSent { 0 }; // available for debug stats
    int _packetsSentThisInterval { 0 }; // used for bandwidth throttle condition
    bool _isShuttingDown { false };
};

#endif // hifi_OctreeSendThread_h

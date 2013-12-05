//
//  OctreeSendThread.h
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded object for sending voxels to a client
//

#ifndef __octree_server__OctreeSendThread__
#define __octree_server__OctreeSendThread__

#include <GenericThread.h>
#include <NetworkPacket.h>
#include <OctreeElementBag.h>
#include "OctreeQueryNode.h"
#include "OctreeServer.h"

/// Threaded processor for sending voxel packets to a single client
class OctreeSendThread : public virtual GenericThread {
public:
    OctreeSendThread(const QUuid& nodeUUID, OctreeServer* myServer);

    static uint64_t _totalBytes;
    static uint64_t _totalWastedBytes;
    static uint64_t _totalPackets;

    static uint64_t _usleepTime;
    static uint64_t _usleepCalls;

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();

private:
    QUuid _nodeUUID;
    OctreeServer* _myServer;

    int handlePacketSend(Node* node, OctreeQueryNode* nodeData, int& trueBytesSent, int& truePacketsSent);
    int packetDistributor(Node* node, OctreeQueryNode* nodeData, bool viewFrustumChanged);

    OctreePacketData _packetData;
};

#endif // __octree_server__OctreeSendThread__

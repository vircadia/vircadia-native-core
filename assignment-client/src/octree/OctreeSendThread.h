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


class OctreeServer;
class OctreeQueryNode;


#include "OctreeQueryNode.h"
#include "OctreeServer.h"

/// Threaded processor for sending voxel packets to a single client
class OctreeSendThread : public GenericThread {
    Q_OBJECT
public:
    OctreeSendThread(const SharedAssignmentPointer& myAssignment, const SharedNodePointer& node);
    virtual ~OctreeSendThread();
    
    void setIsShuttingDown();

    static quint64 _totalBytes;
    static quint64 _totalWastedBytes;
    static quint64 _totalPackets;

    static quint64 _usleepTime;
    static quint64 _usleepCalls;

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();

private:
    SharedAssignmentPointer _myAssignment;
    OctreeServer* _myServer;
    SharedNodePointer _node;
    QUuid _nodeUUID;

    int handlePacketSend(OctreeQueryNode* nodeData, int& trueBytesSent, int& truePacketsSent);
    int packetDistributor(OctreeQueryNode* nodeData, bool viewFrustumChanged);

    OctreePacketData _packetData;
    
    int _nodeMissingCount;
    QMutex _processLock; // don't allow us to have our nodeData, or our thread to be deleted while we're processing
    bool _isShuttingDown;
};

#endif // __octree_server__OctreeSendThread__

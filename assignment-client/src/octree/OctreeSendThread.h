//
//  OctreeSendThread.h
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Threaded or non-threaded object for sending voxels to a client
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeSendThread_h
#define hifi_OctreeSendThread_h

#include <GenericThread.h>
#include <NetworkPacket.h>
#include <OctreeElementBag.h>
#include "OctreeQueryNode.h"
#include "OctreeServer.h"


/// Threaded processor for sending voxel packets to a single client
class OctreeSendThread : public GenericThread {
    Q_OBJECT
public:
    OctreeSendThread(OctreeServer* myServer, SharedNodePointer node);
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
    OctreeServer* _myServer;
    QUuid _nodeUUID;

    int handlePacketSend(const SharedNodePointer& node, OctreeQueryNode* nodeData, int& trueBytesSent, int& truePacketsSent);
    int packetDistributor(const SharedNodePointer& node, OctreeQueryNode* nodeData, bool viewFrustumChanged);

    OctreePacketData _packetData;
    
    int _nodeMissingCount;
    QMutex _processLock; // don't allow us to have our nodeData, or our thread to be deleted while we're processing
    bool _isShuttingDown;
};

#endif // hifi_OctreeSendThread_h

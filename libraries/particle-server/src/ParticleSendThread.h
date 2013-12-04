//
//  ParticleSendThread.h
//  particle-server
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded object for sending particles to a client
//

#ifndef __particle_server__ParticleSendThread__
#define __particle_server__ParticleSendThread__

#include <GenericThread.h>
#include <NetworkPacket.h>
#include <ParticleTree.h>
#include <ParticleNodeBag.h>
#include "ParticleReceiverData.h"
#include "ParticleServer.h"

/// Threaded processor for sending particle packets to a single client
class ParticleSendThread : public virtual GenericThread {
public:
    ParticleSendThread(const QUuid& nodeUUID, ParticleServer* myServer);

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
    ParticleServer* _myServer;

    int handlePacketSend(Node* node, ParticleReceiverData* nodeData, int& trueBytesSent, int& truePacketsSent);
    int deepestLevelParticleDistributor(Node* node, ParticleReceiverData* nodeData, bool viewFrustumChanged);
    
    ParticlePacketData _packetData;
};

#endif // __particle_server__ParticleSendThread__

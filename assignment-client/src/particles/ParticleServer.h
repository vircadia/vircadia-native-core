//
//  ParticleServer.h
//  assignment-client/src/particles
//
//  Created by Brad Hefta-Gaub on 12/2/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParticleServer_h
#define hifi_ParticleServer_h

#include "../octree/OctreeServer.h"

#include "Particle.h"
#include "ParticleServerConsts.h"
#include "ParticleTree.h"

/// Handles assignments of type ParticleServer - sending particles to various clients.
class ParticleServer : public OctreeServer, public NewlyCreatedParticleHook {
    Q_OBJECT
public:
    ParticleServer(const QByteArray& packet);
    ~ParticleServer();

    // Subclasses must implement these methods
    virtual OctreeQueryNode* createOctreeQueryNode();
    virtual Octree* createTree();
    virtual char getMyNodeType() const { return NodeType::ParticleServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeParticleQuery; }
    virtual const char* getMyServerName() const { return PARTICLE_SERVER_NAME; }
    virtual const char* getMyLoggingServerTargetName() const { return PARTICLE_SERVER_LOGGING_TARGET_NAME; }
    virtual const char* getMyDefaultPersistFilename() const { return LOCAL_PARTICLES_PERSIST_FILE; }
    virtual PacketType getMyEditNackType() const { return PacketTypeParticleEditNack; }

    // subclass may implement these method
    virtual void beforeRun();
    virtual bool hasSpecialPacketToSend(const SharedNodePointer& node);
    virtual int sendSpecialPacket(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent);

    virtual void particleCreated(const Particle& newParticle, const SharedNodePointer& senderNode);

public slots:
    void pruneDeletedParticles();

private:
};

#endif // hifi_ParticleServer_h

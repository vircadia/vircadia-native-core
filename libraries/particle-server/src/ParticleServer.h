//
//  ParticleServer.h
//  particle-server
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __particle_server__ParticleServer__
#define __particle_server__ParticleServer__

#include <OctreeServer.h>

#include "Particle.h"
#include "ParticleServerConsts.h"
#include "ParticleTree.h"

/// Handles assignments of type ParticleServer - sending particles to various clients.
class ParticleServer : public OctreeServer, public NewlyCreatedParticleHook {
    Q_OBJECT
public:
    ParticleServer(const unsigned char* dataBuffer, int numBytes);
    ~ParticleServer();

    // Subclasses must implement these methods
    virtual OctreeQueryNode* createOctreeQueryNode(Node* newNode);
    virtual Octree* createTree();
    virtual unsigned char getMyNodeType() const { return NODE_TYPE_PARTICLE_SERVER; }
    virtual PACKET_TYPE getMyQueryMessageType() const { return PACKET_TYPE_PARTICLE_QUERY; }
    virtual const char* getMyServerName() const { return PARTICLE_SERVER_NAME; }
    virtual const char* getMyLoggingServerTargetName() const { return PARTICLE_SERVER_LOGGING_TARGET_NAME; }
    virtual const char* getMyDefaultPersistFilename() const { return LOCAL_PARTICLES_PERSIST_FILE; }

    // subclass may implement these method
    virtual void beforeRun();
    virtual bool hasSpecialPacketToSend(Node* node);
    virtual int sendSpecialPacket(Node* node);

    virtual void particleCreated(const Particle& newParticle, Node* senderNode);

public slots:
    void pruneDeletedParticles();

private:
};

#endif // __particle_server__ParticleServer__

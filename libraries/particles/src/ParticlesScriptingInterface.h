//
//  ParticlesScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ParticlesScriptingInterface__
#define __hifi__ParticlesScriptingInterface__

#include <QtCore/QObject>

#include <OctreeScriptingInterface.h>
#include "ParticleEditPacketSender.h"

/// handles scripting of Particle commands from JS passed to assigned clients
class ParticlesScriptingInterface : public OctreeScriptingInterface {
    Q_OBJECT
public:
    ParticlesScriptingInterface();
    
    ParticleEditPacketSender* getParticlePacketSender() const { return (ParticleEditPacketSender*)getPacketSender(); }
    virtual NODE_TYPE getServerNodeType() const { return NODE_TYPE_PARTICLE_SERVER; }
    virtual OctreeEditPacketSender* createPacketSender() { return new ParticleEditPacketSender(); }

    void setParticleTree(ParticleTree* particleTree) { _particleTree = particleTree; }
    ParticleTree* getParticleTree(ParticleTree*) { return _particleTree; }

public slots:
    ParticleID addParticle(const ParticleProperties& properties);
    void editParticle(ParticleID particleID, const ParticleProperties& properties);
    void deleteParticle(ParticleID particleID);
    ParticleID findClosestParticle(const glm::vec3& center, float radius) const;

private:
    void queueParticleMessage(PACKET_TYPE packetType, ParticleID particleID, const ParticleProperties& properties);

    uint32_t _nextCreatorTokenID;
    ParticleTree* _particleTree;
};

#endif /* defined(__hifi__ParticlesScriptingInterface__) */

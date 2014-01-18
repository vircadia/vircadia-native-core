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

#include <JurisdictionListener.h>
#include <OctreeScriptingInterface.h>
#include "ParticleEditPacketSender.h"

/// handles scripting of Particle commands from JS passed to assigned clients
class ParticlesScriptingInterface : public OctreeScriptingInterface {
    Q_OBJECT
public:
    ParticleEditPacketSender* getParticlePacketSender() const { return (ParticleEditPacketSender*)getPacketSender(); }
    virtual NODE_TYPE getServerNodeType() const { return NODE_TYPE_PARTICLE_SERVER; }
    virtual OctreeEditPacketSender* createPacketSender() { return new ParticleEditPacketSender(); }

public slots:
    ParticleID addParticle(glm::vec3 position, float radius,
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, float lifetime, bool inHand, QString script);

    void editParticle(ParticleID particleID, glm::vec3 position, float radius,
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, float lifetime, bool inHand, QString script);

    ParticleID addParticle(const ParticleProperties& properties);

    void editParticle(ParticleID particleID, const ParticleProperties& properties);
    void deleteParticle(ParticleID particleID);

private:
    void queueParticleMessage(PACKET_TYPE packetType, ParticleDetail& particleDetails);

    uint32_t _nextCreatorTokenID;
};

#endif /* defined(__hifi__ParticlesScriptingInterface__) */

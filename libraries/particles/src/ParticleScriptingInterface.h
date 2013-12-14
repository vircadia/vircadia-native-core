//
//  ParticleScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ParticleScriptingInterface__
#define __hifi__ParticleScriptingInterface__

#include <QtCore/QObject>

#include <JurisdictionListener.h>
#include <OctreeScriptingInterface.h>
#include "ParticleEditPacketSender.h"

/// handles scripting of Particle commands from JS passed to assigned clients
class ParticleScriptingInterface : public OctreeScriptingInterface {
    Q_OBJECT
public:
    ParticleEditPacketSender* getParticlePacketSender() const { return (ParticleEditPacketSender*)getPacketSender(); }
    virtual NODE_TYPE getServerNodeType() const { return NODE_TYPE_PARTICLE_SERVER; }
    virtual OctreeEditPacketSender* createPacketSender() { return new ParticleEditPacketSender(); }

public slots:
    /// queues the creation of a Particle which will be sent by calling process on the PacketSender
    /// returns the creatorTokenID for the newly created particle
    unsigned int queueParticleAdd(glm::vec3 position, float radius, 
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, QString updateScript);

private:
    void queueParticleAdd(PACKET_TYPE addPacketType, ParticleDetail& addParticleDetails);
    
    uint32_t _nextCreatorTokenID;
};

#endif /* defined(__hifi__ParticleScriptingInterface__) */

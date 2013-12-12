//
//  ParticleEditHandle.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleEditHandle__
#define __hifi__ParticleEditHandle__

#include <glm/glm.hpp>
#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <SharedUtil.h>
#include <OctreePacketData.h>

#include "Particle.h"

class ParticleEditPacketSender;
class ParticleTree;

class ParticleEditHandle {
public:
    ParticleEditHandle(ParticleEditPacketSender* packetSender, ParticleTree* localTree, uint32_t id = NEW_PARTICLE);
    ~ParticleEditHandle();

    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    uint32_t getID() const { return _id; }

    bool isKnownID() const { return _isKnownID; }

    void createParticle(glm::vec3 position, float radius, xColor color, glm::vec3 velocity, 
                           glm::vec3 gravity, float damping, QString updateScript);

    bool updateParticle(glm::vec3 position, float radius, xColor color, glm::vec3 velocity, 
                           glm::vec3 gravity, float damping, QString updateScript);
            
    static void handleAddResponse(unsigned char* packetData , int packetLength);
private:
    uint32_t _creatorTokenID;
    uint32_t _id;
    bool _isKnownID;
    static uint32_t _nextCreatorTokenID;
    static std::map<uint32_t,ParticleEditHandle*> _allHandles;
    ParticleEditPacketSender* _packetSender;
    ParticleTree* _localTree;
};

#endif /* defined(__hifi__ParticleEditHandle__) */
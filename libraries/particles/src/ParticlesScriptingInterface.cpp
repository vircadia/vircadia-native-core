//
//  ParticlesScriptingInterface.cpp
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ParticlesScriptingInterface.h"
#include "ParticleTree.h"

ParticlesScriptingInterface::ParticlesScriptingInterface() :
    _nextCreatorTokenID(0),
    _particleTree(NULL)
{
}


void ParticlesScriptingInterface::queueParticleMessage(PacketType packetType,
        ParticleID particleID, const ParticleProperties& properties) {
    getParticlePacketSender()->queueParticleEditMessage(packetType, particleID, properties);
}

ParticleID ParticlesScriptingInterface::addParticle(const ParticleProperties& properties) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = Particle::getNextCreatorTokenID();

    ParticleID id(NEW_PARTICLE, creatorTokenID, false );

    // queue the packet
    queueParticleMessage(PacketTypeParticleAddOrEdit, id, properties);

    // If we have a local particle tree set, then also update it.
    if (_particleTree) {
        _particleTree->lockForWrite();
        _particleTree->addParticle(id, properties);
        _particleTree->unlock();
    }

    return id;
}

ParticleID ParticlesScriptingInterface::identifyParticle(ParticleID particleID) {
    uint32_t actualID = particleID.id;

    if (!particleID.isKnownID) {
        actualID = Particle::getIDfromCreatorTokenID(particleID.creatorTokenID);
        if (actualID == UNKNOWN_PARTICLE_ID) {
            return particleID; // bailing early
        }
        
        // found it!
        particleID.id = actualID;
        particleID.isKnownID = true;
    }
    return particleID;
}

ParticleProperties ParticlesScriptingInterface::getParticleProperties(ParticleID particleID) {
    ParticleProperties results;
    ParticleID identity = identifyParticle(particleID);
    if (!identity.isKnownID) {
        results.setIsUnknownID();
        return results;
    }
    if (_particleTree) {
        _particleTree->lockForRead();
        const Particle* particle = _particleTree->findParticleByID(identity.id, true);
        if (particle) {
            results.copyFromParticle(*particle);
        } else {
            results.setIsUnknownID();
        }
        _particleTree->unlock();
    }
    
    return results;
}



ParticleID ParticlesScriptingInterface::editParticle(ParticleID particleID, const ParticleProperties& properties) {
    uint32_t actualID = particleID.id;
    
    // if the particle is unknown, attempt to look it up
    if (!particleID.isKnownID) {
        actualID = Particle::getIDfromCreatorTokenID(particleID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the particle server
    if (actualID != UNKNOWN_PARTICLE_ID) {
        particleID.id = actualID;
        particleID.isKnownID = true;
        queueParticleMessage(PacketTypeParticleAddOrEdit, particleID, properties);
    }
    
    // If we have a local particle tree set, then also update it. We can do this even if we don't know
    // the actual id, because we can edit out local particles just with creatorTokenID
    if (_particleTree) {
        _particleTree->lockForWrite();
        _particleTree->updateParticle(particleID, properties);
        _particleTree->unlock();
    }
    
    return particleID;
}


// TODO: This deleteParticle() method uses the PacketType_PARTICLE_ADD_OR_EDIT message to send
// a changed particle with a shouldDie() property set to true. This works and is currently the only
// way to tell the particle server to delete a particle. But we should change this to use the PacketType_PARTICLE_ERASE
// message which takes a list of particle id's to delete.
void ParticlesScriptingInterface::deleteParticle(ParticleID particleID) {

    // setup properties to kill the particle
    ParticleProperties properties;
    properties.setShouldDie(true);

    uint32_t actualID = particleID.id;
    
    // if the particle is unknown, attempt to look it up
    if (!particleID.isKnownID) {
        actualID = Particle::getIDfromCreatorTokenID(particleID.creatorTokenID);
    }

    // if at this point, we know the id, send the update to the particle server
    if (actualID != UNKNOWN_PARTICLE_ID) {
        particleID.id = actualID;
        particleID.isKnownID = true;
        queueParticleMessage(PacketTypeParticleAddOrEdit, particleID, properties);
    }

    // If we have a local particle tree set, then also update it.
    if (_particleTree) {
        _particleTree->lockForWrite();
        _particleTree->deleteParticle(particleID);
        _particleTree->unlock();
    }
}

ParticleID ParticlesScriptingInterface::findClosestParticle(const glm::vec3& center, float radius) const {
    ParticleID result(UNKNOWN_PARTICLE_ID, UNKNOWN_TOKEN, false);
    if (_particleTree) {
        _particleTree->lockForRead();
        const Particle* closestParticle = _particleTree->findClosestParticle(center/(float)TREE_SCALE, 
                                                                                radius/(float)TREE_SCALE);
        _particleTree->unlock();
        if (closestParticle) {
            result.id = closestParticle->getID();
            result.isKnownID = true;
        }
    }
    return result;
}


QVector<ParticleID> ParticlesScriptingInterface::findParticles(const glm::vec3& center, float radius) const {
    QVector<ParticleID> result;
    if (_particleTree) {
        _particleTree->lockForRead();
        QVector<const Particle*> particles;
        _particleTree->findParticles(center/(float)TREE_SCALE, radius/(float)TREE_SCALE, particles);
        _particleTree->unlock();

        foreach (const Particle* particle, particles) {
            ParticleID thisParticleID(particle->getID(), UNKNOWN_TOKEN, true);
            result << thisParticleID;
        }
    }
    return result;
}


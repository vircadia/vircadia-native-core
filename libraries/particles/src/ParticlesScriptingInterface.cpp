//
//  ParticlesScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "ParticlesScriptingInterface.h"



void ParticlesScriptingInterface::queueParticleMessage(PACKET_TYPE packetType, ParticleDetail& particleDetails) {
    getParticlePacketSender()->queueParticleEditMessages(packetType, 1, &particleDetails);
}

ParticleID ParticlesScriptingInterface::addParticle(glm::vec3 position, float radius,
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, float lifetime, bool inHand, QString script) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = Particle::getNextCreatorTokenID();

    // setup a ParticleDetail struct with the data
    uint64_t now = usecTimestampNow();
    ParticleDetail addParticleDetail = { NEW_PARTICLE, now,
                                        position, radius, {color.red, color.green, color.blue }, velocity,
                                        gravity, damping, lifetime, inHand, script, creatorTokenID };

    // queue the packet
    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, addParticleDetail);

    ParticleID id(NEW_PARTICLE, creatorTokenID, false);
    return id;
}


void ParticlesScriptingInterface::editParticle(ParticleID particleID, glm::vec3 position, float radius,
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, float lifetime, bool inHand, QString script) {

    uint32_t actualID = particleID.id; // may not be valid... will check below..

    // if we don't know the actual id, look it up
    if (!particleID.isKnownID) {
        actualID = Particle::getIDfromCreatorTokenID(particleID.creatorTokenID);

        // if we couldn't fine it, then bail without changing anything...
        if (actualID == UNKNOWN_PARTICLE_ID) {
            return; // no changes...
        }
    }

    // setup a ParticleDetail struct with the data
    uint64_t now = usecTimestampNow();
    ParticleDetail editParticleDetail = { actualID , now,
                                        position, radius, {color.red, color.green, color.blue }, velocity,
                                        gravity, damping, lifetime, inHand, script, UNKNOWN_TOKEN };

    // queue the packet
    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, editParticleDetail);
}

ParticleID ParticlesScriptingInterface::addParticle(const ParticleProperties& properties) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = Particle::getNextCreatorTokenID();

    // setup a ParticleDetail struct with the data
    uint64_t now = usecTimestampNow();
    xColor color = properties.getColor();
    ParticleDetail addParticleDetail = { NEW_PARTICLE, now,
        properties.getPosition(), properties.getRadius(),
            {color.red, color.green, color.blue }, properties.getVelocity(),
            properties.getGravity(), properties.getDamping(), properties.getLifetime(),
            properties.getInHand(), properties.getScript(),
            creatorTokenID };

    // queue the packet
    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, addParticleDetail);

    ParticleID id(NEW_PARTICLE, creatorTokenID, false );
    return id;
}

void ParticlesScriptingInterface::editParticle(ParticleID particleID, const ParticleProperties& properties) {

    uint32_t actualID = particleID.id; // may not be valid... will check below..

    // if we don't know the actual id, look it up
    if (!particleID.isKnownID) {
        actualID = Particle::getIDfromCreatorTokenID(particleID.creatorTokenID);

        qDebug() << "ParticlesScriptingInterface::editParticle()... actualID: " << actualID;
        
        // if we couldn't fine it, then bail without changing anything...
        if (actualID == UNKNOWN_PARTICLE_ID) {
            return; // no changes...
        }
    }

    // setup a ParticleDetail struct with the data
    uint64_t now = usecTimestampNow();

    xColor color = properties.getColor();
    ParticleDetail editParticleDetail = { actualID, now,
        properties.getPosition(), properties.getRadius(),
            {color.red, color.green, color.blue }, properties.getVelocity(),
            properties.getGravity(), properties.getDamping(), properties.getLifetime(),
            properties.getInHand(), properties.getScript(),
            UNKNOWN_TOKEN };


    // queue the packet
    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, editParticleDetail);
}


void ParticlesScriptingInterface::deleteParticle(ParticleID particleID) {
}

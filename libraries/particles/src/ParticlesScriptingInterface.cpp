//
//  ParticlesScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "ParticlesScriptingInterface.h"



void ParticlesScriptingInterface::queueParticleMessage(PACKET_TYPE packetType,
        ParticleID particleID, const ParticleProperties& properties) {

qDebug() << "ParticlesScriptingInterface::queueParticleMessage()...";

    getParticlePacketSender()->queueParticleEditMessage(packetType, particleID, properties);
}

ParticleID ParticlesScriptingInterface::addParticle(const ParticleProperties& properties) {

    // The application will keep track of creatorTokenID
    uint32_t creatorTokenID = Particle::getNextCreatorTokenID();

    ParticleID id(NEW_PARTICLE, creatorTokenID, false );

    // queue the packet
    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, id, properties);

    return id;
}

void ParticlesScriptingInterface::editParticle(ParticleID particleID, const ParticleProperties& properties) {

qDebug() << "ParticlesScriptingInterface::editParticle() id.id=" << particleID.id << " id.creatorTokenID=" << particleID.creatorTokenID;

    uint32_t actualID = particleID.id;
    if (!particleID.isKnownID) {
        actualID = Particle::getIDfromCreatorTokenID(particleID.creatorTokenID);

        // hmmm... we kind of want to bail if someone attempts to edit an unknown
        if (actualID == UNKNOWN_PARTICLE_ID) {
            return; // bailing early
        }
qDebug() << "ParticlesScriptingInterface::editParticle() actualID=" << actualID;
    }

    particleID.id = actualID;
    particleID.isKnownID = true;

    queueParticleMessage(PACKET_TYPE_PARTICLE_ADD_OR_EDIT, particleID, properties);
}


void ParticlesScriptingInterface::deleteParticle(ParticleID particleID) {
}

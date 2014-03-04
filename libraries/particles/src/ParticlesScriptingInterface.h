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
    virtual NodeType_t getServerNodeType() const { return NodeType::ParticleServer; }
    virtual OctreeEditPacketSender* createPacketSender() { return new ParticleEditPacketSender(); }

    void setParticleTree(ParticleTree* particleTree) { _particleTree = particleTree; }
    ParticleTree* getParticleTree(ParticleTree*) { return _particleTree; }
    
private slots:
    /// inbound slots for external collision systems
    void forwardParticleCollisionWithVoxel(const ParticleID& particleID, const VoxelDetail& voxel) {
        emit particleCollisionWithVoxel(particleID, voxel);
    }

    void forwardParticleCollisionWithParticle(const ParticleID& idA, const ParticleID& idB) {
        emit particleCollisionWithParticle(idA, idB);
    }
    
public slots:
    /// adds a particle with the specific properties
    ParticleID addParticle(const ParticleProperties& properties);

    /// identify a recently created particle to determine its true ID
    ParticleID identifyParticle(ParticleID particleID);

    /// gets the current particle properties for a specific particle
    /// this function will not find return results in script engine contexts which don't have access to particles
    ParticleProperties getParticleProperties(ParticleID particleID);

    /// edits a particle updating only the included properties, will return the identified ParticleID in case of
    /// successful edit, if the input particleID is for an unknown particle this function will have no effect
    ParticleID editParticle(ParticleID particleID, const ParticleProperties& properties);

    /// deletes a particle
    void deleteParticle(ParticleID particleID);

    /// finds the closest particle to the center point, within the radius
    /// will return a ParticleID.isKnownID = false if no particles are in the radius
    /// this function will not find any particles in script engine contexts which don't have access to particles
    ParticleID findClosestParticle(const glm::vec3& center, float radius) const;

    /// finds particles within the search sphere specified by the center point and radius
    /// this function will not find any particles in script engine contexts which don't have access to particles
    QVector<ParticleID> findParticles(const glm::vec3& center, float radius) const;

signals:
    void particleCollisionWithVoxel(const ParticleID& particleID, const VoxelDetail& voxel);
    void particleCollisionWithParticle(const ParticleID& idA, const ParticleID& idB);

private:
    void queueParticleMessage(PacketType packetType, ParticleID particleID, const ParticleProperties& properties);

    uint32_t _nextCreatorTokenID;
    ParticleTree* _particleTree;
};

#endif /* defined(__hifi__ParticlesScriptingInterface__) */

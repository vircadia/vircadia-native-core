//
//  ParticleTree.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ParticleTree__
#define __hifi__ParticleTree__

#include <Octree.h>
#include "ParticleTreeElement.h"

class NewlyCreatedParticleHook {
public:
    virtual void particleCreated(const Particle& newParticle, Node* senderNode) = 0;
};

class ParticleTree : public Octree {
    Q_OBJECT
public:
    ParticleTree(bool shouldReaverage = false);

    /// Implements our type specific root element factory
    virtual ParticleTreeElement* createNewElement(unsigned char * octalCode = NULL) const;

    /// Type safe version of getRoot()
    ParticleTreeElement* getRoot() { return (ParticleTreeElement*)_rootNode; }


    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool getWantSVOfileVersions() const { return true; }
    virtual PACKET_TYPE expectedDataPacketType() const { return PACKET_TYPE_PARTICLE_DATA; }
    virtual bool handlesEditPacketType(PACKET_TYPE packetType) const;
    virtual int processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength, Node* senderNode);

    virtual void update();

    void storeParticle(const Particle& particle, Node* senderNode = NULL);
    const Particle* findClosestParticle(glm::vec3 position, float targetRadius);
    const Particle* findParticleByID(uint32_t id, bool alreadyLocked = false);

    void addNewlyCreatedHook(NewlyCreatedParticleHook* hook);
    void removeNewlyCreatedHook(NewlyCreatedParticleHook* hook);

    bool hasAnyDeletedParticles() const { return _recentlyDeletedParticleIDs.size() > 0; }
    bool hasParticlesDeletedSince(uint64_t sinceTime);
    bool encodeParticlesDeletedSince(uint64_t& sinceTime, unsigned char* packetData, size_t maxLength, size_t& outputLength);
    void forgetParticlesDeletedBefore(uint64_t sinceTime);

    void processEraseMessage(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr, Node* sourceNode);

    /// finds all particles that touch a box
    /// \param box the query box
    /// \param particles[out] vector of Particle pointer
    /// \remark Side effect: any initial contents in particles will be lost
    void findParticles(const AABox& box, QVector<Particle*> particles);

private:

    static bool updateOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateOperation(OctreeElement* element, void* extraData);
    static bool findNearPointOperation(OctreeElement* element, void* extraData);
    static bool pruneOperation(OctreeElement* element, void* extraData);
    static bool findByIDOperation(OctreeElement* element, void* extraData);
    static bool findAndDeleteOperation(OctreeElement* element, void* extraData);

    void notifyNewlyCreatedParticle(const Particle& newParticle, Node* senderNode);

    QReadWriteLock _newlyCreatedHooksLock;
    std::vector<NewlyCreatedParticleHook*> _newlyCreatedHooks;


    QReadWriteLock _recentlyDeletedParticlesLock;
    QMultiMap<uint64_t, uint32_t> _recentlyDeletedParticleIDs;
};

#endif /* defined(__hifi__ParticleTree__) */

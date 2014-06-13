//
//  ParticleTree.h
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParticleTree_h
#define hifi_ParticleTree_h

#include <Octree.h>
#include "ParticleTreeElement.h"

class NewlyCreatedParticleHook {
public:
    virtual void particleCreated(const Particle& newParticle, const SharedNodePointer& senderNode) = 0;
};

class ParticleTree : public Octree {
    Q_OBJECT
public:
    ParticleTree(bool shouldReaverage = false);

    /// Implements our type specific root element factory
    virtual ParticleTreeElement* createNewElement(unsigned char * octalCode = NULL);

    /// Type safe version of getRoot()
    ParticleTreeElement* getRoot() { return static_cast<ParticleTreeElement*>(_rootElement); }


    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool getWantSVOfileVersions() const { return true; }
    virtual PacketType expectedDataPacketType() const { return PacketTypeParticleData; }
    virtual bool handlesEditPacketType(PacketType packetType) const;
    virtual int processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& senderNode);

    virtual void update();

    void storeParticle(const Particle& particle, const SharedNodePointer& senderNode = SharedNodePointer());
    void updateParticle(const ParticleID& particleID, const ParticleProperties& properties);
    void addParticle(const ParticleID& particleID, const ParticleProperties& properties);
    void deleteParticle(const ParticleID& particleID);
    const Particle* findClosestParticle(glm::vec3 position, float targetRadius);
    const Particle* findParticleByID(uint32_t id, bool alreadyLocked = false);

    /// finds all particles that touch a sphere
    /// \param center the center of the sphere
    /// \param radius the radius of the sphere
    /// \param foundParticles[out] vector of const Particle*
    /// \remark Side effect: any initial contents in foundParticles will be lost
    void findParticles(const glm::vec3& center, float radius, QVector<const Particle*>& foundParticles);

    /// finds all particles that touch a box
    /// \param box the query box
    /// \param foundParticles[out] vector of non-const Particle*
    /// \remark Side effect: any initial contents in particles will be lost
    void findParticles(const AACube& box, QVector<Particle*> foundParticles);

    void addNewlyCreatedHook(NewlyCreatedParticleHook* hook);
    void removeNewlyCreatedHook(NewlyCreatedParticleHook* hook);

    bool hasAnyDeletedParticles() const { return _recentlyDeletedParticleIDs.size() > 0; }
    bool hasParticlesDeletedSince(quint64 sinceTime);
    bool encodeParticlesDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime, unsigned char* packetData, size_t maxLength, size_t& outputLength);
    void forgetParticlesDeletedBefore(quint64 sinceTime);

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);
    void handleAddParticleResponse(const QByteArray& packet);

private:

    static bool updateOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateWithIDandPropertiesOperation(OctreeElement* element, void* extraData);
    static bool findNearPointOperation(OctreeElement* element, void* extraData);
    static bool findInSphereOperation(OctreeElement* element, void* extraData);
    static bool pruneOperation(OctreeElement* element, void* extraData);
    static bool findByIDOperation(OctreeElement* element, void* extraData);
    static bool findAndDeleteOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateParticleIDOperation(OctreeElement* element, void* extraData);

    static bool findInCubeOperation(OctreeElement* element, void* extraData);

    void notifyNewlyCreatedParticle(const Particle& newParticle, const SharedNodePointer& senderNode);

    QReadWriteLock _newlyCreatedHooksLock;
    std::vector<NewlyCreatedParticleHook*> _newlyCreatedHooks;


    QReadWriteLock _recentlyDeletedParticlesLock;
    QMultiMap<quint64, uint32_t> _recentlyDeletedParticleIDs;
};

#endif // hifi_ParticleTree_h

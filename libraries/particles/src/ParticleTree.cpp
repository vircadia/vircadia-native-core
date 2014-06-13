//
//  ParticleTree.cpp
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ParticleTree.h"

ParticleTree::ParticleTree(bool shouldReaverage) : Octree(shouldReaverage) {
    _rootElement = createNewElement();
}

ParticleTreeElement* ParticleTree::createNewElement(unsigned char * octalCode) {
    ParticleTreeElement* newElement = new ParticleTreeElement(octalCode);
    newElement->setTree(this);
    return newElement;
}

bool ParticleTree::handlesEditPacketType(PacketType packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeParticleAddOrEdit:
        case PacketTypeParticleErase:
            return true;
        default:
            return false;
    }
}

class FindAndDeleteParticlesArgs {
public:
    QList<uint32_t> _idsToDelete;
};

bool ParticleTree::findAndDeleteOperation(OctreeElement* element, void* extraData) {
    //qDebug() << "findAndDeleteOperation()";

    FindAndDeleteParticlesArgs* args = static_cast< FindAndDeleteParticlesArgs*>(extraData);

    // if we've found and deleted all our target particles, then we can stop looking
    if (args->_idsToDelete.size() <= 0) {
        return false;
    }

    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);

    //qDebug() << "findAndDeleteOperation() args->_idsToDelete.size():" << args->_idsToDelete.size();

    for (QList<uint32_t>::iterator it = args->_idsToDelete.begin(); it != args->_idsToDelete.end(); it++) {
        uint32_t particleID = *it;
        //qDebug() << "findAndDeleteOperation() particleID:" << particleID;

        if (particleTreeElement->removeParticleWithID(particleID)) {
            // if the particle was in this element, then remove it from our search list.
            //qDebug() << "findAndDeleteOperation() it = args->_idsToDelete.erase(it)";
            it = args->_idsToDelete.erase(it);
        }

        if (it == args->_idsToDelete.end()) {
            //qDebug() << "findAndDeleteOperation() breaking";
            break;
        }
    }

    // if we've found and deleted all our target particles, then we can stop looking
    if (args->_idsToDelete.size() <= 0) {
        return false;
    }
    return true;
}


class FindAndUpdateParticleArgs {
public:
    const Particle& searchParticle;
    bool found;
};

bool ParticleTree::findAndUpdateOperation(OctreeElement* element, void* extraData) {
    FindAndUpdateParticleArgs* args = static_cast<FindAndUpdateParticleArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    // Note: updateParticle() will only operate on correctly found particles
    if (particleTreeElement->updateParticle(args->searchParticle)) {
        args->found = true;
        return false; // stop searching
    }
    return true;
}

void ParticleTree::storeParticle(const Particle& particle, const SharedNodePointer& senderNode) {
    // First, look for the existing particle in the tree..
    FindAndUpdateParticleArgs args = { particle, false };
    recurseTreeWithOperation(findAndUpdateOperation, &args);

    // if we didn't find it in the tree, then store it...
    if (!args.found) {
        glm::vec3 position = particle.getPosition();
        float size = std::max(MINIMUM_PARTICLE_ELEMENT_SIZE, particle.getRadius());

        ParticleTreeElement* element = (ParticleTreeElement*)getOrCreateChildElementAt(position.x, position.y, position.z, size);
        element->storeParticle(particle);
    }
    // what else do we need to do here to get reaveraging to work
    _isDirty = true;
}

class FindAndUpdateParticleWithIDandPropertiesArgs {
public:
    const ParticleID& particleID;
    const ParticleProperties& properties;
    bool found;
};

bool ParticleTree::findAndUpdateWithIDandPropertiesOperation(OctreeElement* element, void* extraData) {
    FindAndUpdateParticleWithIDandPropertiesArgs* args = static_cast<FindAndUpdateParticleWithIDandPropertiesArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    // Note: updateParticle() will only operate on correctly found particles
    if (particleTreeElement->updateParticle(args->particleID, args->properties)) {
        args->found = true;
        return false; // stop searching
    }

    // if we've found our particle stop searching
    if (args->found) {
        return false;
    }

    return true;
}

void ParticleTree::updateParticle(const ParticleID& particleID, const ParticleProperties& properties) {
    // First, look for the existing particle in the tree..
    FindAndUpdateParticleWithIDandPropertiesArgs args = { particleID, properties, false };
    recurseTreeWithOperation(findAndUpdateWithIDandPropertiesOperation, &args);
    // if we found it in the tree, then mark the tree as dirty
    if (args.found) {
        _isDirty = true;
    }
}

void ParticleTree::addParticle(const ParticleID& particleID, const ParticleProperties& properties) {
    // This only operates on locally created particles
    if (particleID.isKnownID) {
        return; // not allowed
    }
    Particle particle(particleID, properties);
    glm::vec3 position = particle.getPosition();
    float size = std::max(MINIMUM_PARTICLE_ELEMENT_SIZE, particle.getRadius());
    
    ParticleTreeElement* element = (ParticleTreeElement*)getOrCreateChildElementAt(position.x, position.y, position.z, size);
    element->storeParticle(particle);
    
    _isDirty = true;
}

void ParticleTree::deleteParticle(const ParticleID& particleID) {
    if (particleID.isKnownID) {
        FindAndDeleteParticlesArgs args;
        args._idsToDelete.push_back(particleID.id);
        recurseTreeWithOperation(findAndDeleteOperation, &args);
    }
}

// scans the tree and handles mapping locally created particles to know IDs.
// in the event that this tree is also viewing the scene, then we need to also
// search the tree to make sure we don't have a duplicate particle from the viewing
// operation.
bool ParticleTree::findAndUpdateParticleIDOperation(OctreeElement* element, void* extraData) {
    bool keepSearching = true;

    FindAndUpdateParticleIDArgs* args = static_cast<FindAndUpdateParticleIDArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);

    // Note: updateParticleID() will only operate on correctly found particles
    particleTreeElement->updateParticleID(args);

    // if we've found and replaced both the creatorTokenID and the viewedParticle, then we
    // can stop looking, otherwise we will keep looking    
    if (args->creatorTokenFound && args->viewedParticleFound) {
        keepSearching = false;
    }
    
    return keepSearching;
}

void ParticleTree::handleAddParticleResponse(const QByteArray& packet) {
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    
    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data()) + numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t particleID;
    memcpy(&particleID, dataAt, sizeof(particleID));
    dataAt += sizeof(particleID);

    // update particles in our tree
    bool assumeParticleFound = !getIsViewing(); // if we're not a viewing tree, then we don't have to find the actual particle
    FindAndUpdateParticleIDArgs args = { 
        particleID, 
        creatorTokenID, 
        false, 
        assumeParticleFound,
        getIsViewing() 
    };
    
    const bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "looking for creatorTokenID=" << creatorTokenID << " particleID=" << particleID 
                << " getIsViewing()=" << getIsViewing();
    }
    lockForWrite();
    recurseTreeWithOperation(findAndUpdateParticleIDOperation, &args);
    unlock();
}


class FindNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    bool found;
    const Particle* closestParticle;
    float closestParticleDistance;
};


bool ParticleTree::findNearPointOperation(OctreeElement* element, void* extraData) {
    FindNearPointArgs* args = static_cast<FindNearPointArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);

    glm::vec3 penetration;
    bool sphereIntersection = particleTreeElement->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this particleTreeElement contains the point, then search it...
    if (sphereIntersection) {
        const Particle* thisClosestParticle = particleTreeElement->getClosestParticle(args->position);

        // we may have gotten NULL back, meaning no particle was available
        if (thisClosestParticle) {
            glm::vec3 particlePosition = thisClosestParticle->getPosition();
            float distanceFromPointToParticle = glm::distance(particlePosition, args->position);

            // If we're within our target radius
            if (distanceFromPointToParticle <= args->targetRadius) {
                // we are closer than anything else we've found
                if (distanceFromPointToParticle < args->closestParticleDistance) {
                    args->closestParticle = thisClosestParticle;
                    args->closestParticleDistance = distanceFromPointToParticle;
                    args->found = true;
                }
            }
        }

        // we should be able to optimize this...
        return true; // keep searching in case children have closer particles
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

const Particle* ParticleTree::findClosestParticle(glm::vec3 position, float targetRadius) {
    FindNearPointArgs args = { position, targetRadius, false, NULL, FLT_MAX };
    lockForRead();
    recurseTreeWithOperation(findNearPointOperation, &args);
    unlock();
    return args.closestParticle;
}

class FindAllNearPointArgs {
public:
    glm::vec3 position;
    float targetRadius;
    QVector<const Particle*> particles;
};


bool ParticleTree::findInSphereOperation(OctreeElement* element, void* extraData) {
    FindAllNearPointArgs* args = static_cast<FindAllNearPointArgs*>(extraData);
    glm::vec3 penetration;
    bool sphereIntersection = element->getAACube().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this element contains the point, then search it...
    if (sphereIntersection) {
        ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
        particleTreeElement->getParticles(args->position, args->targetRadius, args->particles);
        return true; // keep searching in case children have closer particles
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

void ParticleTree::findParticles(const glm::vec3& center, float radius, QVector<const Particle*>& foundParticles) {
    FindAllNearPointArgs args = { center, radius };
    lockForRead();
    recurseTreeWithOperation(findInSphereOperation, &args);
    unlock();
    // swap the two lists of particle pointers instead of copy
    foundParticles.swap(args.particles);
}

class FindParticlesInCubeArgs {
public:
    FindParticlesInCubeArgs(const AACube& cube) 
        : _cube(cube), _foundParticles() {
    }

    AACube _cube;
    QVector<Particle*> _foundParticles;
};

bool ParticleTree::findInCubeOperation(OctreeElement* element, void* extraData) {
    FindParticlesInCubeArgs* args = static_cast< FindParticlesInCubeArgs*>(extraData);
    const AACube& elementBox = element->getAACube();
    if (elementBox.touches(args->_cube)) {
        ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
        particleTreeElement->getParticles(args->_cube, args->_foundParticles);
        return true;
    }
    return false;
}

void ParticleTree::findParticles(const AACube& cube, QVector<Particle*> foundParticles) {
    FindParticlesInCubeArgs args(cube);
    lockForRead();
    recurseTreeWithOperation(findInCubeOperation, &args);
    unlock();
    // swap the two lists of particle pointers instead of copy
    foundParticles.swap(args._foundParticles);
}

class FindByIDArgs {
public:
    uint32_t id;
    bool found;
    const Particle* foundParticle;
};


bool ParticleTree::findByIDOperation(OctreeElement* element, void* extraData) {
//qDebug() << "ParticleTree::findByIDOperation()....";

    FindByIDArgs* args = static_cast<FindByIDArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);

    // if already found, stop looking
    if (args->found) {
        return false;
    }

    // as the tree element if it has this particle
    const Particle* foundParticle = particleTreeElement->getParticleWithID(args->id);
    if (foundParticle) {
        args->foundParticle = foundParticle;
        args->found = true;
        return false;
    }

    // keep looking
    return true;
}


const Particle* ParticleTree::findParticleByID(uint32_t id, bool alreadyLocked) {
    FindByIDArgs args = { id, false, NULL };

    if (!alreadyLocked) {
        lockForRead();
    }
    recurseTreeWithOperation(findByIDOperation, &args);
    if (!alreadyLocked) {
        unlock();
    }
    return args.foundParticle;
}


int ParticleTree::processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& senderNode) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeParticleAddOrEdit: {
            bool isValid;
            Particle newParticle = Particle::fromEditPacket(editData, maxLength, processedBytes, this, isValid);
            if (isValid) {
                storeParticle(newParticle, senderNode);
                if (newParticle.isNewlyCreated()) {
                    notifyNewlyCreatedParticle(newParticle, senderNode);
                }
            }
        } break;

        // TODO: wire in support here for server to get PACKET_TYPE_PARTICLE_ERASE messages
        // instead of using PACKET_TYPE_PARTICLE_ADD_OR_EDIT messages to delete particles
        case PacketTypeParticleErase:
            processedBytes = 0;
            break;
        default:
            processedBytes = 0;
            break;
    }
    
    return processedBytes;
}

void ParticleTree::notifyNewlyCreatedParticle(const Particle& newParticle, const SharedNodePointer& senderNode) {
    _newlyCreatedHooksLock.lockForRead();
    for (size_t i = 0; i < _newlyCreatedHooks.size(); i++) {
        _newlyCreatedHooks[i]->particleCreated(newParticle, senderNode);
    }
    _newlyCreatedHooksLock.unlock();
}

void ParticleTree::addNewlyCreatedHook(NewlyCreatedParticleHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    _newlyCreatedHooks.push_back(hook);
    _newlyCreatedHooksLock.unlock();
}

void ParticleTree::removeNewlyCreatedHook(NewlyCreatedParticleHook* hook) {
    _newlyCreatedHooksLock.lockForWrite();
    for (size_t i = 0; i < _newlyCreatedHooks.size(); i++) {
        if (_newlyCreatedHooks[i] == hook) {
            _newlyCreatedHooks.erase(_newlyCreatedHooks.begin() + i);
            break;
        }
    }
    _newlyCreatedHooksLock.unlock();
}


bool ParticleTree::updateOperation(OctreeElement* element, void* extraData) {
    ParticleTreeUpdateArgs* args = static_cast<ParticleTreeUpdateArgs*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    particleTreeElement->update(*args);
    return true;
}

bool ParticleTree::pruneOperation(OctreeElement* element, void* extraData) {
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        ParticleTreeElement* childAt = particleTreeElement->getChildAtIndex(i);
        if (childAt && childAt->isLeaf() && !childAt->hasParticles()) {
            particleTreeElement->deleteChildAtIndex(i);
        }
    }
    return true;
}

void ParticleTree::update() {
    lockForWrite();
    _isDirty = true;

    ParticleTreeUpdateArgs args = { };
    recurseTreeWithOperation(updateOperation, &args);

    // now add back any of the particles that moved elements....
    int movingParticles = args._movingParticles.size();
    for (int i = 0; i < movingParticles; i++) {
        bool shouldDie = args._movingParticles[i].getShouldDie();

        // if the particle is still inside our total bounds, then re-add it
        AACube treeBounds = getRoot()->getAACube();

        if (!shouldDie && treeBounds.contains(args._movingParticles[i].getPosition())) {
            storeParticle(args._movingParticles[i]);
        } else {
            uint32_t particleID = args._movingParticles[i].getID();
            quint64 deletedAt = usecTimestampNow();
            _recentlyDeletedParticlesLock.lockForWrite();
            _recentlyDeletedParticleIDs.insert(deletedAt, particleID);
            _recentlyDeletedParticlesLock.unlock();
        }
    }

    // prune the tree...
    recurseTreeWithOperation(pruneOperation, NULL);
    unlock();
}


bool ParticleTree::hasParticlesDeletedSince(quint64 sinceTime) {
    // we can probably leverage the ordered nature of QMultiMap to do this quickly...
    bool hasSomethingNewer = false;

    _recentlyDeletedParticlesLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedParticleIDs.constBegin();
    while (iterator != _recentlyDeletedParticleIDs.constEnd()) {
        //qDebug() << "considering... time/key:" << iterator.key();
        if (iterator.key() > sinceTime) {
            //qDebug() << "YES newer... time/key:" << iterator.key();
            hasSomethingNewer = true;
        }
        ++iterator;
    }
    _recentlyDeletedParticlesLock.unlock();
    return hasSomethingNewer;
}

// sinceTime is an in/out parameter - it will be side effected with the last time sent out
bool ParticleTree::encodeParticlesDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime, unsigned char* outputBuffer,
                                                size_t maxLength, size_t& outputLength) {

    bool hasMoreToSend = true;

    unsigned char* copyAt = outputBuffer;
    size_t numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(outputBuffer), PacketTypeParticleErase);
    copyAt += numBytesPacketHeader;
    outputLength = numBytesPacketHeader;

    // pack in flags
    OCTREE_PACKET_FLAGS flags = 0;
    memcpy(copyAt, &flags, sizeof(OCTREE_PACKET_FLAGS));
    copyAt += sizeof(OCTREE_PACKET_FLAGS);
    outputLength += sizeof(OCTREE_PACKET_FLAGS);

    // pack in sequence number
    memcpy(copyAt, &sequenceNumber, sizeof(OCTREE_PACKET_SEQUENCE));
    copyAt += sizeof(OCTREE_PACKET_SEQUENCE);
    outputLength += sizeof(OCTREE_PACKET_SEQUENCE);

    // pack in timestamp
    OCTREE_PACKET_SENT_TIME now = usecTimestampNow();
    memcpy(copyAt, &now, sizeof(OCTREE_PACKET_SENT_TIME));
    copyAt += sizeof(OCTREE_PACKET_SENT_TIME);
    outputLength += sizeof(OCTREE_PACKET_SENT_TIME);


    uint16_t numberOfIds = 0; // placeholder for now
    unsigned char* numberOfIDsAt = copyAt;
    memcpy(copyAt, &numberOfIds, sizeof(numberOfIds));
    copyAt += sizeof(numberOfIds);
    outputLength += sizeof(numberOfIds);

    // we keep a multi map of particle IDs to timestamps, we only want to include the particle IDs that have been
    // deleted since we last sent to this node
    _recentlyDeletedParticlesLock.lockForRead();
    QMultiMap<quint64, uint32_t>::const_iterator iterator = _recentlyDeletedParticleIDs.constBegin();
    while (iterator != _recentlyDeletedParticleIDs.constEnd()) {
        QList<uint32_t> values = _recentlyDeletedParticleIDs.values(iterator.key());
        for (int valueItem = 0; valueItem < values.size(); ++valueItem) {

            // if the timestamp is more recent then out last sent time, include it
            if (iterator.key() > sinceTime) {
                uint32_t particleID = values.at(valueItem);
                memcpy(copyAt, &particleID, sizeof(particleID));
                copyAt += sizeof(particleID);
                outputLength += sizeof(particleID);
                numberOfIds++;

                // check to make sure we have room for one more id...
                if (outputLength + sizeof(uint32_t) > maxLength) {
                    break;
                }
            }
        }

        // check to make sure we have room for one more id...
        if (outputLength + sizeof(uint32_t) > maxLength) {

            // let our caller know how far we got
            sinceTime = iterator.key();
            break;
        }
        ++iterator;
    }

    // if we got to the end, then we're done sending
    if (iterator == _recentlyDeletedParticleIDs.constEnd()) {
        hasMoreToSend = false;
    }
    _recentlyDeletedParticlesLock.unlock();

    // replace the correct count for ids included
    memcpy(numberOfIDsAt, &numberOfIds, sizeof(numberOfIds));

    return hasMoreToSend;
}

// called by the server when it knows all nodes have been sent deleted packets

void ParticleTree::forgetParticlesDeletedBefore(quint64 sinceTime) {
    //qDebug() << "forgetParticlesDeletedBefore()";
    QSet<quint64> keysToRemove;

    _recentlyDeletedParticlesLock.lockForWrite();
    QMultiMap<quint64, uint32_t>::iterator iterator = _recentlyDeletedParticleIDs.begin();

    // First find all the keys in the map that are older and need to be deleted
    while (iterator != _recentlyDeletedParticleIDs.end()) {
        if (iterator.key() <= sinceTime) {
            keysToRemove << iterator.key();
        }
        ++iterator;
    }

    // Now run through the keysToRemove and remove them
    foreach (quint64 value, keysToRemove) {
        //qDebug() << "removing the key, _recentlyDeletedParticleIDs.remove(value); time/key:" << value;
        _recentlyDeletedParticleIDs.remove(value);
    }
    
    _recentlyDeletedParticlesLock.unlock();
}


void ParticleTree::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {

    const unsigned char* packetData = (const unsigned char*)dataByteArray.constData();
    const unsigned char* dataAt = packetData;
    size_t packetLength = dataByteArray.size();

    size_t numBytesPacketHeader = numBytesForPacketHeader(dataByteArray);
    size_t processedBytes = numBytesPacketHeader;
    dataAt += numBytesPacketHeader;

    dataAt += sizeof(OCTREE_PACKET_FLAGS);
    dataAt += sizeof(OCTREE_PACKET_SEQUENCE);
    dataAt += sizeof(OCTREE_PACKET_SENT_TIME);

    uint16_t numberOfIds = 0; // placeholder for now
    memcpy(&numberOfIds, dataAt, sizeof(numberOfIds));
    dataAt += sizeof(numberOfIds);
    processedBytes += sizeof(numberOfIds);

    if (numberOfIds > 0) {
        FindAndDeleteParticlesArgs args;

        for (size_t i = 0; i < numberOfIds; i++) {
            if (processedBytes + sizeof(uint32_t) > packetLength) {
                break; // bail to prevent buffer overflow
            }

            uint32_t particleID = 0; // placeholder for now
            memcpy(&particleID, dataAt, sizeof(particleID));
            dataAt += sizeof(particleID);
            processedBytes += sizeof(particleID);

            args._idsToDelete.push_back(particleID);
        }

        // calling recurse to actually delete the particles
        recurseTreeWithOperation(findAndDeleteOperation, &args);
    }
}

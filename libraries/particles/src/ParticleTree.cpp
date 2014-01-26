//
//  ParticleTree.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "ParticleTree.h"

ParticleTree::ParticleTree(bool shouldReaverage) : Octree(shouldReaverage) {
    ParticleTreeElement* rootNode = createNewElement();
    _rootNode = rootNode;
}

ParticleTreeElement* ParticleTree::createNewElement(unsigned char * octalCode) {
    ParticleTreeElement* newElement = new ParticleTreeElement(octalCode);
    newElement->setTree(this);
    return newElement;
}

bool ParticleTree::handlesEditPacketType(PACKET_TYPE packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_PARTICLE_ADD_OR_EDIT:
        case PACKET_TYPE_PARTICLE_ERASE:
            return true;
    }
    return false;
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
    if (particleTreeElement->containsParticle(args->searchParticle)) {
        particleTreeElement->updateParticle(args->searchParticle);
        args->found = true;
        return false; // stop searching
    }
    return true;
}

void ParticleTree::storeParticle(const Particle& particle, Node* senderNode) {
    // First, look for the existing particle in the tree..
    FindAndUpdateParticleArgs args = { particle, false };
    recurseTreeWithOperation(findAndUpdateOperation, &args);

    // if we didn't find it in the tree, then store it...
    if (!args.found) {
        glm::vec3 position = particle.getPosition();
        float size = std::max(MINIMUM_PARTICLE_ELEMENT_SIZE, particle.getRadius());
        ParticleTreeElement* element = (ParticleTreeElement*)getOrCreateChildElementAt(position.x, position.y, position.z, size);

        element->storeParticle(particle, senderNode);
    }
    // what else do we need to do here to get reaveraging to work
    _isDirty = true;
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
    bool sphereIntersection = particleTreeElement->getAABox().findSpherePenetration(args->position,
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
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);

    glm::vec3 penetration;
    bool sphereIntersection = particleTreeElement->getAABox().findSpherePenetration(args->position,
                                                                    args->targetRadius, penetration);

    // If this particleTreeElement contains the point, then search it...
    if (sphereIntersection) {
        QVector<const Particle*> moreParticles = particleTreeElement->getParticles(args->position, args->targetRadius);
        args->particles << moreParticles;
        return true; // keep searching in case children have closer particles
    }

    // if this element doesn't contain the point, then none of it's children can contain the point, so stop searching
    return false;
}

QVector<const Particle*> ParticleTree::findParticles(const glm::vec3& center, float radius) {
    QVector<Particle*> result;
    FindAllNearPointArgs args = { center, radius };
    lockForRead();
    recurseTreeWithOperation(findInSphereOperation, &args);
    unlock();
    return args.particles;
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
        //qDebug() << "ParticleTree::findParticleByID().... about to call lockForRead()....";
        lockForRead();
        //qDebug() << "ParticleTree::findParticleByID().... after call lockForRead()....";
    }
    recurseTreeWithOperation(findByIDOperation, &args);
    if (!alreadyLocked) {
        unlock();
    }
    return args.foundParticle;
}


int ParticleTree::processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength, Node* senderNode) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_PARTICLE_ADD_OR_EDIT: {
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
        case PACKET_TYPE_PARTICLE_ERASE: {
            processedBytes = 0;
        } break;
    }
    return processedBytes;
}

void ParticleTree::notifyNewlyCreatedParticle(const Particle& newParticle, Node* senderNode) {
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
    _isDirty = true;

    ParticleTreeUpdateArgs args = { };
    recurseTreeWithOperation(updateOperation, &args);

    // now add back any of the particles that moved elements....
    int movingParticles = args._movingParticles.size();
    for (int i = 0; i < movingParticles; i++) {
        bool shouldDie = args._movingParticles[i].getShouldDie();

        // if the particle is still inside our total bounds, then re-add it
        AABox treeBounds = getRoot()->getAABox();

        if (!shouldDie && treeBounds.contains(args._movingParticles[i].getPosition())) {
            storeParticle(args._movingParticles[i]);
        } else {
            uint32_t particleID = args._movingParticles[i].getID();
            uint64_t deletedAt = usecTimestampNow();
            _recentlyDeletedParticlesLock.lockForWrite();
            _recentlyDeletedParticleIDs.insert(deletedAt, particleID);
            _recentlyDeletedParticlesLock.unlock();
        }
    }

    // prune the tree...
    recurseTreeWithOperation(pruneOperation, NULL);
}


bool ParticleTree::hasParticlesDeletedSince(uint64_t sinceTime) {
    // we can probably leverage the ordered nature of QMultiMap to do this quickly...
    bool hasSomethingNewer = false;

    _recentlyDeletedParticlesLock.lockForRead();
    QMultiMap<uint64_t, uint32_t>::const_iterator iterator = _recentlyDeletedParticleIDs.constBegin();
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
bool ParticleTree::encodeParticlesDeletedSince(uint64_t& sinceTime, unsigned char* outputBuffer, size_t maxLength,
                                                    size_t& outputLength) {

    bool hasMoreToSend = true;

    unsigned char* copyAt = outputBuffer;
    size_t numBytesPacketHeader = populateTypeAndVersion(outputBuffer, PACKET_TYPE_PARTICLE_ERASE);
    copyAt += numBytesPacketHeader;
    outputLength = numBytesPacketHeader;

    uint16_t numberOfIds = 0; // placeholder for now
    unsigned char* numberOfIDsAt = copyAt;
    memcpy(copyAt, &numberOfIds, sizeof(numberOfIds));
    copyAt += sizeof(numberOfIds);
    outputLength += sizeof(numberOfIds);

    // we keep a multi map of particle IDs to timestamps, we only want to include the particle IDs that have been
    // deleted since we last sent to this node
    _recentlyDeletedParticlesLock.lockForRead();
    QMultiMap<uint64_t, uint32_t>::const_iterator iterator = _recentlyDeletedParticleIDs.constBegin();
    while (iterator != _recentlyDeletedParticleIDs.constEnd()) {
        QList<uint32_t> values = _recentlyDeletedParticleIDs.values(iterator.key());
        for (int valueItem = 0; valueItem < values.size(); ++valueItem) {
            //qDebug() << "considering... " << iterator.key() << ": " << values.at(valueItem);

            // if the timestamp is more recent then out last sent time, include it
            if (iterator.key() > sinceTime) {
                //qDebug() << "including... " << iterator.key() << ": " << values.at(valueItem);
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
void ParticleTree::forgetParticlesDeletedBefore(uint64_t sinceTime) {
    //qDebug() << "forgetParticlesDeletedBefore()";
    QSet<uint64_t> keysToRemove;

    _recentlyDeletedParticlesLock.lockForWrite();
    QMultiMap<uint64_t, uint32_t>::iterator iterator = _recentlyDeletedParticleIDs.begin();
    // First find all the keys in the map that are older and need to be deleted    
    while (iterator != _recentlyDeletedParticleIDs.end()) {
        //qDebug() << "considering... time/key:" << iterator.key();
        if (iterator.key() <= sinceTime) {
            //qDebug() << "YES older... time/key:" << iterator.key();
            keysToRemove << iterator.key();
        }
        ++iterator;
    }

    // Now run through the keysToRemove and remove them    
    foreach (uint64_t value, keysToRemove) {
        //qDebug() << "removing the key, _recentlyDeletedParticleIDs.remove(value); time/key:" << value;
        _recentlyDeletedParticleIDs.remove(value);
    }
    
    _recentlyDeletedParticlesLock.unlock();
    //qDebug() << "DONE forgetParticlesDeletedBefore()";
}


void ParticleTree::processEraseMessage(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr,
        Node* sourceNode) {
    //qDebug() << "ParticleTree::processEraseMessage()...";

    const unsigned char* packetData = (const unsigned char*)dataByteArray.constData();
    const unsigned char* dataAt = packetData;
    size_t packetLength = dataByteArray.size();

    size_t numBytesPacketHeader = numBytesForPacketHeader(packetData);
    size_t processedBytes = numBytesPacketHeader;
    dataAt += numBytesPacketHeader;

    uint16_t numberOfIds = 0; // placeholder for now
    memcpy(&numberOfIds, dataAt, sizeof(numberOfIds));
    dataAt += sizeof(numberOfIds);
    processedBytes += sizeof(numberOfIds);

    //qDebug() << "got erase message for numberOfIds:" << numberOfIds;

    if (numberOfIds > 0) {
        FindAndDeleteParticlesArgs args;

        for (size_t i = 0; i < numberOfIds; i++) {
            if (processedBytes + sizeof(uint32_t) > packetLength) {
                //qDebug() << "bailing?? processedBytes:" << processedBytes << " packetLength:" << packetLength;
                break; // bail to prevent buffer overflow
            }

            uint32_t particleID = 0; // placeholder for now
            memcpy(&particleID, dataAt, sizeof(particleID));
            dataAt += sizeof(particleID);
            processedBytes += sizeof(particleID);

            //qDebug() << "got erase message for particleID:" << particleID;
            args._idsToDelete.push_back(particleID);
        }

        // calling recurse to actually delete the particles
        //qDebug() << "calling recurse to actually delete the particles";
        recurseTreeWithOperation(findAndDeleteOperation, &args);
    }
}

class FindParticlesArgs {
public:
    FindParticlesArgs(const AABox& box) 
        : _box(box), _foundParticles() {
    }

    AABox _box;
    QVector<Particle*> _foundParticles;
};

bool findOperation(OctreeElement* element, void* extraData) {
    FindParticlesArgs* args = static_cast< FindParticlesArgs*>(extraData);
    const AABox& elementBox = element->getAABox();
    if (elementBox.touches(args->_box)) {
        ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);
        particleTreeElement->findParticles(args->_box, args->_foundParticles);
        return true;
    }
    return false;
}

void ParticleTree::findParticles(const AABox& box, QVector<Particle*> foundParticles) {
    FindParticlesArgs args(box);
    recurseTreeWithOperation(findOperation, &args);
    // swap the two lists of particle pointers instead of copy
    foundParticles.swap(args._foundParticles);
}

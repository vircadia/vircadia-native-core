//
//  ParticleTreeElement.cpp
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <GeometryUtil.h>

#include "ParticleTree.h"
#include "ParticleTreeElement.h"

ParticleTreeElement::ParticleTreeElement(unsigned char* octalCode) : OctreeElement(), _particles(NULL) {
    init(octalCode);
};

ParticleTreeElement::~ParticleTreeElement() {
    _voxelMemoryUsage -= sizeof(ParticleTreeElement);
    QList<Particle>* tmpParticles = _particles;
    _particles = NULL;
    delete tmpParticles;
}

// This will be called primarily on addChildAt(), which means we're adding a child of our
// own type to our own tree. This means we should initialize that child with any tree and type
// specific settings that our children must have. One example is out VoxelSystem, which
// we know must match ours.
OctreeElement* ParticleTreeElement::createNewElement(unsigned char* octalCode) {
    ParticleTreeElement* newChild = new ParticleTreeElement(octalCode);
    newChild->setTree(_myTree);
    return newChild;
}

void ParticleTreeElement::init(unsigned char* octalCode) {
    OctreeElement::init(octalCode);
    _particles = new QList<Particle>;
    _voxelMemoryUsage += sizeof(ParticleTreeElement);
}

ParticleTreeElement* ParticleTreeElement::addChildAtIndex(int index) {
    ParticleTreeElement* newElement = (ParticleTreeElement*)OctreeElement::addChildAtIndex(index);
    newElement->setTree(_myTree);
    return newElement;
}


OctreeElement::AppendState ParticleTreeElement::appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const {
    bool success = true; // assume the best...

    // write our particles out...
    uint16_t numberOfParticles = _particles->size();
    success = packetData->appendValue(numberOfParticles);

    if (success) {
        for (uint16_t i = 0; i < numberOfParticles; i++) {
            const Particle& particle = (*_particles)[i];
            success = particle.appendParticleData(packetData);
            if (!success) {
                break;
            }
        }
    }
    return success ? OctreeElement::COMPLETED : OctreeElement::NONE;
}

void ParticleTreeElement::update(ParticleTreeUpdateArgs& args) {
    markWithChangedTime();
    // TODO: early exit when _particles is empty

    // update our contained particles
    QList<Particle>::iterator particleItr = _particles->begin();
    while(particleItr != _particles->end()) {
        Particle& particle = (*particleItr);
        particle.update(_lastChanged);

        // If the particle wants to die, or if it's left our bounding box, then move it
        // into the arguments moving particles. These will be added back or deleted completely
        if (particle.getShouldDie() || !_cube.contains(particle.getPosition())) {
            args._movingParticles.push_back(particle);

            // erase this particle
            particleItr = _particles->erase(particleItr);
        } else {
            ++particleItr;
        }
    }
    // TODO: if _particles is empty after while loop consider freeing memory in _particles if
    // internal array is too big (QList internal array does not decrease size except in dtor and
    // assignment operator).  Otherwise _particles could become a "resource leak" for large
    // roaming piles of particles.
}

bool ParticleTreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                                    glm::vec3& penetration, void** penetratedObject) const {
    QList<Particle>::iterator particleItr = _particles->begin();
    QList<Particle>::const_iterator particleEnd = _particles->end();
    while(particleItr != particleEnd) {
        Particle& particle = (*particleItr);
        glm::vec3 particleCenter = particle.getPosition();
        float particleRadius = particle.getRadius();

        // don't penetrate yourself
        if (particleCenter == center && particleRadius == radius) {
            return false;
        }

        // We've considered making "inHand" particles not collide, if we want to do that,
        // we should change this setting... but now, we do allow inHand particles to collide
        const bool IN_HAND_PARTICLES_DONT_COLLIDE = false;
        if (IN_HAND_PARTICLES_DONT_COLLIDE) {
            // don't penetrate if the particle is "inHand" -- they don't collide
            if (particle.getInHand()) {
                ++particleItr;
                continue;
            }
        }

        if (findSphereSpherePenetration(center, radius, particleCenter, particleRadius, penetration)) {
            // return true on first valid particle penetration
            *penetratedObject = (void*)(&particle);
            return true;
        }
        ++particleItr;
    }
    return false;
}

bool ParticleTreeElement::updateParticle(const Particle& particle) {
    // NOTE: this method must first lookup the particle by ID, hence it is O(N)
    // and "particle is not found" is worst-case (full N) but maybe we don't care?
    // (guaranteed that num particles per elemen is small?)
    const bool wantDebug = false;
    uint16_t numberOfParticles = _particles->size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        Particle& thisParticle = (*_particles)[i];
        if (thisParticle.getID() == particle.getID()) {
            int difference = thisParticle.getLastUpdated() - particle.getLastUpdated();
            bool changedOnServer = thisParticle.getLastEdited() < particle.getLastEdited();
            bool localOlder = thisParticle.getLastUpdated() < particle.getLastUpdated();
            if (changedOnServer || localOlder) {
                if (wantDebug) {
                    qDebug("local particle [id:%d] %s and %s than server particle by %d, particle.isNewlyCreated()=%s",
                            particle.getID(), (changedOnServer ? "CHANGED" : "same"),
                            (localOlder ? "OLDER" : "NEWER"),
                            difference, debug::valueOf(particle.isNewlyCreated()) );
                }
                thisParticle.copyChangedProperties(particle);
            } else {
                if (wantDebug) {
                    qDebug(">>> IGNORING SERVER!!! Would've caused jutter! <<<  "
                            "local particle [id:%d] %s and %s than server particle by %d, particle.isNewlyCreated()=%s",
                            particle.getID(), (changedOnServer ? "CHANGED" : "same"),
                            (localOlder ? "OLDER" : "NEWER"),
                            difference, debug::valueOf(particle.isNewlyCreated()) );
                }
            }
            return true;
        }
    }
    return false;
}

bool ParticleTreeElement::updateParticle(const ParticleID& particleID, const ParticleProperties& properties) {
    uint16_t numberOfParticles = _particles->size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        // note: unlike storeParticle() which is called from inbound packets, this is only called by local editors
        // and therefore we can be confident that this change is higher priority and should be honored
        Particle& thisParticle = (*_particles)[i];
        
        bool found = false;
        if (particleID.isKnownID) {
            found = thisParticle.getID() == particleID.id;
        } else {
            found = thisParticle.getCreatorTokenID() == particleID.creatorTokenID;
        }
        if (found) {
            thisParticle.setProperties(properties);

            const bool wantDebug = false;
            if (wantDebug) {
                uint64_t now = usecTimestampNow();
                int elapsed = now - thisParticle.getLastEdited();

                qDebug() << "ParticleTreeElement::updateParticle() AFTER update... edited AGO=" << elapsed <<
                        "now=" << now << " thisParticle.getLastEdited()=" << thisParticle.getLastEdited();
            }                
            return true;
        }
    }
    return false;
}

void ParticleTreeElement::updateParticleID(FindAndUpdateParticleIDArgs* args) {
    uint16_t numberOfParticles = _particles->size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        Particle& thisParticle = (*_particles)[i];
        
        if (!args->creatorTokenFound) {
            // first, we're looking for matching creatorTokenIDs, if we find that, then we fix it to know the actual ID
            if (thisParticle.getCreatorTokenID() == args->creatorTokenID) {
                thisParticle.setID(args->particleID);
                args->creatorTokenFound = true;
            }
        }
        
        // if we're in an isViewing tree, we also need to look for an kill any viewed particles
        if (!args->viewedParticleFound && args->isViewing) {
            if (thisParticle.getCreatorTokenID() == UNKNOWN_TOKEN && thisParticle.getID() == args->particleID) {
                _particles->removeAt(i); // remove the particle at this index
                numberOfParticles--; // this means we have 1 fewer particle in this list
                i--; // and we actually want to back up i as well.
                args->viewedParticleFound = true;
            }
        }
    }
}



const Particle* ParticleTreeElement::getClosestParticle(glm::vec3 position) const {
    const Particle* closestParticle = NULL;
    float closestParticleDistance = FLT_MAX;
    uint16_t numberOfParticles = _particles->size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        float distanceToParticle = glm::distance(position, (*_particles)[i].getPosition());
        if (distanceToParticle < closestParticleDistance) {
            closestParticle = &(*_particles)[i];
        }
    }
    return closestParticle;
}

void ParticleTreeElement::getParticles(const glm::vec3& searchPosition, float searchRadius, QVector<const Particle*>& foundParticles) const {
    uint16_t numberOfParticles = _particles->size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        const Particle* particle = &(*_particles)[i];
        float distance = glm::length(particle->getPosition() - searchPosition);
        if (distance < searchRadius + particle->getRadius()) {
            foundParticles.push_back(particle);
        }
    }
}

void ParticleTreeElement::getParticles(const AACube& box, QVector<Particle*>& foundParticles) {
    QList<Particle>::iterator particleItr = _particles->begin();
    QList<Particle>::iterator particleEnd = _particles->end();
    AACube particleCube;
    while(particleItr != particleEnd) {
        Particle* particle = &(*particleItr);
        float radius = particle->getRadius();
        // NOTE: we actually do box-box collision queries here, which is sloppy but good enough for now
        // TODO: decide whether to replace particleBox-box query with sphere-box (requires a square root
        // but will be slightly more accurate).
        particleCube.setBox(particle->getPosition() - glm::vec3(radius), 2.f * radius);
        if (particleCube.touches(_cube)) {
            foundParticles.push_back(particle);
        }
        ++particleItr;
    }
}

const Particle* ParticleTreeElement::getParticleWithID(uint32_t id) const {
    // NOTE: this lookup is O(N) but maybe we don't care? (guaranteed that num particles per elemen is small?)
    const Particle* foundParticle = NULL;
    uint16_t numberOfParticles = _particles->size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        if ((*_particles)[i].getID() == id) {
            foundParticle = &(*_particles)[i];
            break;
        }
    }
    return foundParticle;
}

bool ParticleTreeElement::removeParticleWithID(uint32_t id) {
    bool foundParticle = false;
    if (_particles) {
        uint16_t numberOfParticles = _particles->size();
        for (uint16_t i = 0; i < numberOfParticles; i++) {
            if ((*_particles)[i].getID() == id) {
                foundParticle = true;
                _particles->removeAt(i);
                break;
            }
        }
    }
    return foundParticle;
}

int ParticleTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
            ReadBitstreamToTreeParams& args) {

    const unsigned char* dataAt = data;
    int bytesRead = 0;
    uint16_t numberOfParticles = 0;
    int expectedBytesPerParticle = Particle::expectedBytes();

    if (bytesLeftToRead >= (int)sizeof(numberOfParticles)) {
        // read our particles in....
        numberOfParticles = *(uint16_t*)dataAt;
        dataAt += sizeof(numberOfParticles);
        bytesLeftToRead -= (int)sizeof(numberOfParticles);
        bytesRead += sizeof(numberOfParticles);

        if (bytesLeftToRead >= (int)(numberOfParticles * expectedBytesPerParticle)) {
            for (uint16_t i = 0; i < numberOfParticles; i++) {
                Particle tempParticle;
                int bytesForThisParticle = tempParticle.readParticleDataFromBuffer(dataAt, bytesLeftToRead, args);
                _myTree->storeParticle(tempParticle);
                dataAt += bytesForThisParticle;
                bytesLeftToRead -= bytesForThisParticle;
                bytesRead += bytesForThisParticle;
            }
        }
    }

    return bytesRead;
}

// will average a "common reduced LOD view" from the the child elements...
void ParticleTreeElement::calculateAverageFromChildren() {
    // nothing to do here yet...
}

// will detect if children are leaves AND collapsable into the parent node
// and in that case will collapse children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a
// single node
bool ParticleTreeElement::collapseChildren() {
    // nothing to do here yet...
    return false;
}


void ParticleTreeElement::storeParticle(const Particle& particle) {
    _particles->push_back(particle);
    markWithChangedTime();
}


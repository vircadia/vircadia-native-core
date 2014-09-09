//
//  EntityTreeElement.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/transform.hpp>

#include <FBXReader.h>
#include <GeometryUtil.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"

EntityTreeElement::EntityTreeElement(unsigned char* octalCode) : OctreeElement(), _entityItems(NULL) {
    init(octalCode);
};

EntityTreeElement::~EntityTreeElement() {
    _voxelMemoryUsage -= sizeof(EntityTreeElement);
    delete _entityItems;
    _entityItems = NULL;
}

// This will be called primarily on addChildAt(), which means we're adding a child of our
// own type to our own tree. This means we should initialize that child with any tree and type
// specific settings that our children must have. One example is out VoxelSystem, which
// we know must match ours.
OctreeElement* EntityTreeElement::createNewElement(unsigned char* octalCode) {
    EntityTreeElement* newChild = new EntityTreeElement(octalCode);
    newChild->setTree(_myTree);
    return newChild;
}

void EntityTreeElement::init(unsigned char* octalCode) {
    OctreeElement::init(octalCode);
    _entityItems = new QList<EntityItem*>;
    _voxelMemoryUsage += sizeof(EntityTreeElement);
}

EntityTreeElement* EntityTreeElement::addChildAtIndex(int index) {
    EntityTreeElement* newElement = (EntityTreeElement*)OctreeElement::addChildAtIndex(index);
    newElement->setTree(_myTree);
    return newElement;
}

void EntityTreeElement::debugExtraEncodeData(EncodeBitstreamParams& params) const { 
    qDebug() << "EntityTreeElement::debugExtraEncodeData()... ";
    qDebug() << "    element:" << getAACube();

    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes

    if (extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData 
                    = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));
        qDebug() << "    encode data:" << entityTreeElementExtraEncodeData;
    } else {
        qDebug() << "    encode data: MISSING!!";
    }
}

void EntityTreeElement::initializeExtraEncodeData(EncodeBitstreamParams& params) const { 
    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes
    // Check to see if this element yet has encode data... if it doesn't create it
    if (!extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData = new EntityTreeElementExtraEncodeData();
        entityTreeElementExtraEncodeData->elementCompleted = (_entityItems->size() == 0);
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            EntityTreeElement* child = getChildAtIndex(i);
            if (!child) {
                entityTreeElementExtraEncodeData->childCompleted[i] = true; // if no child exists, it is completed
            } else {
                if (child->hasEntities()) {
                    entityTreeElementExtraEncodeData->childCompleted[i] = false; // HAS ENTITIES NEEDS ENCODING
                } else {
                    entityTreeElementExtraEncodeData->childCompleted[i] = true; // child doesn't have enities, it is completed
                }
            }
        }
        for (uint16_t i = 0; i < _entityItems->size(); i++) {
            EntityItem* entity = (*_entityItems)[i];
            entityTreeElementExtraEncodeData->entities.insert(entity->getEntityItemID(), entity->getEntityProperties(params));
        }
        
        // TODO: some of these inserts might be redundant!!!
        extraEncodeData->insert(this, entityTreeElementExtraEncodeData);
    }
}

bool EntityTreeElement::shouldIncludeChildData(int childIndex, EncodeBitstreamParams& params) const { 
    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes

    if (extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData 
                        = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));
                        
        bool childCompleted = entityTreeElementExtraEncodeData->childCompleted[childIndex];
        
        // If we haven't completely sent the child yet, then we should include it
        return !childCompleted;
    }
    
    // I'm not sure this should ever happen, since we should have the extra encode data if we're considering
    // the child data for this element
    assert(false);
    return false;
}

bool EntityTreeElement::shouldRecurseChildTree(int childIndex, EncodeBitstreamParams& params) const { 
    EntityTreeElement* childElement = getChildAtIndex(childIndex);
    if (childElement->alreadyFullyEncoded(params)) {
        return false;
    }
    
    return true; // if we don't know otherwise than recurse!
}

bool EntityTreeElement::alreadyFullyEncoded(EncodeBitstreamParams& params) const { 
    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes

    if (extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData 
                        = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));

        // If we know that ALL subtrees below us have already been recursed, then we don't 
        // need to recurse this child.
        return entityTreeElementExtraEncodeData->subtreeCompleted;
    }
    return false;
}

void EntityTreeElement::updateEncodedData(int childIndex, AppendState childAppendState, EncodeBitstreamParams& params) const {
    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes
    if (extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData 
                        = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));

        if (childAppendState == OctreeElement::COMPLETED) {
            entityTreeElementExtraEncodeData->childCompleted[childIndex] = true;
        }
    } else {
        assert(false); // this shouldn't happen!
    }
}




void EntityTreeElement::elementEncodeComplete(EncodeBitstreamParams& params, OctreeElementBag* bag) const {
    const bool wantDebug = false;
    
    if (wantDebug) {
        qDebug() << "EntityTreeElement::elementEncodeComplete() element:" << getAACube();
    }

    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes
    assert(extraEncodeData->contains(this));

    EntityTreeElementExtraEncodeData* thisExtraEncodeData
                = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));

    // Note: this will be called when OUR element has finished running through encodeTreeBitstreamRecursion()
    // which means, it's possible that our parent element hasn't finished encoding OUR data... so
    // in this case, our children may be complete, and we should clean up their encode data...
    // but not necessarily cleanup our own encode data...
    //
    // If we're really complete here's what must be true...
    //    1) out own data must be complete
    //    2) the data for all our immediate children must be complete.
    // However, the following might also be the case...
    //    1) it's ok for our child trees to not yet be fully encoded/complete... 
    //       SO LONG AS... the our child's node is in the bag ready for encoding

    bool someChildTreeNotComplete = false;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        EntityTreeElement* childElement = getChildAtIndex(i);
        if (childElement) {

            // why would this ever fail???
            // If we've encoding this element before... but we're coming back a second time in an attempt to
            // encoud our parent... this might happen.
            if (extraEncodeData->contains(childElement)) {
                EntityTreeElementExtraEncodeData* childExtraEncodeData 
                                = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(childElement));
                                
                if (wantDebug) {
                    qDebug() << "checking child: " << childElement->getAACube();
                    qDebug() << "    childElement->isLeaf():" << childElement->isLeaf();
                    qDebug() << "    childExtraEncodeData->elementCompleted:" << childExtraEncodeData->elementCompleted;
                    qDebug() << "    childExtraEncodeData->subtreeCompleted:" << childExtraEncodeData->subtreeCompleted;
                }
                
                if (childElement->isLeaf() && childExtraEncodeData->elementCompleted) {
                    if (wantDebug) {
                        qDebug() << "    CHILD IS LEAF -- AND CHILD ELEMENT DATA COMPLETED!!!";
                    }
                    childExtraEncodeData->subtreeCompleted = true;
                }

                if (!childExtraEncodeData->elementCompleted || !childExtraEncodeData->subtreeCompleted) {
                    someChildTreeNotComplete = true;
                }
            }
        }
    }

    if (wantDebug) {
        qDebug() << "for this element: " << getAACube();
        qDebug() << "    WAS elementCompleted:" << thisExtraEncodeData->elementCompleted;
        qDebug() << "    WAS subtreeCompleted:" << thisExtraEncodeData->subtreeCompleted;
    }
    
    thisExtraEncodeData->subtreeCompleted = !someChildTreeNotComplete;

    if (wantDebug) {
        qDebug() << "    NOW elementCompleted:" << thisExtraEncodeData->elementCompleted;
        qDebug() << "    NOW subtreeCompleted:" << thisExtraEncodeData->subtreeCompleted;
    
        if (thisExtraEncodeData->subtreeCompleted) {
            qDebug() << "    YEAH!!!!! >>>>>>>>>>>>>> NOW subtreeCompleted:" << thisExtraEncodeData->subtreeCompleted;
        }
    }
}

OctreeElement::AppendState EntityTreeElement::appendElementData(OctreePacketData* packetData, 
                                                                    EncodeBitstreamParams& params) const {
                     
    OctreeElement::AppendState appendElementState = OctreeElement::COMPLETED; // assume the best...
    
    // first, check the params.extraEncodeData to see if there's any partial re-encode data for this element
    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData = NULL;
    bool hadElementExtraData = false;
    if (extraEncodeData && extraEncodeData->contains(this)) {
        entityTreeElementExtraEncodeData = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));
        hadElementExtraData = true;
    } else {
        // if there wasn't one already, then create one
        entityTreeElementExtraEncodeData = new EntityTreeElementExtraEncodeData();
        entityTreeElementExtraEncodeData->elementCompleted = (_entityItems->size() == 0);

        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            EntityTreeElement* child = getChildAtIndex(i);
            if (!child) {
                entityTreeElementExtraEncodeData->childCompleted[i] = true; // if no child exists, it is completed
            } else {
                if (child->hasEntities()) {
                    entityTreeElementExtraEncodeData->childCompleted[i] = false;
                } else {
                    entityTreeElementExtraEncodeData->childCompleted[i] = true; // if the child doesn't have enities, it is completed
                }
            }
        }
        for (uint16_t i = 0; i < _entityItems->size(); i++) {
            EntityItem* entity = (*_entityItems)[i];
            entityTreeElementExtraEncodeData->entities.insert(entity->getEntityItemID(), entity->getEntityProperties(params));
        }
    }

    //assert(extraEncodeData);
    //assert(extraEncodeData->contains(this));
    //entityTreeElementExtraEncodeData = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));

    LevelDetails elementLevel = packetData->startLevel();

    // write our entities out... first determine which of the entities are in view based on our params
    uint16_t numberOfEntities = 0;
    uint16_t actualNumberOfEntities = 0;
    QVector<uint16_t> indexesOfEntitiesToInclude;

    // It's possible that our element has been previous completed. In this case we'll simply not include any of our
    // entities for encoding. This is needed because we encode the element data at the "parent" level, and so we 
    // need to handle the case where our sibling elements need encoding but we don't.
    if (!entityTreeElementExtraEncodeData->elementCompleted) {
        for (uint16_t i = 0; i < _entityItems->size(); i++) {
            EntityItem* entity = (*_entityItems)[i];
            bool includeThisEntity = true;
            if (!params.forceSendScene && entity->getLastEdited() < params.lastViewFrustumSent) {
                includeThisEntity = false;
            }
        
            if (hadElementExtraData) {
                includeThisEntity = includeThisEntity && 
                                        entityTreeElementExtraEncodeData->entities.contains(entity->getEntityItemID());
            }
        
            if (includeThisEntity && params.viewFrustum) {
                AACube entityCube = entity->getAACube();
                entityCube.scale(TREE_SCALE);
                if (params.viewFrustum->cubeInFrustum(entityCube) == ViewFrustum::OUTSIDE) {
                    includeThisEntity = false; // out of view, don't include it
                }
            }
        
            if (includeThisEntity) {
                indexesOfEntitiesToInclude << i;
                numberOfEntities++;
            }
        }
    }

    int numberOfEntitiesOffset = packetData->getUncompressedByteOffset();
    bool successAppendEntityCount = packetData->appendValue(numberOfEntities);

    if (successAppendEntityCount) {
        foreach (uint16_t i, indexesOfEntitiesToInclude) {
            EntityItem* entity = (*_entityItems)[i];

            LevelDetails entityLevel = packetData->startLevel();
            OctreeElement::AppendState appendEntityState = entity->appendEntityData(packetData, 
                                                                        params, entityTreeElementExtraEncodeData);

            // If none of this entity data was able to be appended, then discard it
            // and don't include it in our entity count
            if (appendEntityState == OctreeElement::NONE) {
                packetData->discardLevel(entityLevel);
            } else {
                // If either ALL or some of it got appended, then end the level (commit it)
                // and include the entity in our final count of entities
                packetData->endLevel(entityLevel);
                actualNumberOfEntities++;
            }
            
            // If the entity item got completely appended, then we can remove it from the extra encode data
            if (appendEntityState == OctreeElement::COMPLETED) {
                entityTreeElementExtraEncodeData->entities.remove(entity->getEntityItemID());
            }

            // If any part of the entity items didn't fit, then the element is considered partial
            // NOTE: if the entity item didn't fit or only partially fit, then the entity item should have
            // added itself to the extra encode data.
            if (appendEntityState != OctreeElement::COMPLETED) {
                appendElementState = OctreeElement::PARTIAL;
            }
        }
    } else {
        // we we couldn't add the entity count, then we couldn't add anything for this element and we're in a NONE state
        appendElementState = OctreeElement::NONE;
    }

    // If we were provided with extraEncodeData, and we allocated and/or got entityTreeElementExtraEncodeData
    // then we need to do some additional processing, namely make sure our extraEncodeData is up to date for
    // this octree element.
    if (extraEncodeData && entityTreeElementExtraEncodeData) {

        // After processing, if we are PARTIAL or COMPLETED then we need to re-include our extra data. 
        // Only our patent can remove our extra data in these cases and only after it knows that all of it's 
        // children have been encoded.
        // If we weren't able to encode ANY data about ourselves, then we go ahead and remove our element data
        // since that will signal that the entire element needs to be encoded on the next attempt
        if (appendElementState == OctreeElement::NONE) {

            if (!entityTreeElementExtraEncodeData->elementCompleted && entityTreeElementExtraEncodeData->entities.size() == 0) {
                // TODO: we used to delete the extra encode data here. But changing the logic around
                // this is now a dead code branch. Clean this up!
            } else {
                // TODO: some of these inserts might be redundant!!!
                extraEncodeData->insert(this, entityTreeElementExtraEncodeData);
            }
        } else {
        
            // If we weren't previously completed, check to see if we are
            if (!entityTreeElementExtraEncodeData->elementCompleted) {
                // If all of our items have been encoded, then we are complete as an element.
                if (entityTreeElementExtraEncodeData->entities.size() == 0) {
                    entityTreeElementExtraEncodeData->elementCompleted = true;
                }
            }

            // TODO: some of these inserts might be redundant!!!
            extraEncodeData->insert(this, entityTreeElementExtraEncodeData);
        }
    }

    // Determine if no entities at all were able to fit    
    bool noEntitiesFit = (numberOfEntities > 0 && actualNumberOfEntities == 0);
    
    // If we wrote fewer entities than we expected, update the number of entities in our packet
    bool successUpdateEntityCount = true;
    if (!noEntitiesFit && numberOfEntities != actualNumberOfEntities) {
        successUpdateEntityCount = packetData->updatePriorBytes(numberOfEntitiesOffset,
                                            (const unsigned char*)&actualNumberOfEntities, sizeof(actualNumberOfEntities));
    }

    // If we weren't able to update our entity count, or we couldn't fit any entities, then
    // we should discard our element and return a result of NONE
    if (!successUpdateEntityCount || noEntitiesFit) {
        packetData->discardLevel(elementLevel);
        appendElementState = OctreeElement::NONE;
    } else {
        packetData->endLevel(elementLevel);
    }
    return appendElementState;
}

bool EntityTreeElement::containsEntityBounds(const EntityItem* entity) const {
    return containsBounds(entity->getMinimumPoint(), entity->getMaximumPoint());
}

bool EntityTreeElement::bestFitEntityBounds(const EntityItem* entity) const {
    return bestFitBounds(entity->getMinimumPoint(), entity->getMaximumPoint());
}

bool EntityTreeElement::containsBounds(const EntityItemProperties& properties) const {
    return containsBounds(properties.getMinimumPointTreeUnits(), properties.getMaximumPointTreeUnits());
}

bool EntityTreeElement::bestFitBounds(const EntityItemProperties& properties) const {
    return bestFitBounds(properties.getMinimumPointTreeUnits(), properties.getMaximumPointTreeUnits());
}

bool EntityTreeElement::containsBounds(const AACube& bounds) const {
    return containsBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::bestFitBounds(const AACube& bounds) const {
    return bestFitBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::containsBounds(const AABox& bounds) const {
    return containsBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::bestFitBounds(const AABox& bounds) const {
    return bestFitBounds(bounds.getMinimumPoint(), bounds.getMaximumPoint());
}

bool EntityTreeElement::containsBounds(const glm::vec3& minPoint, const glm::vec3& maxPoint) const {
    glm::vec3 clampedMin = glm::clamp(minPoint, 0.0f, 1.0f);
    glm::vec3 clampedMax = glm::clamp(maxPoint, 0.0f, 1.0f);
    return _cube.contains(clampedMin) && _cube.contains(clampedMax);
}

bool EntityTreeElement::bestFitBounds(const glm::vec3& minPoint, const glm::vec3& maxPoint) const {
    glm::vec3 clampedMin = glm::clamp(minPoint, 0.0f, 1.0f);
    glm::vec3 clampedMax = glm::clamp(maxPoint, 0.0f, 1.0f);

    if (_cube.contains(clampedMin) && _cube.contains(clampedMax)) {
        
        // If our child would be smaller than our smallest reasonable element, then we are the best fit.
        float childScale = _cube.getScale() / 2.0f;
        if (childScale <= SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE) {
            return true;
        }
        int childForMinimumPoint = getMyChildContainingPoint(clampedMin);
        int childForMaximumPoint = getMyChildContainingPoint(clampedMax);

        // If I contain both the minimum and maximum point, but two different children of mine
        // contain those points, then I am the best fit for that entity
        if (childForMinimumPoint != childForMaximumPoint) {
            return true;
        }
    }
    return false;
}

bool EntityTreeElement::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject) {

    // only called if we do intersect our bounding cube, but find if we actually intersect with entities...
    
    QList<EntityItem*>::iterator entityItr = _entityItems->begin();
    QList<EntityItem*>::const_iterator entityEnd = _entityItems->end();
    bool somethingIntersected = false;
    while(entityItr != entityEnd) {
        EntityItem* entity = (*entityItr);
        
        AACube entityCube = entity->getAACube();
        float localDistance;
        BoxFace localFace;

        // if the ray doesn't intersect with our cube, we can stop searching!
        if (entityCube.findRayIntersection(origin, direction, localDistance, localFace)) {
            const FBXGeometry* fbxGeometry = _myTree->getGeometryForEntity(entity);
            if (fbxGeometry && fbxGeometry->meshExtents.isValid()) {
                Extents extents = fbxGeometry->meshExtents;

                // NOTE: If the entity has a bad mesh, then extents will be 0,0,0 & 0,0,0
                if (extents.minimum == extents.maximum && extents.minimum == glm::vec3(0,0,0)) {
                    extents.maximum = glm::vec3(1.0f,1.0f,1.0f); // in this case we will simulate the unit cube
                }

                // NOTE: these extents are entity space, so we need to scale and center them accordingly
                // size is our "target size in world space"
                // we need to set our entity scale so that the extents of the mesh, fit in a cube that size...
                float maxDimension = glm::distance(extents.maximum, extents.minimum);
                float scale = entity->getSize() / maxDimension;

                glm::vec3 halfDimensions = (extents.maximum - extents.minimum) * 0.5f;
                glm::vec3 offset = -extents.minimum - halfDimensions;
                
                extents.minimum += offset;
                extents.maximum += offset;

                extents.minimum *= scale;
                extents.maximum *= scale;
                
                Extents rotatedExtents = extents;

                calculateRotatedExtents(rotatedExtents, entity->getRotation());

                rotatedExtents.minimum += entity->getPosition();
                rotatedExtents.maximum += entity->getPosition();


                AABox rotatedExtentsBox(rotatedExtents.minimum, (rotatedExtents.maximum - rotatedExtents.minimum));
                
                // if it's in our AABOX for our rotated extents, then check to see if it's in our non-AABox
                if (rotatedExtentsBox.findRayIntersection(origin, direction, localDistance, localFace)) {
                
                    // extents is the entity relative, scaled, centered extents of the entity
                    glm::mat4 rotation = glm::mat4_cast(entity->getRotation());
                    glm::mat4 translation = glm::translate(entity->getPosition());
                    glm::mat4 entityToWorldMatrix = translation * rotation;
                    glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

                    AABox entityFrameBox(extents.minimum, (extents.maximum - extents.minimum));

                    glm::vec3 entityFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
                    glm::vec3 entityFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));

                    // we can use the AABox's ray intersection by mapping our origin and direction into the entity frame
                    // and testing intersection there.
                    if (entityFrameBox.findRayIntersection(entityFrameOrigin, entityFrameDirection, localDistance, localFace)) {
                        if (localDistance < distance) {
                            distance = localDistance;
                            face = localFace;
                            *intersectedObject = (void*)entity;
                            somethingIntersected = true;
                        }
                    }
                }
            } else if (localDistance < distance) {
                distance = localDistance;
                face = localFace;
                *intersectedObject = (void*)entity;
                somethingIntersected = true;
            }
        }
        
        ++entityItr;
    }
    return somethingIntersected;
}

// TODO: change this to use better bounding shape for entity than sphere
bool EntityTreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                                    glm::vec3& penetration, void** penetratedObject) const {
    QList<EntityItem*>::iterator entityItr = _entityItems->begin();
    QList<EntityItem*>::const_iterator entityEnd = _entityItems->end();
    while(entityItr != entityEnd) {
        EntityItem* entity = (*entityItr);
        glm::vec3 entityCenter = entity->getPosition();
        float entityRadius = entity->getRadius();

        // don't penetrate yourself
        if (entityCenter == center && entityRadius == radius) {
            return false;
        }

        if (findSphereSpherePenetration(center, radius, entityCenter, entityRadius, penetration)) {
            // return true on first valid entity penetration
            *penetratedObject = (void*)(entity);
            return true;
        }
        ++entityItr;
    }
    return false;
}

void EntityTreeElement::updateEntityItemID(const EntityItemID& creatorTokenEntityID, const EntityItemID& knownIDEntityID) {
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem* thisEntity = (*_entityItems)[i];

        EntityItemID thisEntityID = thisEntity->getEntityItemID();
        
        if (thisEntityID == creatorTokenEntityID) {
            thisEntity->setID(knownIDEntityID.id);
        }
    }
}

const EntityItem* EntityTreeElement::getClosestEntity(glm::vec3 position) const {
    const EntityItem* closestEntity = NULL;
    float closestEntityDistance = FLT_MAX;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        float distanceToEntity = glm::distance(position, (*_entityItems)[i]->getPosition());
        if (distanceToEntity < closestEntityDistance) {
            closestEntity = (*_entityItems)[i];
        }
    }
    return closestEntity;
}

// TODO: change this to use better bounding shape for entity than sphere
void EntityTreeElement::getEntities(const glm::vec3& searchPosition, float searchRadius, QVector<const EntityItem*>& foundEntities) const {
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        const EntityItem* entity = (*_entityItems)[i];
        float distance = glm::length(entity->getPosition() - searchPosition);
        if (distance < searchRadius + entity->getRadius()) {
            foundEntities.push_back(entity);
        }
    }
}

// TODO: change this to use better bounding shape for entity than sphere
void EntityTreeElement::getEntities(const AACube& box, QVector<EntityItem*>& foundEntities) {
    QList<EntityItem*>::iterator entityItr = _entityItems->begin();
    QList<EntityItem*>::iterator entityEnd = _entityItems->end();
    AACube entityCube;
    while(entityItr != entityEnd) {
        EntityItem* entity = (*entityItr);
        float radius = entity->getRadius();
        // NOTE: we actually do cube-cube collision queries here, which is sloppy but good enough for now
        // TODO: decide whether to replace entityCube-cube query with sphere-cube (requires a square root
        // but will be slightly more accurate).
        entityCube.setBox(entity->getPosition() - glm::vec3(radius), 2.f * radius);
        if (entityCube.touches(box)) {
            foundEntities.push_back(entity);
        }
        ++entityItr;
    }
}

const EntityItem* EntityTreeElement::getEntityWithEntityItemID(const EntityItemID& id) const {
    const EntityItem* foundEntity = NULL;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        if ((*_entityItems)[i]->getEntityItemID() == id) {
            foundEntity = (*_entityItems)[i];
            break;
        }
    }
    return foundEntity;
}

EntityItem* EntityTreeElement::getEntityWithEntityItemID(const EntityItemID& id) {
    EntityItem* foundEntity = NULL;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        if ((*_entityItems)[i]->getEntityItemID() == id) {
            foundEntity = (*_entityItems)[i];
            break;
        }
    }
    return foundEntity;
}

void EntityTreeElement::cleanupEntities() {
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem* entity = (*_entityItems)[i];
        delete entity;
    }
    _entityItems->clear();
}

bool EntityTreeElement::removeEntityWithEntityItemID(const EntityItemID& id) {
    bool foundEntity = false;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        if ((*_entityItems)[i]->getEntityItemID() == id) {
            foundEntity = true;
            _entityItems->removeAt(i);
            break;
        }
    }
    return foundEntity;
}

bool EntityTreeElement::removeEntityItem(EntityItem* entity) {
    return _entityItems->removeAll(entity) > 0;
}


// Things we want to accomplish as we read these entities from the data buffer.
//
// 1) correctly update the properties of the entity
// 2) add any new entities that didn't previously exist
//
// TODO: Do we also need to do this?
//    3) mark our tree as dirty down to the path of the previous location of the entity
//    4) mark our tree as dirty down to the path of the new location of the entity
//
// Since we're potentially reading several entities, we'd prefer to do all the moving around
// and dirty path marking in one pass.
int EntityTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
            ReadBitstreamToTreeParams& args) {

    // If we're the root, but this bitstream doesn't support root elements with data, then
    // return without reading any bytes
    if (this == _myTree->getRoot() && args.bitstreamVersion < VERSION_ROOT_ELEMENT_HAS_DATA) {
        return 0;
    }
    
    const unsigned char* dataAt = data;
    int bytesRead = 0;
    uint16_t numberOfEntities = 0;
    int expectedBytesPerEntity = EntityItem::expectedBytes();

    if (bytesLeftToRead >= (int)sizeof(numberOfEntities)) {
        // read our entities in....
        numberOfEntities = *(uint16_t*)dataAt;

        dataAt += sizeof(numberOfEntities);
        bytesLeftToRead -= (int)sizeof(numberOfEntities);
        bytesRead += sizeof(numberOfEntities);

        if (bytesLeftToRead >= (int)(numberOfEntities * expectedBytesPerEntity)) {
            for (uint16_t i = 0; i < numberOfEntities; i++) {
                int bytesForThisEntity = 0;
                EntityItemID entityItemID;
                EntityItem* entityItem = NULL;

                // Old model files don't have UUIDs in them. So we don't want to try to read those IDs from the stream.
                // Since this can only happen on loading an old file, we can safely treat these as new entity cases,
                // which will correctly handle the case of creating models and letting them parse the old format.
                if (args.bitstreamVersion >= VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
                    entityItemID = EntityItemID::readEntityItemIDFromBuffer(dataAt, bytesLeftToRead);
                    entityItem = _myTree->findEntityByEntityItemID(entityItemID);
                }
                
                // If the item already exists in our tree, we want do the following...
                // 1) allow the existing item to read from the databuffer
                // 2) check to see if after reading the item, the containing element is still correct, fix it if needed
                //
                // TODO: Do we need to also do this?
                //    3) remember the old cube for the entity so we can mark it as dirty
                if (entityItem) {
                    bool bestFitBefore = bestFitEntityBounds(entityItem);
                    EntityTreeElement* currentContainingElement = _myTree->getContainingElement(entityItemID);
                    EntityItem::SimulationState oldState = entityItem->getSimulationState();
                    bytesForThisEntity = entityItem->readEntityDataFromBuffer(dataAt, bytesLeftToRead, args);
                    EntityItem::SimulationState newState = entityItem->getSimulationState();
                    if (oldState != newState) {
                        _myTree->changeEntityState(entityItem, oldState, newState);
                    }
                    bool bestFitAfter = bestFitEntityBounds(entityItem);

                    if (bestFitBefore != bestFitAfter) {
                        // This is the case where the entity existed, and is in some element in our tree...                    
                        if (!bestFitBefore && bestFitAfter) {
                            // This is the case where the entity existed, and is in some element in our tree...                    
                            if (currentContainingElement != this) {
                                currentContainingElement->removeEntityItem(entityItem);
                                addEntityItem(entityItem);
                                _myTree->setContainingElement(entityItemID, this);
                            }
                        }
                    }
                } else {
                    entityItem = EntityTypes::constructEntityItem(dataAt, bytesLeftToRead, args);
                    if (entityItem) {
                        bytesForThisEntity = entityItem->readEntityDataFromBuffer(dataAt, bytesLeftToRead, args);
                        addEntityItem(entityItem); // add this new entity to this elements entities
                        entityItemID = entityItem->getEntityItemID();
                        _myTree->setContainingElement(entityItemID, this);
                        EntityItem::SimulationState newState = entityItem->getSimulationState();
                        _myTree->changeEntityState(entityItem, EntityItem::Static, newState);
                    }
                }
                // Move the buffer forward to read more entities
                dataAt += bytesForThisEntity;
                bytesLeftToRead -= bytesForThisEntity;
                bytesRead += bytesForThisEntity;
            }
        }
    }
    
    return bytesRead;
}

void EntityTreeElement::addEntityItem(EntityItem* entity) {
    _entityItems->push_back(entity);
}

// will average a "common reduced LOD view" from the the child elements...
void EntityTreeElement::calculateAverageFromChildren() {
    // nothing to do here yet...
}

// will detect if children are leaves AND collapsable into the parent node
// and in that case will collapse children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a
// single node
bool EntityTreeElement::collapseChildren() {
    // nothing to do here yet...
    return false;
}

bool EntityTreeElement::pruneChildren() {
    bool somethingPruned = false;
    for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; childIndex++) {
        EntityTreeElement* child = getChildAtIndex(childIndex);
        
        // if my child is a leaf, but has no entities, then it's safe to delete my child
        if (child && child->isLeaf() && !child->hasEntities()) {
            deleteChildAtIndex(childIndex);
            somethingPruned = true;
        }
    }
    return somethingPruned;
}


void EntityTreeElement::debugDump() {
    qDebug() << "EntityTreeElement...";
    qDebug() << "entity count:" << _entityItems->size();
    qDebug() << "cube:" << getAACube();
    for (uint16_t i = 0; i < _entityItems->size(); i++) {
        EntityItem* entity = (*_entityItems)[i];
        entity->debugDump();
    }
}
    

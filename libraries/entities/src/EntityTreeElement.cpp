//
//  EntityTreeElement.cpp
//  libraries/models/src
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
    //qDebug() << "EntityTreeElement::~EntityTreeElement() this=" << this;
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
    _entityItems = new QList<EntityItem>;
    _voxelMemoryUsage += sizeof(EntityTreeElement);
}

EntityTreeElement* EntityTreeElement::addChildAtIndex(int index) {
    EntityTreeElement* newElement = (EntityTreeElement*)OctreeElement::addChildAtIndex(index);
    newElement->setTree(_myTree);
    return newElement;
}


// TODO: This will attempt to store as many models as will fit in the packetData, if an individual model won't
// fit, but some models did fit, then the element outputs what can fit. Once the general Octree::encodeXXX()
// process supports partial encoding of an octree element, this will need to be updated to handle spanning its
// contents across multiple packets.
OctreeElement::AppendState EntityTreeElement::appendElementData(OctreePacketData* packetData, 
                                                                    EncodeBitstreamParams& params) const {

    OctreeElement::AppendState appendElementState = OctreeElement::COMPLETED; // assume the best...
    
    // first, check the params.extraEncodeData to see if there's any partial re-encode data for this element
    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData = NULL;
    bool hadElementExtraData = false;
    if (extraEncodeData && extraEncodeData->contains(this)) {
        modelTreeElementExtraEncodeData = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));
        hadElementExtraData = true;
    } else {
        // if there wasn't one already, then create one
        modelTreeElementExtraEncodeData = new EntityTreeElementExtraEncodeData();
    }

    LevelDetails elementLevel = packetData->startLevel();

    // write our models out... first determine which of the models are in view based on our params
    uint16_t numberOfEntities = 0;
    uint16_t actualNumberOfEntities = 0;
    QVector<uint16_t> indexesOfEntitiesToInclude;

    for (uint16_t i = 0; i < _entityItems->size(); i++) {
        const EntityItem& model = (*_entityItems)[i];
        bool includeThisEntity = true;
        
        if (hadElementExtraData) {
            includeThisEntity = modelTreeElementExtraEncodeData->includedItems.contains(model.getEntityItemID());
        }
        
        if (includeThisEntity && params.viewFrustum) {
            AACube modelCube = model.getAACube();
            modelCube.scale(TREE_SCALE);
            if (params.viewFrustum->cubeInFrustum(modelCube) == ViewFrustum::OUTSIDE) {
                includeThisEntity = false; // out of view, don't include it
            }
        }
        
        if (includeThisEntity) {
            indexesOfEntitiesToInclude << i;
            numberOfEntities++;
        }
    }

    int numberOfEntitiesOffset = packetData->getUncompressedByteOffset();
    bool successAppendEntityCount = packetData->appendValue(numberOfEntities);

    if (successAppendEntityCount) {
        foreach (uint16_t i, indexesOfEntitiesToInclude) {
            const EntityItem& model = (*_entityItems)[i];
            
            LevelDetails modelLevel = packetData->startLevel();
    
            OctreeElement::AppendState appendEntityState = model.appendEntityData(packetData, params, modelTreeElementExtraEncodeData);

            // If none of this model data was able to be appended, then discard it
            // and don't include it in our model count
            if (appendEntityState == OctreeElement::NONE) {
                packetData->discardLevel(modelLevel);
            } else {
                // If either ALL or some of it got appended, then end the level (commit it)
                // and include the model in our final count of models
                packetData->endLevel(modelLevel);
                actualNumberOfEntities++;
            }
            
            // If the model item got completely appended, then we can remove it from the extra encode data
            if (appendEntityState == OctreeElement::COMPLETED) {
                modelTreeElementExtraEncodeData->includedItems.remove(model.getEntityItemID());
            }

            // If any part of the model items didn't fit, then the element is considered partial
            // NOTE: if the model item didn't fit or only partially fit, then the model item should have
            // added itself to the extra encode data.
            if (appendEntityState != OctreeElement::COMPLETED) {
                appendElementState = OctreeElement::PARTIAL;
            }
        }
    }
    
    // If we were provided with extraEncodeData, and we allocated and/or got modelTreeElementExtraEncodeData
    // then we need to do some additional processing, namely make sure our extraEncodeData is up to date for
    // this octree element.
    if (extraEncodeData && modelTreeElementExtraEncodeData) {

        // If after processing we have some includedItems left in it, then make sure we re-add it back to our map
        if (modelTreeElementExtraEncodeData->includedItems.size()) {
            extraEncodeData->insert(this, modelTreeElementExtraEncodeData);
        } else {
            // otherwise, clean things up...
            extraEncodeData->remove(this);
            delete modelTreeElementExtraEncodeData;
        }
    }

    // Determine if no models at all were able to fit    
    bool noEntitiesFit = (numberOfEntities > 0 && actualNumberOfEntities == 0);
    
    // If we wrote fewer models than we expected, update the number of models in our packet
    bool successUpdateEntityCount = true;
    if (!noEntitiesFit && numberOfEntities != actualNumberOfEntities) {
        successUpdateEntityCount = packetData->updatePriorBytes(numberOfEntitiesOffset,
                                            (const unsigned char*)&actualNumberOfEntities, sizeof(actualNumberOfEntities));
    }

    // If we weren't able to update our model count, or we couldn't fit any models, then
    // we should discard our element and return a result of NONE
    if (!successUpdateEntityCount || noEntitiesFit) {
        packetData->discardLevel(elementLevel);
        appendElementState = OctreeElement::NONE;
    } else {
        packetData->endLevel(elementLevel);
    }
    
    return appendElementState;
}

bool EntityTreeElement::containsEntityBounds(const EntityItem& model) const {
    glm::vec3 clampedMin = glm::clamp(model.getMinimumPoint(), 0.0f, 1.0f);
    glm::vec3 clampedMax = glm::clamp(model.getMaximumPoint(), 0.0f, 1.0f);
    return _cube.contains(clampedMin) && _cube.contains(clampedMax);
}

bool EntityTreeElement::bestFitEntityBounds(const EntityItem& model) const {
    glm::vec3 clampedMin = glm::clamp(model.getMinimumPoint(), 0.0f, 1.0f);
    glm::vec3 clampedMax = glm::clamp(model.getMaximumPoint(), 0.0f, 1.0f);
    if (_cube.contains(clampedMin) && _cube.contains(clampedMax)) {
        int childForMinimumPoint = getMyChildContainingPoint(clampedMin);
        int childForMaximumPoint = getMyChildContainingPoint(clampedMax);
        
        // if this is a really small box, then it's close enough!
        if (_cube.getScale() <= SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE) {
            return true;
        }
        // If I contain both the minimum and maximum point, but two different children of mine
        // contain those points, then I am the best fit for that model
        if (childForMinimumPoint != childForMaximumPoint) {
            return true;
        }
    }
    return false;
}

void EntityTreeElement::update(EntityTreeUpdateArgs& args) {
    args._totalElements++;
    // update our contained models
    QList<EntityItem>::iterator modelItr = _entityItems->begin();
    while(modelItr != _entityItems->end()) {
        EntityItem& model = (*modelItr);
        args._totalItems++;
        
        // TODO: this _lastChanged isn't actually changing because we're not marking this element as changed.
        // how do we want to handle this??? We really only want to consider an element changed when it is
        // edited... not just animated...
        model.update(_lastChanged);

        // If the model wants to die, or if it's left our bounding box, then move it
        // into the arguments moving models. These will be added back or deleted completely
        if (model.getShouldBeDeleted() || !bestFitEntityBounds(model)) {
            args._movingEntities.push_back(model);

            // erase this model
            modelItr = _entityItems->erase(modelItr);

            args._movingItems++;
            
            // this element has changed so mark it...
            markWithChangedTime();

            // TODO: is this a good place to change the containing element map???
            qDebug() << "EntityTreeElement::update()... calling _myTree->setContainingElement(model.getEntityItemID(), NULL); ********";
            _myTree->setContainingElement(model.getEntityItemID(), NULL);

        } else {
            ++modelItr;
        }
    }
}

bool EntityTreeElement::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject) {

    // only called if we do intersect our bounding cube, but find if we actually intersect with models...
    
    QList<EntityItem>::iterator entityItr = _entityItems->begin();
    QList<EntityItem>::const_iterator entityEnd = _entityItems->end();
    bool somethingIntersected = false;
    while(entityItr != entityEnd) {
        EntityItem& entity = (*entityItr);
        
        AACube entityCube = entity.getAACube();
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
                float scale = entity.getSize() / maxDimension;

                glm::vec3 halfDimensions = (extents.maximum - extents.minimum) * 0.5f;
                glm::vec3 offset = -extents.minimum - halfDimensions;
                
                extents.minimum += offset;
                extents.maximum += offset;

                extents.minimum *= scale;
                extents.maximum *= scale;
                
                Extents rotatedExtents = extents;

                calculateRotatedExtents(rotatedExtents, entity.getRotation());

                rotatedExtents.minimum += entity.getPosition();
                rotatedExtents.maximum += entity.getPosition();


                AABox rotatedExtentsBox(rotatedExtents.minimum, (rotatedExtents.maximum - rotatedExtents.minimum));
                
                // if it's in our AABOX for our rotated extents, then check to see if it's in our non-AABox
                if (rotatedExtentsBox.findRayIntersection(origin, direction, localDistance, localFace)) {
                
                    // extents is the entity relative, scaled, centered extents of the entity
                    glm::mat4 rotation = glm::mat4_cast(entity.getRotation());
                    glm::mat4 translation = glm::translate(entity.getPosition());
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
                            *intersectedObject = (void*)(&entity);
                            somethingIntersected = true;
                        }
                    }
                }
            } else if (localDistance < distance) {
                distance = localDistance;
                face = localFace;
                *intersectedObject = (void*)(&entity);
                somethingIntersected = true;
            }
        }
        
        ++entityItr;
    }
    return somethingIntersected;
}

bool EntityTreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                                    glm::vec3& penetration, void** penetratedObject) const {
    QList<EntityItem>::iterator entityItr = _entityItems->begin();
    QList<EntityItem>::const_iterator entityEnd = _entityItems->end();
    while(entityItr != entityEnd) {
        EntityItem& entity = (*entityItr);
        glm::vec3 entityCenter = entity.getPosition();
        float entityRadius = entity.getRadius();

        // don't penetrate yourself
        if (entityCenter == center && entityRadius == radius) {
            return false;
        }

        if (findSphereSpherePenetration(center, radius, entityCenter, entityRadius, penetration)) {
            // return true on first valid entity penetration
            *penetratedObject = (void*)(&entity);
            return true;
        }
        ++entityItr;
    }
    return false;
}

bool EntityTreeElement::updateEntity(const EntityItem& entity) {
    const bool wantDebug = false;
    if (wantDebug) {
        EntityItemID entityItemID = entity.getEntityItemID();
        qDebug() << "EntityTreeElement::updateEntity(entity) entityID.id="
                        << entityItemID.id << "creatorTokenID=" << entityItemID.creatorTokenID;
    }
    
    // NOTE: this method must first lookup the entity by ID, hence it is O(N)
    // and "entity is not found" is worst-case (full N) but maybe we don't care?
    // (guaranteed that num entitys per elemen is small?)
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem& thisEntity = (*_entityItems)[i];
        if (thisEntity.getID() == entity.getID()) {
            if (wantDebug) {
                qDebug() << "found entity with id";
            }
            int difference = thisEntity.getLastUpdated() - entity.getLastUpdated();
            bool changedOnServer = thisEntity.getLastEdited() <= entity.getLastEdited();
            bool localOlder = thisEntity.getLastUpdated() < entity.getLastUpdated();
            if (changedOnServer || localOlder) {
                if (wantDebug) {
                    qDebug("local entity [id:%d] %s and %s than server entity by %d, entity.isNewlyCreated()=%s",
                            entity.getID(), (changedOnServer ? "CHANGED" : "same"),
                            (localOlder ? "OLDER" : "NEWER"),
                            difference, debug::valueOf(entity.isNewlyCreated()) );
                }
                
                thisEntity.copyChangedProperties(entity);
                markWithChangedTime();
                
                // seems like we shouldn't need this
                _myTree->setContainingElement(entity.getEntityItemID(), this);
            } else {
                if (wantDebug) {
                    qDebug(">>> IGNORING SERVER!!! Would've caused jutter! <<<  "
                            "local entity [id:%d] %s and %s than server entity by %d, entity.isNewlyCreated()=%s",
                            entity.getID(), (changedOnServer ? "CHANGED" : "same"),
                            (localOlder ? "OLDER" : "NEWER"),
                            difference, debug::valueOf(entity.isNewlyCreated()) );
                }
            }
            return true;
        }
    }
    
    // If we didn't find the entity here, then let's check to see if we should add it...
    if (bestFitEntityBounds(entity)) {
        _entityItems->push_back(entity);
        markWithChangedTime();
        // Since we're adding this item to this element, we need to let the tree know about it
        _myTree->setContainingElement(entity.getEntityItemID(), this);
        return true;
    }
    
    return false;
}

void EntityTreeElement::updateEntityItemID(FindAndUpdateEntityItemIDArgs* args) {
    bool wantDebug = false;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem& thisEntity = (*_entityItems)[i];
        
        if (!args->creatorTokenFound) {
            // first, we're looking for matching creatorTokenIDs, if we find that, then we fix it to know the actual ID
            if (thisEntity.getCreatorTokenID() == args->creatorTokenID) {
                if (wantDebug) {
                    qDebug() << "EntityTreeElement::updateEntityItemID()... found the entity... updating it's ID... "
                        << "creatorTokenID=" << args->creatorTokenID
                        << "entityID=" << args->entityID;
                }

                thisEntity.setID(args->entityID);
                args->creatorTokenFound = true;
            }
        }
        
        // if we're in an isViewing tree, we also need to look for an kill any viewed entitys
        if (!args->viewedEntityFound && args->isViewing) {
            if (thisEntity.getCreatorTokenID() == UNKNOWN_MODEL_TOKEN && thisEntity.getID() == args->entityID) {

                if (wantDebug) {
                    qDebug() << "EntityTreeElement::updateEntityItemID()... VIEWED entity FOUND??? "
                        << "args->creatorTokenID=" << args->creatorTokenID
                        << "thisEntity.getCreatorTokenID()=" << thisEntity.getCreatorTokenID()
                        << "args->entityID=" << args->entityID;
                }

                _entityItems->removeAt(i); // remove the entity at this index
                numberOfEntities--; // this means we have 1 fewer entity in this list
                i--; // and we actually want to back up i as well.
                args->viewedEntityFound = true;
            }
        }
    }
}



const EntityItem* EntityTreeElement::getClosestEntity(glm::vec3 position) const {
    const EntityItem* closestEntity = NULL;
    float closestEntityDistance = FLT_MAX;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        float distanceToEntity = glm::distance(position, (*_entityItems)[i].getPosition());
        if (distanceToEntity < closestEntityDistance) {
            closestEntity = &(*_entityItems)[i];
        }
    }
    return closestEntity;
}

void EntityTreeElement::getEntities(const glm::vec3& searchPosition, float searchRadius, QVector<const EntityItem*>& foundEntities) const {
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        const EntityItem* entity = &(*_entityItems)[i];
        float distance = glm::length(entity->getPosition() - searchPosition);
        if (distance < searchRadius + entity->getRadius()) {
            foundEntities.push_back(entity);
        }
    }
}

void EntityTreeElement::getEntities(const AACube& box, QVector<EntityItem*>& foundEntities) {
    QList<EntityItem>::iterator entityItr = _entityItems->begin();
    QList<EntityItem>::iterator entityEnd = _entityItems->end();
    AACube entityCube;
    while(entityItr != entityEnd) {
        EntityItem* entity = &(*entityItr);
        float radius = entity->getRadius();
        // NOTE: we actually do cube-cube collision queries here, which is sloppy but good enough for now
        // TODO: decide whether to replace entityCube-cube query with sphere-cube (requires a square root
        // but will be slightly more accurate).
        entityCube.setBox(entity->getPosition() - glm::vec3(radius), 2.f * radius);
        if (entityCube.touches(_cube)) {
            foundEntities.push_back(entity);
        }
        ++entityItr;
    }
}

const EntityItem* EntityTreeElement::getEntityWithID(uint32_t id) const {
    // NOTE: this lookup is O(N) but maybe we don't care? (guaranteed that num entitys per elemen is small?)
    const EntityItem* foundEntity = NULL;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        if ((*_entityItems)[i].getID() == id) {
            foundEntity = &(*_entityItems)[i];
            break;
        }
    }
    return foundEntity;
}

const EntityItem* EntityTreeElement::getEntityWithEntityItemID(const EntityItemID& id) const {
    // NOTE: this lookup is O(N) but maybe we don't care? (guaranteed that num entitys per elemen is small?)
    const EntityItem* foundEntity = NULL;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        if ((*_entityItems)[i].getEntityItemID() == id) {
            foundEntity = &(*_entityItems)[i];
            break;
        }
    }
    return foundEntity;
}

bool EntityTreeElement::removeEntityWithID(uint32_t id) {
    bool foundEntity = false;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        if ((*_entityItems)[i].getID() == id) {
            foundEntity = true;
            _entityItems->removeAt(i);
            
            break;
        }
    }
    return foundEntity;
}

bool EntityTreeElement::removeEntityWithEntityItemID(const EntityItemID& id) {
    bool foundEntity = false;
    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        if ((*_entityItems)[i].getEntityItemID() == id) {
            foundEntity = true;
            _entityItems->removeAt(i);
            break;
        }
    }
    return foundEntity;
}

int EntityTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
            ReadBitstreamToTreeParams& args) {

    // If we're the root, but this bitstream doesn't support root elements with data, then
    // return without reading any bytes
    if (this == _myTree->getRoot() && args.bitstreamVersion < VERSION_ROOT_ELEMENT_HAS_DATA) {
        qDebug() << "ROOT ELEMENT: no root data for "
                    "bitstreamVersion=" << (int)args.bitstreamVersion << " bytesLeftToRead=" << bytesLeftToRead;
        return 0;
    }

    const unsigned char* dataAt = data;
    int bytesRead = 0;
    uint16_t numberOfEntities = 0;
    int expectedBytesPerEntity = EntityItem::expectedBytes();

    if (bytesLeftToRead >= (int)sizeof(numberOfEntities)) {
        // read our entitys in....
        numberOfEntities = *(uint16_t*)dataAt;

        dataAt += sizeof(numberOfEntities);
        bytesLeftToRead -= (int)sizeof(numberOfEntities);
        bytesRead += sizeof(numberOfEntities);
        
        if (bytesLeftToRead >= (int)(numberOfEntities * expectedBytesPerEntity)) {
            for (uint16_t i = 0; i < numberOfEntities; i++) {
                EntityItem tempEntity; // we will read into this
                EntityItemID entityItemID = EntityItem::readEntityItemIDFromBuffer(dataAt, bytesLeftToRead, args);
                const EntityItem* existingEntityItem = _myTree->findEntityByEntityItemID(entityItemID);
                if (existingEntityItem) {
                    // copy original properties...
                    tempEntity.copyChangedProperties(*existingEntityItem); 
                }
                // read only the changed properties
                int bytesForThisEntity = tempEntity.readEntityDataFromBuffer(dataAt, bytesLeftToRead, args);
                
                _myTree->storeEntity(tempEntity);
                dataAt += bytesForThisEntity;
                bytesLeftToRead -= bytesForThisEntity;
                bytesRead += bytesForThisEntity;
            }
        }
    }

    return bytesRead;
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

//
//  ModelTreeElement.cpp
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

#include "ModelTree.h"
#include "ModelTreeElement.h"

ModelTreeElement::ModelTreeElement(unsigned char* octalCode) : OctreeElement(), _modelItems(NULL) {
    init(octalCode);
};

ModelTreeElement::~ModelTreeElement() {
    _voxelMemoryUsage -= sizeof(ModelTreeElement);
    delete _modelItems;
    _modelItems = NULL;
}

// This will be called primarily on addChildAt(), which means we're adding a child of our
// own type to our own tree. This means we should initialize that child with any tree and type
// specific settings that our children must have. One example is out VoxelSystem, which
// we know must match ours.
OctreeElement* ModelTreeElement::createNewElement(unsigned char* octalCode) {
    ModelTreeElement* newChild = new ModelTreeElement(octalCode);
    newChild->setTree(_myTree);
    return newChild;
}

void ModelTreeElement::init(unsigned char* octalCode) {
    OctreeElement::init(octalCode);
    _modelItems = new QList<ModelItem>;
    _voxelMemoryUsage += sizeof(ModelTreeElement);
}

ModelTreeElement* ModelTreeElement::addChildAtIndex(int index) {
    ModelTreeElement* newElement = (ModelTreeElement*)OctreeElement::addChildAtIndex(index);
    newElement->setTree(_myTree);
    return newElement;
}


// TODO: This will attempt to store as many models as will fit in the packetData, if an individual model won't
// fit, but some models did fit, then the element outputs what can fit. Once the general Octree::encodeXXX()
// process supports partial encoding of an octree element, this will need to be updated to handle spanning its
// contents across multiple packets.
bool ModelTreeElement::appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const {
    bool success = true; // assume the best...

    // write our models out... first determine which of the models are in view based on our params
    uint16_t numberOfModels = 0;
    uint16_t actualNumberOfModels = 0;
    QVector<uint16_t> indexesOfModelsToInclude;

    for (uint16_t i = 0; i < _modelItems->size(); i++) {
        if (params.viewFrustum) {
            const ModelItem& model = (*_modelItems)[i];
            AACube modelCube = model.getAACube();
            modelCube.scale(TREE_SCALE);
            if (params.viewFrustum->cubeInFrustum(modelCube) != ViewFrustum::OUTSIDE) {
                indexesOfModelsToInclude << i;
                numberOfModels++;
            }
        } else {
            indexesOfModelsToInclude << i;
            numberOfModels++;
        }
    }

    int numberOfModelsOffset = packetData->getUncompressedByteOffset();
    success = packetData->appendValue(numberOfModels);

    if (success) {
        foreach (uint16_t i, indexesOfModelsToInclude) {
            const ModelItem& model = (*_modelItems)[i];
            
            LevelDetails modelLevel = packetData->startLevel();
    
            success = model.appendModelData(packetData);

            if (success) {
                packetData->endLevel(modelLevel);
                actualNumberOfModels++;
            }
            if (!success) {
                packetData->discardLevel(modelLevel);
                break;
            }
        }
    }
    
    if (!success) {
        success = packetData->updatePriorBytes(numberOfModelsOffset, 
                                            (const unsigned char*)&actualNumberOfModels, sizeof(actualNumberOfModels));
    }
    
    return success;
}

bool ModelTreeElement::containsModelBounds(const ModelItem& model) const {
    glm::vec3 clampedMin = glm::clamp(model.getMinimumPoint(), 0.0f, 1.0f);
    glm::vec3 clampedMax = glm::clamp(model.getMaximumPoint(), 0.0f, 1.0f);
    return _cube.contains(clampedMin) && _cube.contains(clampedMax);
}

bool ModelTreeElement::bestFitModelBounds(const ModelItem& model) const {
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

void ModelTreeElement::update(ModelTreeUpdateArgs& args) {
    args._totalElements++;
    // update our contained models
    QList<ModelItem>::iterator modelItr = _modelItems->begin();
    while(modelItr != _modelItems->end()) {
        ModelItem& model = (*modelItr);
        args._totalItems++;
        
        // TODO: this _lastChanged isn't actually changing because we're not marking this element as changed.
        // how do we want to handle this??? We really only want to consider an element changed when it is
        // edited... not just animated...
        model.update(_lastChanged);

        // If the model wants to die, or if it's left our bounding box, then move it
        // into the arguments moving models. These will be added back or deleted completely
        if (model.getShouldDie() || !bestFitModelBounds(model)) {
            args._movingModels.push_back(model);

            // erase this model
            modelItr = _modelItems->erase(modelItr);

            args._movingItems++;
            
            // this element has changed so mark it...
            markWithChangedTime();
        } else {
            ++modelItr;
        }
    }
}

bool ModelTreeElement::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject) {

    // only called if we do intersect our bounding cube, but find if we actually intersect with models...
    
    QList<ModelItem>::iterator modelItr = _modelItems->begin();
    QList<ModelItem>::const_iterator modelEnd = _modelItems->end();
    bool somethingIntersected = false;
    while(modelItr != modelEnd) {
        ModelItem& model = (*modelItr);
        
        AACube modelCube = model.getAACube();
        float localDistance;
        BoxFace localFace;

        // if the ray doesn't intersect with our cube, we can stop searching!
        if (modelCube.findRayIntersection(origin, direction, localDistance, localFace)) {
            const FBXGeometry* fbxGeometry = _myTree->getGeometryForModel(model);
            if (fbxGeometry && fbxGeometry->meshExtents.isValid()) {
                Extents extents = fbxGeometry->meshExtents;

                // NOTE: If the model has a bad mesh, then extents will be 0,0,0 & 0,0,0
                if (extents.minimum == extents.maximum && extents.minimum == glm::vec3(0,0,0)) {
                    extents.maximum = glm::vec3(1.0f,1.0f,1.0f); // in this case we will simulate the unit cube
                }

                // NOTE: these extents are model space, so we need to scale and center them accordingly
                // size is our "target size in world space"
                // we need to set our model scale so that the extents of the mesh, fit in a cube that size...
                float maxDimension = glm::distance(extents.maximum, extents.minimum);
                float scale = model.getSize() / maxDimension;

                glm::vec3 halfDimensions = (extents.maximum - extents.minimum) * 0.5f;
                glm::vec3 offset = -extents.minimum - halfDimensions;
                
                extents.minimum += offset;
                extents.maximum += offset;

                extents.minimum *= scale;
                extents.maximum *= scale;
                
                Extents rotatedExtents = extents;

                calculateRotatedExtents(rotatedExtents, model.getModelRotation());

                rotatedExtents.minimum += model.getPosition();
                rotatedExtents.maximum += model.getPosition();


                AABox rotatedExtentsBox(rotatedExtents.minimum, (rotatedExtents.maximum - rotatedExtents.minimum));
                
                // if it's in our AABOX for our rotated extents, then check to see if it's in our non-AABox
                if (rotatedExtentsBox.findRayIntersection(origin, direction, localDistance, localFace)) {
                
                    // extents is the model relative, scaled, centered extents of the model
                    glm::mat4 rotation = glm::mat4_cast(model.getModelRotation());
                    glm::mat4 translation = glm::translate(model.getPosition());
                    glm::mat4 modelToWorldMatrix = translation * rotation;
                    glm::mat4 worldToModelMatrix = glm::inverse(modelToWorldMatrix);

                    AABox modelFrameBox(extents.minimum, (extents.maximum - extents.minimum));

                    glm::vec3 modelFrameOrigin = glm::vec3(worldToModelMatrix * glm::vec4(origin, 1.0f));
                    glm::vec3 modelFrameDirection = glm::vec3(worldToModelMatrix * glm::vec4(direction, 0.0f));

                    // we can use the AABox's ray intersection by mapping our origin and direction into the model frame
                    // and testing intersection there.
                    if (modelFrameBox.findRayIntersection(modelFrameOrigin, modelFrameDirection, localDistance, localFace)) {
                        if (localDistance < distance) {
                            distance = localDistance;
                            face = localFace;
                            *intersectedObject = (void*)(&model);
                            somethingIntersected = true;
                        }
                    }
                }
            } else if (localDistance < distance) {
                distance = localDistance;
                face = localFace;
                *intersectedObject = (void*)(&model);
                somethingIntersected = true;
            }
        }
        
        ++modelItr;
    }
    return somethingIntersected;
}

bool ModelTreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                                    glm::vec3& penetration, void** penetratedObject) const {
    QList<ModelItem>::iterator modelItr = _modelItems->begin();
    QList<ModelItem>::const_iterator modelEnd = _modelItems->end();
    while(modelItr != modelEnd) {
        ModelItem& model = (*modelItr);
        glm::vec3 modelCenter = model.getPosition();
        float modelRadius = model.getRadius();

        // don't penetrate yourself
        if (modelCenter == center && modelRadius == radius) {
            return false;
        }

        if (findSphereSpherePenetration(center, radius, modelCenter, modelRadius, penetration)) {
            // return true on first valid model penetration
            *penetratedObject = (void*)(&model);
            return true;
        }
        ++modelItr;
    }
    return false;
}

bool ModelTreeElement::updateModel(const ModelItem& model) {
    // NOTE: this method must first lookup the model by ID, hence it is O(N)
    // and "model is not found" is worst-case (full N) but maybe we don't care?
    // (guaranteed that num models per elemen is small?)
    const bool wantDebug = false;
    uint16_t numberOfModels = _modelItems->size();
    for (uint16_t i = 0; i < numberOfModels; i++) {
        ModelItem& thisModel = (*_modelItems)[i];
        if (thisModel.getID() == model.getID()) {
            int difference = thisModel.getLastUpdated() - model.getLastUpdated();
            bool changedOnServer = thisModel.getLastEdited() < model.getLastEdited();
            bool localOlder = thisModel.getLastUpdated() < model.getLastUpdated();
            if (changedOnServer || localOlder) {
                if (wantDebug) {
                    qDebug("local model [id:%d] %s and %s than server model by %d, model.isNewlyCreated()=%s",
                            model.getID(), (changedOnServer ? "CHANGED" : "same"),
                            (localOlder ? "OLDER" : "NEWER"),
                            difference, debug::valueOf(model.isNewlyCreated()) );
                }
                
                thisModel.copyChangedProperties(model);
                markWithChangedTime();
            } else {
                if (wantDebug) {
                    qDebug(">>> IGNORING SERVER!!! Would've caused jutter! <<<  "
                            "local model [id:%d] %s and %s than server model by %d, model.isNewlyCreated()=%s",
                            model.getID(), (changedOnServer ? "CHANGED" : "same"),
                            (localOlder ? "OLDER" : "NEWER"),
                            difference, debug::valueOf(model.isNewlyCreated()) );
                }
            }
            return true;
        }
    }
    return false;
}

bool ModelTreeElement::updateModel(const ModelItemID& modelID, const ModelItemProperties& properties) {
    uint16_t numberOfModels = _modelItems->size();
    for (uint16_t i = 0; i < numberOfModels; i++) {
        // note: unlike storeModel() which is called from inbound packets, this is only called by local editors
        // and therefore we can be confident that this change is higher priority and should be honored
        ModelItem& thisModel = (*_modelItems)[i];
        
        bool found = false;
        if (modelID.isKnownID) {
            found = thisModel.getID() == modelID.id;
        } else {
            found = thisModel.getCreatorTokenID() == modelID.creatorTokenID;
        }
        if (found) {
            thisModel.setProperties(properties);
            if (_myTree->getGeometryForModel(thisModel)) {
                thisModel.setSittingPoints(_myTree->getGeometryForModel(thisModel)->sittingPoints);
            }
            markWithChangedTime(); // mark our element as changed..
            const bool wantDebug = false;
            if (wantDebug) {
                uint64_t now = usecTimestampNow();
                int elapsed = now - thisModel.getLastEdited();

                qDebug() << "ModelTreeElement::updateModel() AFTER update... edited AGO=" << elapsed <<
                        "now=" << now << " thisModel.getLastEdited()=" << thisModel.getLastEdited();
            }                
            return true;
        }
    }
    return false;
}

void ModelTreeElement::updateModelItemID(FindAndUpdateModelItemIDArgs* args) {
    uint16_t numberOfModels = _modelItems->size();
    for (uint16_t i = 0; i < numberOfModels; i++) {
        ModelItem& thisModel = (*_modelItems)[i];
        
        if (!args->creatorTokenFound) {
            // first, we're looking for matching creatorTokenIDs, if we find that, then we fix it to know the actual ID
            if (thisModel.getCreatorTokenID() == args->creatorTokenID) {
                thisModel.setID(args->modelID);
                args->creatorTokenFound = true;
            }
        }
        
        // if we're in an isViewing tree, we also need to look for an kill any viewed models
        if (!args->viewedModelFound && args->isViewing) {
            if (thisModel.getCreatorTokenID() == UNKNOWN_MODEL_TOKEN && thisModel.getID() == args->modelID) {
                _modelItems->removeAt(i); // remove the model at this index
                numberOfModels--; // this means we have 1 fewer model in this list
                i--; // and we actually want to back up i as well.
                args->viewedModelFound = true;
            }
        }
    }
}



const ModelItem* ModelTreeElement::getClosestModel(glm::vec3 position) const {
    const ModelItem* closestModel = NULL;
    float closestModelDistance = FLT_MAX;
    uint16_t numberOfModels = _modelItems->size();
    for (uint16_t i = 0; i < numberOfModels; i++) {
        float distanceToModel = glm::distance(position, (*_modelItems)[i].getPosition());
        if (distanceToModel < closestModelDistance) {
            closestModel = &(*_modelItems)[i];
        }
    }
    return closestModel;
}

void ModelTreeElement::getModels(const glm::vec3& searchPosition, float searchRadius, QVector<const ModelItem*>& foundModels) const {
    uint16_t numberOfModels = _modelItems->size();
    for (uint16_t i = 0; i < numberOfModels; i++) {
        const ModelItem* model = &(*_modelItems)[i];
        float distance = glm::length(model->getPosition() - searchPosition);
        if (distance < searchRadius + model->getRadius()) {
            foundModels.push_back(model);
        }
    }
}

void ModelTreeElement::getModelsInside(const AACube& box, QVector<ModelItem*>& foundModels) {
    QList<ModelItem>::iterator modelItr = _modelItems->begin();
    QList<ModelItem>::iterator modelEnd = _modelItems->end();
    AACube modelCube;
    while(modelItr != modelEnd) {
        ModelItem* model = &(*modelItr);
        if (box.contains(model->getPosition())) {
            foundModels.push_back(model);
        }
        ++modelItr;
    }
}

void ModelTreeElement::getModelsForUpdate(const AACube& box, QVector<ModelItem*>& foundModels) {
    QList<ModelItem>::iterator modelItr = _modelItems->begin();
    QList<ModelItem>::iterator modelEnd = _modelItems->end();
    AACube modelCube;
    while(modelItr != modelEnd) {
        ModelItem* model = &(*modelItr);
        float radius = model->getRadius();
        // NOTE: we actually do cube-cube collision queries here, which is sloppy but good enough for now
        // TODO: decide whether to replace modelCube-cube query with sphere-cube (requires a square root
        // but will be slightly more accurate).
        modelCube.setBox(model->getPosition() - glm::vec3(radius), 2.f * radius);
        if (modelCube.touches(_cube)) {
            foundModels.push_back(model);
        }
        ++modelItr;
    }
}

const ModelItem* ModelTreeElement::getModelWithID(uint32_t id) const {
    // NOTE: this lookup is O(N) but maybe we don't care? (guaranteed that num models per elemen is small?)
    const ModelItem* foundModel = NULL;
    uint16_t numberOfModels = _modelItems->size();
    for (uint16_t i = 0; i < numberOfModels; i++) {
        if ((*_modelItems)[i].getID() == id) {
            foundModel = &(*_modelItems)[i];
            break;
        }
    }
    return foundModel;
}

bool ModelTreeElement::removeModelWithID(uint32_t id) {
    bool foundModel = false;
    uint16_t numberOfModels = _modelItems->size();
    for (uint16_t i = 0; i < numberOfModels; i++) {
        if ((*_modelItems)[i].getID() == id) {
            foundModel = true;
            _modelItems->removeAt(i);
            break;
        }
    }
    return foundModel;
}

int ModelTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
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
    uint16_t numberOfModels = 0;
    int expectedBytesPerModel = ModelItem::expectedBytes();

    if (bytesLeftToRead >= (int)sizeof(numberOfModels)) {
        // read our models in....
        numberOfModels = *(uint16_t*)dataAt;

        dataAt += sizeof(numberOfModels);
        bytesLeftToRead -= (int)sizeof(numberOfModels);
        bytesRead += sizeof(numberOfModels);
        
        if (bytesLeftToRead >= (int)(numberOfModels * expectedBytesPerModel)) {
            for (uint16_t i = 0; i < numberOfModels; i++) {
                ModelItem tempModel;
                int bytesForThisModel = tempModel.readModelDataFromBuffer(dataAt, bytesLeftToRead, args);
                _myTree->storeModel(tempModel);
                dataAt += bytesForThisModel;
                bytesLeftToRead -= bytesForThisModel;
                bytesRead += bytesForThisModel;
            }
        }
    }

    return bytesRead;
}

// will average a "common reduced LOD view" from the the child elements...
void ModelTreeElement::calculateAverageFromChildren() {
    // nothing to do here yet...
}

// will detect if children are leaves AND collapsable into the parent node
// and in that case will collapse children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a
// single node
bool ModelTreeElement::collapseChildren() {
    // nothing to do here yet...
    return false;
}


void ModelTreeElement::storeModel(const ModelItem& model) {
    _modelItems->push_back(model);
    markWithChangedTime();
}


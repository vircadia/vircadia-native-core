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

    const bool wantDebug = false;
    
    // Check to see if this element yet has encode data... if it doesn't create it
    if (!extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData = new EntityTreeElementExtraEncodeData();

        entityTreeElementExtraEncodeData->elementCompleted = (_entityItems->size() == 0);

        if (wantDebug) {
            qDebug() << "EntityTreeElement::initializeExtraEncodeData()... ";
            qDebug() << "    element:" << getAACube();
            qDebug() << "    elementCompleted:" << entityTreeElementExtraEncodeData->elementCompleted << "[ _entityItems->size()=" << _entityItems->size() <<" ]";
            qDebug() << "    --- initialize this element's child element state ---";
        }
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            EntityTreeElement* child = getChildAtIndex(i);
            if (!child) {
                entityTreeElementExtraEncodeData->childCompleted[i] = true; // if no child exists, it is completed
                if (wantDebug) {
                    qDebug() << "    childCompleted[" << i <<"]= true -- completed";
                }
            } else {
                if (child->hasEntities()) {
                    entityTreeElementExtraEncodeData->childCompleted[i] = false; // HAS ENTITIES NEEDS ENCODING
                    if (wantDebug) {
                        qDebug() << "    childCompleted[" << i <<"] " << child->getAACube()  << "= false -- HAS ENTITIES NEEDS ENCODING";
                    }
                } else {
                    entityTreeElementExtraEncodeData->childCompleted[i] = true; // if the child doesn't have enities, it is completed
                    if (wantDebug) {
                        qDebug() << "    childCompleted[" << i <<"] " << child->getAACube()  << "= true -- doesn't have entities";
                    }
                }
            }
        }
        if (wantDebug) {
            qDebug() << "    --- initialize this element's entities state ---";
        }
        for (uint16_t i = 0; i < _entityItems->size(); i++) {
            EntityItem* entity = (*_entityItems)[i];
            entityTreeElementExtraEncodeData->entities.insert(entity->getEntityItemID(), entity->getEntityProperties(params));
        }
        
        // TODO: some of these inserts might be redundant!!!
        //qDebug() << " ADDING encode data (" << __LINE__ << ") for element " << getAACube() << " data=" << entityTreeElementExtraEncodeData;
        extraEncodeData->insert(this, entityTreeElementExtraEncodeData);
    }
}

bool EntityTreeElement::shouldIncludeChild(int childIndex, EncodeBitstreamParams& params) const { 
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

void EntityTreeElement::updateEncodedData(int childIndex, AppendState childAppendState, EncodeBitstreamParams& params) const {
    OctreeElementExtraEncodeData* extraEncodeData = params.extraEncodeData;
    assert(extraEncodeData); // EntityTrees always require extra encode data on their encoding passes

    const bool wantDebug = false;
    
    if (wantDebug) {
        qDebug() << "EntityTreeElement::updateEncodedData()... ";
        qDebug() << "    element:" << getAACube();
        qDebug() << "    child:" << childIndex << getChildAtIndex(childIndex)->getAACube();
        switch(childAppendState) {
            case OctreeElement::NONE:
                qDebug() << "    childAppendState: NONE";
            break;
            case OctreeElement::PARTIAL:
                qDebug() << "    childAppendState: PARTIAL";
            break;
            case OctreeElement::COMPLETED:
                qDebug() << "    childAppendState: COMPLETED";
            break;
        }
    }

    if (extraEncodeData->contains(this)) {
        EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData 
                        = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(this));

                        
        if (childAppendState == OctreeElement::COMPLETED) {
            entityTreeElementExtraEncodeData->childCompleted[childIndex] = true;
            if (wantDebug) {
                qDebug() << "    SETTING childCompleted[" << childIndex << "] = true - " 
                                        << getChildAtIndex(childIndex)->getAACube();
            }
        }
        
        if (wantDebug) {
            qDebug() << "    encode data:" << entityTreeElementExtraEncodeData;
        }
    } else {
        assert(false); // this shouldn't happen!
    }
}




void EntityTreeElement::elementEncodeComplete(EncodeBitstreamParams& params, OctreeElementBag* bag) const {
    const bool wantDebug = false;
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

    if (wantDebug) {
        qDebug() << "------------------------------------------------------------------------------------";
        qDebug() << "EntityTreeElement::elementEncodeComplete()...";
        qDebug() << "    element=" << getAACube();
        qDebug() << "    encode data (this):" << thisExtraEncodeData;
        if (!thisExtraEncodeData->elementCompleted) {
            qDebug() << "        ******* PROBABLY OK ---- WARNING ********* this element was not complete!!";
            qDebug() << "            we really only care that our data is complete when we attempt to check our parent...";
        }
    }

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        EntityTreeElement* childElement = getChildAtIndex(i);
        if (childElement) {
            bool isThisChildReallyComplete = thisExtraEncodeData->childCompleted[i];

            // why would this ever fail???
            // If we've encoding this element before... but we're coming back a second time in an attempt to
            // encoud our parent... this might happen.
            if (extraEncodeData->contains(childElement)) {
                EntityTreeElementExtraEncodeData* childExtraEncodeData 
                                = static_cast<EntityTreeElementExtraEncodeData*>(extraEncodeData->value(childElement));
                            
                if (wantDebug) {
                    // If the child element is not complete, then it should be in the bag for re-encoding
                    if (!thisExtraEncodeData->childCompleted[i]) {
                        if (bag->contains(childElement)) {
                            qDebug() << "        GOOD this element's child["<< i << "] " 
                                                << childElement->getAACube() << " was not complete, but it's in the bag!!";
                        } else {
                            qDebug() << "        ******* WARNING ********* this element's child["<< i << "] " 
                                                << childElement->getAACube() << " was not complete, AND IT'S NOT IN THE BAG!!";
                        }
                    }
            
                    qDebug() << "    child[" << i <<"] has extra data";
                    qDebug() << "        child:" << childElement->getAACube();
                    qDebug() << "        encode data (child):" << childExtraEncodeData;
                    // If we're completing THIS element then ALL of our child elements must have been able to add their element data
                    if (childExtraEncodeData->elementCompleted) {
                        qDebug() << "        GOOD this element's child["<< i << "] " 
                                                << childElement->getAACube() << " element data was complete!!";
                    } else {
                        qDebug() << "        ******* WARNING ********* this element's child["<< i << "] " 
                                                << childElement->getAACube() << " element data was NOT COMPLETE!!";
                    }

                }
            
                for (int ii = 0; ii < NUMBER_OF_CHILDREN; ii++) {
                    if (!childExtraEncodeData->childCompleted[ii]) {
                        if (wantDebug) {
                            OctreeElement* grandChild = childElement->getChildAtIndex(ii);
                            if (bag->contains(childElement)) {
                                qDebug() << "        GOOD this element's child["<< i << "]'s child["<< ii << "] " << grandChild->getAACube() 
                                                        << " was not complete, but the child " << childElement->getAACube() 
                                                        << " is in the bag!!";
                            } else {
                                qDebug() << "        ******* WARNING ********* this element's child["<< i 
                                                        << "]'s child["<< ii << "] " << grandChild->getAACube() 
                                                        << " was not complete, AND THE CHILD " << childElement->getAACube() 
                                                        << " IS NOT IN THE BAG!!";
                            }
                        }
                    
                        isThisChildReallyComplete = false;
                    }
                }

                if (isThisChildReallyComplete) {
                    if (wantDebug) {
                        qDebug() << "        REMOVE CHILD EXTRA DATA....";
                        qDebug() << "        DELETING -- CHILD EXTRA DATA....";
                        qDebug() << "        REMOVING encode data (" << __LINE__ << ") for element " 
                                                        << childElement->getAACube() << " data=" << childExtraEncodeData;
                    }
                    extraEncodeData->remove(childElement);
                    delete childExtraEncodeData;
                }
            } else {
                if (wantDebug) {
                    qDebug() << "******* WARNING ********* this element's child["<< i << "] " 
                            << childElement->getAACube() << " didn't have extra encode data ------ UNEXPECTED!!!!!";
                }
            }
        }
    }
}

OctreeElement::AppendState EntityTreeElement::appendElementData(OctreePacketData* packetData, 
                                                                    EncodeBitstreamParams& params) const {
                     
    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "EntityTreeElement::appendElementData()";
        qDebug() << "    getAACube()=" << getAACube();
        qDebug() << "    START OF ELEMENT packetData->uncompressed size:" << packetData->getUncompressedSize();
        qDebug() << "    params.lastViewFrustumSent=" << params.lastViewFrustumSent;
    }

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

        if (wantDebug) {
            qDebug() << "EntityTreeElement::appendElementData()... ENCODE DATA MISSING, SETTING IT UP NOW ";
            qDebug() << "    element:" << getAACube();
            qDebug() << "    elementCompleted:" << entityTreeElementExtraEncodeData->elementCompleted 
                                        << "[ _entityItems->size()=" << _entityItems->size() <<" ]";
            qDebug() << "    --- initialize child elements state ---";
        }
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            EntityTreeElement* child = getChildAtIndex(i);
            if (!child) {
                entityTreeElementExtraEncodeData->childCompleted[i] = true; // if no child exists, it is completed
                if (wantDebug) {
                    qDebug() << "    childCompleted[" << i <<"]= true -- completed";
                }
            } else {
                if (child->hasEntities()) {
                    entityTreeElementExtraEncodeData->childCompleted[i] = false;
                    if (wantDebug) {
                        qDebug() << "    childCompleted[" << i <<"] " << child->getAACube() 
                                                << "= false -- HAS ENTITIES NEEDS ENCODING";
                    }
                } else {
                    entityTreeElementExtraEncodeData->childCompleted[i] = true; // if the child doesn't have enities, it is completed
                    if (wantDebug) {
                        qDebug() << "    childCompleted[" << i <<"] " << child->getAACube()  << "= true -- doesn't have entities";
                    }
                }
            }
        }
        if (wantDebug) {
            qDebug() << "    --- initialize this element's entities state ---";
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
    if (wantDebug) {
        qDebug() << "------------- elementLevel = packetData->startLevel() -------------";
    }

    // write our entities out... first determine which of the entities are in view based on our params
    uint16_t numberOfEntities = 0;
    uint16_t actualNumberOfEntities = 0;
    QVector<uint16_t> indexesOfEntitiesToInclude;

    if (wantDebug) {
        qDebug() << "EntityTreeElement::appendElementData() _entityItems->size()=" << _entityItems->size();
    }

    // It's possible that our element has been previous completed. In this case we'll simply not include any of our
    // entities for encoding. This is needed because we encode the element data at the "parent" level, and so we 
    // need to handle the case where our sibling elements need encoding but we don't.
    if (!entityTreeElementExtraEncodeData->elementCompleted) {
        for (uint16_t i = 0; i < _entityItems->size(); i++) {
            EntityItem* entity = (*_entityItems)[i];
            bool includeThisEntity = true;
        
            if (false && wantDebug) {
                qDebug() << "params.forceSendScene=" << params.forceSendScene;
                qDebug() << "entity->getLastEdited()=" << entity->getLastEdited();
                qDebug() << "entity->getLastEdited() > params.lastViewFrustumSent=" 
                            << (entity->getLastEdited() > params.lastViewFrustumSent);
            }
        
            if (!params.forceSendScene && entity->getLastEdited() < params.lastViewFrustumSent) {
                if (false && wantDebug) {
                    qDebug() << "NOT forceSendScene, and not changed since last sent SUPPRESSING this ENTITY" 
                                    << entity->getEntityItemID();
                }
                includeThisEntity = false;
            }
        
            if (hadElementExtraData) {
                includeThisEntity = includeThisEntity && 
                                        entityTreeElementExtraEncodeData->entities.contains(entity->getEntityItemID());
                if (wantDebug) {
                    if (includeThisEntity) {
                        qDebug() << "    entity[" << i <<"].entityItemID=" << entity->getEntityItemID();
                        qDebug() << "    entity[" << i <<"].includeThisEntity=" << includeThisEntity;
                    }
                }
            }
        
            if (includeThisEntity && params.viewFrustum) {
                AACube entityCube = entity->getAACube();
                entityCube.scale(TREE_SCALE);
                if (params.viewFrustum->cubeInFrustum(entityCube) == ViewFrustum::OUTSIDE) {
                    includeThisEntity = false; // out of view, don't include it
                    if (wantDebug) {
                        qDebug() << "    entity[" << i <<"] cubeInFrustum(entityCube) == ViewFrustum::OUTSIDE "
                                                                "includeThisEntity=" << includeThisEntity;
                    }
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

    if (wantDebug) {
        qDebug() << "    numberOfEntities=" << numberOfEntities;
        qDebug() << "    successAppendEntityCount=" << successAppendEntityCount;
        qDebug() << "--- before child loop ---";
        qDebug() << "    packetData->getUncompressedSize()=" << packetData->getUncompressedSize() << "line:" << __LINE__;
        qDebug() << "    packetData->getReservedBytes()=" << packetData->getReservedBytes();
    }
    
    if (successAppendEntityCount) {
    
        if (wantDebug) {                                               
            qDebug() << "EntityTreeElement::appendElementData()";
            qDebug() << "    indexesOfEntitiesToInclude loop.... numberOfEntities=" << numberOfEntities;
        }
    
        foreach (uint16_t i, indexesOfEntitiesToInclude) {
            EntityItem* entity = (*_entityItems)[i];

            if (wantDebug) {                                               
                qDebug() << "    indexesOfEntitiesToInclude.... entity[" << i <<"].entityItemID=" << entity->getEntityItemID();
            }
            
            LevelDetails entityLevel = packetData->startLevel();

            if (wantDebug) {
                qDebug() << "--- BEFORE entity ---";
                qDebug() << "    packetData->getUncompressedSize=" << packetData->getUncompressedSize() << "line:" << __LINE__;
                qDebug() << "    packetData->getReservedBytes=" << packetData->getReservedBytes();
            }
            
            OctreeElement::AppendState appendEntityState = entity->appendEntityData(packetData, 
                                                                        params, entityTreeElementExtraEncodeData);

            if (wantDebug) {
                qDebug() << "--- AFTER entity ---";
                qDebug() << "    packetData->getUncompressedSize=" << packetData->getUncompressedSize() << "line:" << __LINE__;
                qDebug() << "    packetData->getReservedBytes=" << packetData->getReservedBytes();

                switch(appendEntityState) {
                    case OctreeElement::NONE:
                        qDebug() << "    indexesOfEntitiesToInclude.... entity[" << i <<"] DIDN'T FIT!!!";
                    break;
                    case OctreeElement::PARTIAL:
                        qDebug() << "    indexesOfEntitiesToInclude.... entity[" << i <<"] PARTIAL FIT!!!";
                    break;
                    case OctreeElement::COMPLETED:
                        qDebug() << "    indexesOfEntitiesToInclude.... entity[" << i <<"] IT ALL FIT!!!";
                    break;
                }
            }

            // If none of this entity data was able to be appended, then discard it
            // and don't include it in our entity count
            if (appendEntityState == OctreeElement::NONE) {
                if (wantDebug) {
                    qDebug() << "    calling discardLevel(entityLevel)...";
                }
                packetData->discardLevel(entityLevel);
            } else {
                // If either ALL or some of it got appended, then end the level (commit it)
                // and include the entity in our final count of entities
                packetData->endLevel(entityLevel);
                actualNumberOfEntities++;
                if (wantDebug) {
                    qDebug() << "    calling endLevel(entityLevel)...";
                }
            }
            
            // If the entity item got completely appended, then we can remove it from the extra encode data
            if (appendEntityState == OctreeElement::COMPLETED) {
                if (wantDebug) {
                    qDebug() << "    since entity fit, removing it from entities...";
                }
                entityTreeElementExtraEncodeData->entities.remove(entity->getEntityItemID());
            }

            // If any part of the entity items didn't fit, then the element is considered partial
            // NOTE: if the entity item didn't fit or only partially fit, then the entity item should have
            // added itself to the extra encode data.
            if (appendEntityState != OctreeElement::COMPLETED) {
                if (wantDebug) {
                    qDebug() << "    entity not complete, element must be partial!!!!";
                }
                appendElementState = OctreeElement::PARTIAL;
            }
        }
    } else {
        // we we couldn't add the entity count, then we couldn't add anything for this element and we're in a NONE state
        appendElementState = OctreeElement::NONE;
    }

    if (wantDebug) {
        qDebug() << "--- done with loop ---";
        qDebug() << "    actualNumberOfEntities=" << actualNumberOfEntities;
        qDebug() << "    numberOfEntities=" << numberOfEntities;
        qDebug() << "    appendElementState=" << appendElementState;
        switch(appendElementState) {
            case OctreeElement::NONE:
                qDebug() << "            OctreeElement::NONE";
            break;
            case OctreeElement::PARTIAL:
                qDebug() << "            OctreeElement::PARTIAL";
            break;
            case OctreeElement::COMPLETED:
                qDebug() << "            OctreeElement::COMPLETED";
            break;
        }
    }
        
    // If we were provided with extraEncodeData, and we allocated and/or got entityTreeElementExtraEncodeData
    // then we need to do some additional processing, namely make sure our extraEncodeData is up to date for
    // this octree element.
    if (extraEncodeData && entityTreeElementExtraEncodeData) {
        if (wantDebug) {
            qDebug() << "    handling extra encode data....";
        }

        // After processing, if we are PARTIAL or COMPLETED then we need to re-include our extra data. 
        // Only our patent can remove our extra data in these cases and only after it knows that all of it's 
        // children have been encoded.
        // If we weren't able to encode ANY data about ourselves, then we go ahead and remove our element data
        // since that will signal that the entire element needs to be encoded on the next attempt
        if (appendElementState == OctreeElement::NONE) {

            if (!entityTreeElementExtraEncodeData->elementCompleted && entityTreeElementExtraEncodeData->entities.size() == 0) {
                /*
                if (wantDebug) {
                    qDebug() << "    REMOVING OUR EXTRA DATA BECAUSE NOTHING FIT AND WE HAD NO PREVIOUS DATA....";
                    qDebug() << "        for element:" << getAACube();
                    qDebug() << "        elementCompleted=" << entityTreeElementExtraEncodeData->elementCompleted;
                    qDebug() << "        entities.size()=" << entityTreeElementExtraEncodeData->entities.size();
                    qDebug() << "        appendElementState=";
                    switch(appendElementState) {
                        case OctreeElement::NONE:
                            qDebug() << "            OctreeElement::NONE";
                        break;
                        case OctreeElement::PARTIAL:
                            qDebug() << "            OctreeElement::PARTIAL";
                        break;
                        case OctreeElement::COMPLETED:
                            qDebug() << "            OctreeElement::COMPLETED";
                        break;
                    }
                }

                qDebug() << " --------- DO WE REALLY WANT TO DO THIS?????????????? --------------------";
                qDebug() << " REMOVING encode data (" << __LINE__ << ") for element " << getAACube() << " data=" << entityTreeElementExtraEncodeData;
                extraEncodeData->remove(this);
                delete entityTreeElementExtraEncodeData;
                */
                
                
            } else {
                // TODO: some of these inserts might be redundant!!!
                //qDebug() << " ADDING encode data (" << __LINE__ << ") for element " << getAACube() << " data=" << entityTreeElementExtraEncodeData;
                extraEncodeData->insert(this, entityTreeElementExtraEncodeData);

                if (wantDebug) {
                    qDebug() << "    RE INSERT OUR EXTRA DATA.... NOTHING FIT... BUT WE PREVIOUSLY STORED SOMETHING...";
                    qDebug() << "        for element:" << getAACube();
                    qDebug() << "        elementCompleted=" << entityTreeElementExtraEncodeData->elementCompleted;
                    qDebug() << "        entities.size()=" << entityTreeElementExtraEncodeData->entities.size();
                    qDebug() << "        appendElementState=";
                    switch(appendElementState) {
                        case OctreeElement::NONE:
                            qDebug() << "            OctreeElement::NONE";
                        break;
                        case OctreeElement::PARTIAL:
                            qDebug() << "            OctreeElement::PARTIAL";
                        break;
                        case OctreeElement::COMPLETED:
                            qDebug() << "            OctreeElement::COMPLETED";
                        break;
                    }
                }
            }
        } else {
        
            // If we weren't previously completed, check to see if we are
            if (!entityTreeElementExtraEncodeData->elementCompleted) {
                // If all of our items have been encoded, then we are complete as an element.
                if (entityTreeElementExtraEncodeData->entities.size() == 0) {
                    if (wantDebug) {
                        qDebug() << "    since our entities.size() is 0 --- we are assuming we're done <<<<<<<<<<<<<<<<<<";
                    }
                    entityTreeElementExtraEncodeData->elementCompleted = true;
                }
            }

            // TODO: some of these inserts might be redundant!!!
            //qDebug() << " ADDING encode data (" << __LINE__ << ") for element " << getAACube() << " data=" << entityTreeElementExtraEncodeData;
            extraEncodeData->insert(this, entityTreeElementExtraEncodeData);
            if (wantDebug) {
                qDebug() << "    RE INSERT OUR EXTRA DATA....";
                qDebug() << "        for element:" << getAACube();
                qDebug() << "        elementCompleted=" << entityTreeElementExtraEncodeData->elementCompleted;
                qDebug() << "        entities.size()=" << entityTreeElementExtraEncodeData->entities.size();
                qDebug() << "        appendElementState=";
                switch(appendElementState) {
                    case OctreeElement::NONE:
                        qDebug() << "            OctreeElement::NONE";
                    break;
                    case OctreeElement::PARTIAL:
                        qDebug() << "            OctreeElement::PARTIAL";
                    break;
                    case OctreeElement::COMPLETED:
                        qDebug() << "            OctreeElement::COMPLETED";
                    break;
                }
            }

        }
    }

    // Determine if no entities at all were able to fit    
    bool noEntitiesFit = (numberOfEntities > 0 && actualNumberOfEntities == 0);
    
    // If we wrote fewer entities than we expected, update the number of entities in our packet
    bool successUpdateEntityCount = true;
    if (!noEntitiesFit && numberOfEntities != actualNumberOfEntities) {
        successUpdateEntityCount = packetData->updatePriorBytes(numberOfEntitiesOffset,
                                            (const unsigned char*)&actualNumberOfEntities, sizeof(actualNumberOfEntities));
        if (wantDebug) {
            qDebug() << "    UPDATE NUMER OF ENTITIES.... actualNumberOfEntities=" << actualNumberOfEntities;
        }
    }

    // If we weren't able to update our entity count, or we couldn't fit any entities, then
    // we should discard our element and return a result of NONE
    if (!successUpdateEntityCount || noEntitiesFit) {
        packetData->discardLevel(elementLevel);
        appendElementState = OctreeElement::NONE;
        if (wantDebug) {
            qDebug() << "    something went wrong...  discardLevel().... appendElementState = OctreeElement::NONE;";
            qDebug() << "        successUpdateEntityCount=" << successUpdateEntityCount;
            qDebug() << "        noEntitiesFit=" << noEntitiesFit;
            if (extraEncodeData) {
                qDebug() << "        do we still have extra data?? " << extraEncodeData->contains(this);
            } else {
                qDebug() << "        what happened to extraEncodeData??";
            }
        }
    } else {
        packetData->endLevel(elementLevel);
        if (wantDebug) {
            qDebug() << "    looks good  endLevel().... appendElementState=" << appendElementState;
            qDebug() << "        successUpdateEntityCount=" << successUpdateEntityCount;
            qDebug() << "        noEntitiesFit=" << noEntitiesFit;
        }
    }

    if (wantDebug) {
        qDebug() << "END OF ELEMENT packetData->uncompressed size:" << packetData->getUncompressedSize();
        qDebug() << "RETURNING appendElementState=";
        switch(appendElementState) {
            case OctreeElement::NONE:
                qDebug() << "    OctreeElement::NONE";
            break;
            case OctreeElement::PARTIAL:
                qDebug() << "    OctreeElement::PARTIAL";
            break;
            case OctreeElement::COMPLETED:
                qDebug() << "    OctreeElement::COMPLETED";
            break;
        }
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
    bool wantDebug = false;

    glm::vec3 clampedMin = glm::clamp(minPoint, 0.0f, 1.0f);
    glm::vec3 clampedMax = glm::clamp(maxPoint, 0.0f, 1.0f);

    if (wantDebug) {
        qDebug() << "        EntityTreeElement::bestFitBounds()";
        qDebug() << "            minPoint=" << minPoint;
        qDebug() << "            maxPoint=" << maxPoint;
        qDebug() << "            clampedMin=" << clampedMin;
        qDebug() << "            clampedMax=" << clampedMax;
        qDebug() << "            _cube=" << _cube;
    }

    if (_cube.contains(clampedMin) && _cube.contains(clampedMax)) {
        int childForMinimumPoint = getMyChildContainingPoint(clampedMin);
        int childForMaximumPoint = getMyChildContainingPoint(clampedMax);

        if (wantDebug) {
            qDebug() << "            _cube.contains BOTH";
            qDebug() << "            childForMinimumPoint=" << childForMinimumPoint;
            qDebug() << "            childForMaximumPoint=" << childForMaximumPoint;
        }
        
        // if this is a really small box, then it's close enough!
        if (_cube.getScale() <= SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE) {

            if (wantDebug) {
                qDebug() << "            (_cube.getScale() <= SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE).... RETURN TRUE";
            }

            return true;
        }
        // If I contain both the minimum and maximum point, but two different children of mine
        // contain those points, then I am the best fit for that entity
        if (childForMinimumPoint != childForMaximumPoint) {
            if (wantDebug) {
                qDebug() << "            (childForMinimumPoint != childForMaximumPoint).... RETURN TRUE";
            }
            return true;
        }
    } else {
        if (wantDebug) {
            qDebug() << "            NOT _cube.contains BOTH";
        }
    }
    if (wantDebug) {
        qDebug() << "            RETURN FALSE....";
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


// TODO: do we need to handle "killing" viewed entities as well???
void EntityTreeElement::updateEntityItemID(const EntityItemID& creatorTokenEntityID, const EntityItemID& knownIDEntityID) {
    bool wantDebug = false;

    if (wantDebug) {
        qDebug() << "EntityTreeElement::updateEntityItemID()... LOOKING FOR entity: " <<
                    "creatorTokenEntityID=" << creatorTokenEntityID <<
                    "knownIDEntityID=" << knownIDEntityID;
    }

    uint16_t numberOfEntities = _entityItems->size();
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem* thisEntity = (*_entityItems)[i];

        EntityItemID thisEntityID = thisEntity->getEntityItemID();
        
        if (thisEntityID == creatorTokenEntityID) {
            if (wantDebug) {
                qDebug() << "EntityTreeElement::updateEntityItemID()... FOUND IT entity: " <<
                            "thisEntityID=" << thisEntityID <<
                            "creatorTokenEntityID=" << creatorTokenEntityID <<
                            "knownIDEntityID=" << knownIDEntityID;
            }
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
                EntityItemID entityItemID = EntityItemID::readEntityItemIDFromBuffer(dataAt, bytesLeftToRead);
                EntityItem* entityItem = _myTree->findEntityByEntityItemID(entityItemID);
                bool newEntity = false;
                
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
                        _myTree->setContainingElement(entityItemID, this);
                        newEntity = true;
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
            bool wantDebug = false;
            if (wantDebug) {
                qDebug() << "EntityTreeElement::pruneChildren()... WANT TO PRUNE!!!! childIndex=" << childIndex;
            }
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
    
//
//  EntityItem.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QObject>

#include <ByteCountCoding.h>
#include <Octree.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()
#include <VoxelsScriptingInterface.h>
#include <VoxelDetail.h>


// This is not ideal, but adding script-engine as a linked library, will cause a circular reference
// I'm open to other potential solutions. Could we change cmake to allow libraries to reference each others
// headers, but not link to each other, this is essentially what this construct is doing, but would be
// better to add includes to the include path, but not link
#include "../../script-engine/src/ScriptEngine.h"

#include "EntityScriptingInterface.h"
#include "EntityItem.h"
#include "EntityTree.h"

QHash<EntityTypes::EntityType_t, QString> EntityTypes::_typeNameHash;

const QString UNKNOWN_EntityType_t_NAME = "Unknown";
const QString& EntityTypes::getEntityTypeName(EntityType_t entityType) {
    QHash<EntityType_t, QString>::iterator matchedTypeName = _typeNameHash.find(entityType);
    return matchedTypeName != _typeNameHash.end() ? matchedTypeName.value() : UNKNOWN_EntityType_t_NAME;
}

bool EntityTypes::registerEntityTypeName(EntityType_t entityType, const QString& name) {
    _typeNameHash.insert(entityType, name);
    return true;
}

bool registered = EntityTypes::registerEntityTypeName(EntityTypes::Base, "Base")
                    && EntityTypes::registerEntityTypeName(EntityTypes::Model, "Model"); // TODO: move this to model subclass

uint32_t EntityItem::_nextID = 0;

// for locally created models
std::map<uint32_t,uint32_t> EntityItem::_tokenIDsToIDs;
uint32_t EntityItem::_nextCreatorTokenID = 0;

uint32_t EntityItem::getIDfromCreatorTokenID(uint32_t creatorTokenID) {
    if (_tokenIDsToIDs.find(creatorTokenID) != _tokenIDsToIDs.end()) {
        return _tokenIDsToIDs[creatorTokenID];
    }
    return UNKNOWN_ENTITY_ID;
}

uint32_t EntityItem::getNextCreatorTokenID() {
    uint32_t creatorTokenID = _nextCreatorTokenID;
    _nextCreatorTokenID++;
    return creatorTokenID;
}

void EntityItem::handleAddEntityResponse(const QByteArray& packet) {
    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data());
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    dataAt += numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t entityItemID;
    memcpy(&entityItemID, dataAt, sizeof(entityItemID));
    dataAt += sizeof(entityItemID);

    // add our token to id mapping
    _tokenIDsToIDs[creatorTokenID] = entityItemID;
}

EntityItem::EntityItem() {
    _type = EntityTypes::Base;
    rgbColor noColor = { 0, 0, 0 };
    init(glm::vec3(0,0,0), 0, noColor, NEW_ENTITY);
}

void EntityItem::initFromEntityItemID(const EntityItemID& entityItemID) {
    _id = entityItemID.id;
    _creatorTokenID = entityItemID.creatorTokenID;

    // init values with defaults before calling setProperties
    uint64_t now = usecTimestampNow();
    _lastEdited = now;
    _lastUpdated = now;

    _position = glm::vec3(0,0,0);
    _radius = 0;
    rgbColor noColor = { 0, 0, 0 };
    memcpy(_color, noColor, sizeof(_color));
    _shouldBeDeleted = false;
    _modelURL = ENTITY_DEFAULT_MODEL_URL;
    _rotation = ENTITY_DEFAULT_ROTATION;
    
    // animation related
    _animationURL = ENTITY_DEFAULT_ANIMATION_URL;
    _animationIsPlaying = false;
    _animationFrameIndex = 0.0f;
    _animationFPS = ENTITY_DEFAULT_ANIMATION_FPS;
    _glowLevel = 0.0f;

    _jointMappingCompleted = false;
    _lastAnimated = now;
}

EntityItem::EntityItem(const EntityItemID& entityItemID) {
    _type = EntityTypes::Base;
    initFromEntityItemID(entityItemID);
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) {
    _type = EntityTypes::Base;
    initFromEntityItemID(entityItemID);
    setProperties(properties, true); // force copy
}

EntityItem::~EntityItem() {
}

void EntityItem::init(glm::vec3 position, float radius, rgbColor color, uint32_t id) {
    if (id == NEW_ENTITY) {
        _id = _nextID;
        _nextID++;
    } else {
        _id = id;
    }
    quint64 now = usecTimestampNow();
    _lastEdited = now;
    _lastUpdated = now;

    _position = position;
    _radius = radius;
    memcpy(_color, color, sizeof(_color));
    _shouldBeDeleted = false;
    _modelURL = ENTITY_DEFAULT_MODEL_URL;
    _rotation = ENTITY_DEFAULT_ROTATION;

    // animation related
    _animationURL = ENTITY_DEFAULT_ANIMATION_URL;
    _animationIsPlaying = false;
    _animationFrameIndex = 0.0f;
    _animationFPS = ENTITY_DEFAULT_ANIMATION_FPS;
    _glowLevel = 0.0f;
    _jointMappingCompleted = false;
    _lastAnimated = now;
}

OctreeElement::AppendState EntityItem::appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                            EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData) const {

    // ALL this fits...
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    
    OctreeElement::AppendState appendState = OctreeElement::COMPLETED; // assume the best

    // encode our ID as a byte count coded byte stream
    ByteCountCoded<quint32> idCoder = getID();
    QByteArray encodedID = idCoder;

    // encode our type as a byte count coded byte stream
    ByteCountCoded<quint32> typeCoder = getType();
    QByteArray encodedType = typeCoder;

    quint64 updateDelta = getLastUpdated() <= getLastEdited() ? 0 : getLastUpdated() - getLastEdited();
    ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
    QByteArray encodedUpdateDelta = updateDeltaCoder;
    EntityPropertyFlags propertyFlags(PROP_LAST_ITEM);
    EntityPropertyFlags requestedProperties;
    
    requestedProperties += PROP_POSITION;
    requestedProperties += PROP_RADIUS;
    requestedProperties += PROP_MODEL_URL;
    requestedProperties += PROP_ROTATION;
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_ANIMATION_URL;
    requestedProperties += PROP_ANIMATION_FPS;
    requestedProperties += PROP_ANIMATION_FRAME_INDEX;
    requestedProperties += PROP_ANIMATION_PLAYING;
    requestedProperties += PROP_SHOULD_BE_DELETED;

    EntityPropertyFlags propertiesDidntFit = requestedProperties;

    // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
    // then our modelTreeElementExtraEncodeData should include data about which properties we need to append.
    if (modelTreeElementExtraEncodeData && modelTreeElementExtraEncodeData->includedItems.contains(getEntityItemID())) {
        requestedProperties = modelTreeElementExtraEncodeData->includedItems.value(getEntityItemID());
    }

    //qDebug() << "requestedProperties=";
    //requestedProperties.debugDumpBits();
    
    LevelDetails modelLevel = packetData->startLevel();

    bool successIDFits = packetData->appendValue(encodedID);
    bool successTypeFits = packetData->appendValue(encodedType);
    bool successLastEditedFits = packetData->appendValue(getLastEdited());
    bool successLastUpdatedFits = packetData->appendValue(encodedUpdateDelta);
    
    int propertyFlagsOffset = packetData->getUncompressedByteOffset();
    QByteArray encodedPropertyFlags = propertyFlags;
    int oldPropertyFlagsLength = encodedPropertyFlags.length();
    bool successPropertyFlagsFits = packetData->appendValue(encodedPropertyFlags);
    int propertyCount = 0;

    bool headerFits = successIDFits && successTypeFits && successLastEditedFits 
                              && successLastUpdatedFits && successPropertyFlagsFits;

    int startOfEntityItemData = packetData->getUncompressedByteOffset();

    if (headerFits) {
        bool successPropertyFits;

        propertyFlags -= PROP_LAST_ITEM; // clear the last item for now, we may or may not set it as the actual item

        // These items would go here once supported....
        //      PROP_PAGED_PROPERTY,
        //      PROP_CUSTOM_PROPERTIES_INCLUDED,
        //      PROP_VISIBLE,

        // PROP_POSITION
        if (requestedProperties.getHasProperty(PROP_POSITION)) {
            //qDebug() << "PROP_POSITION requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendPosition(getPosition());
            if (successPropertyFits) {
                propertyFlags |= PROP_POSITION;
                propertiesDidntFit -= PROP_POSITION;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_POSITION didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_POSITION NOT requested...";
            propertiesDidntFit -= PROP_POSITION;
        }

        // PROP_RADIUS
        if (requestedProperties.getHasProperty(PROP_RADIUS)) {
            //qDebug() << "PROP_RADIUS requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getRadius());
            if (successPropertyFits) {
                propertyFlags |= PROP_RADIUS;
                propertiesDidntFit -= PROP_RADIUS;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_RADIUS didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_RADIUS NOT requested...";
            propertiesDidntFit -= PROP_RADIUS;
        }

        // PROP_MODEL_URL
        if (requestedProperties.getHasProperty(PROP_MODEL_URL)) {
            //qDebug() << "PROP_MODEL_URL requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getModelURL());
            if (successPropertyFits) {
                propertyFlags |= PROP_MODEL_URL;
                propertiesDidntFit -= PROP_MODEL_URL;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_MODEL_URL didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_MODEL_URL NOT requested...";
            propertiesDidntFit -= PROP_MODEL_URL;
        }

        // PROP_ROTATION
        if (requestedProperties.getHasProperty(PROP_ROTATION)) {
            //qDebug() << "PROP_ROTATION requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getRotation());
            if (successPropertyFits) {
                propertyFlags |= PROP_ROTATION;
                propertiesDidntFit -= PROP_ROTATION;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_ROTATION didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_ROTATION NOT requested...";
            propertiesDidntFit -= PROP_ROTATION;
        }

        // PROP_COLOR
        if (requestedProperties.getHasProperty(PROP_COLOR)) {
            //qDebug() << "PROP_COLOR requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendColor(getColor());
            if (successPropertyFits) {
                propertyFlags |= PROP_COLOR;
                propertiesDidntFit -= PROP_COLOR;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_COLOR didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_COLOR NOT requested...";
            propertiesDidntFit -= PROP_COLOR;
        }

        // PROP_SCRIPT
        //     script would go here...
        
        // PROP_ANIMATION_URL
        if (requestedProperties.getHasProperty(PROP_ANIMATION_URL)) {
            //qDebug() << "PROP_ANIMATION_URL requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getAnimationURL());
            if (successPropertyFits) {
                propertyFlags |= PROP_ANIMATION_URL;
                propertiesDidntFit -= PROP_ANIMATION_URL;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_ANIMATION_URL didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_ANIMATION_URL NOT requested...";
            propertiesDidntFit -= PROP_ANIMATION_URL;
        }

        // PROP_ANIMATION_FPS
        if (requestedProperties.getHasProperty(PROP_ANIMATION_FPS)) {
            //qDebug() << "PROP_ANIMATION_FPS requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getAnimationFPS());
            if (successPropertyFits) {
                propertyFlags |= PROP_ANIMATION_FPS;
                propertiesDidntFit -= PROP_ANIMATION_FPS;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_ANIMATION_FPS didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_ANIMATION_FPS NOT requested...";
            propertiesDidntFit -= PROP_ANIMATION_FPS;
        }

        // PROP_ANIMATION_FRAME_INDEX
        if (requestedProperties.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
            //qDebug() << "PROP_ANIMATION_FRAME_INDEX requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getAnimationFrameIndex());
            if (successPropertyFits) {
                propertyFlags |= PROP_ANIMATION_FRAME_INDEX;
                propertiesDidntFit -= PROP_ANIMATION_FRAME_INDEX;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_ANIMATION_FRAME_INDEX didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_ANIMATION_FRAME_INDEX NOT requested...";
            propertiesDidntFit -= PROP_ANIMATION_FRAME_INDEX;
        }

        // PROP_ANIMATION_PLAYING
        if (requestedProperties.getHasProperty(PROP_ANIMATION_PLAYING)) {
            //qDebug() << "PROP_ANIMATION_PLAYING requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getAnimationIsPlaying());
            if (successPropertyFits) {
                propertyFlags |= PROP_ANIMATION_PLAYING;
                propertiesDidntFit -= PROP_ANIMATION_PLAYING;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_ANIMATION_PLAYING didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_ANIMATION_PLAYING NOT requested...";
            propertiesDidntFit -= PROP_ANIMATION_PLAYING;
        }

        // PROP_SHOULD_BE_DELETED
        if (requestedProperties.getHasProperty(PROP_SHOULD_BE_DELETED)) {
            //qDebug() << "PROP_SHOULD_BE_DELETED requested...";
            LevelDetails propertyLevel = packetData->startLevel();
            successPropertyFits = packetData->appendValue(getShouldBeDeleted());
            if (successPropertyFits) {
                propertyFlags |= PROP_SHOULD_BE_DELETED;
                propertiesDidntFit -= PROP_SHOULD_BE_DELETED;
                propertyCount++;
                packetData->endLevel(propertyLevel);
            } else {
                //qDebug() << "PROP_SHOULD_BE_DELETED didn't fit...";
                packetData->discardLevel(propertyLevel);
                appendState = OctreeElement::PARTIAL;
            }
        } else {
            //qDebug() << "PROP_SHOULD_BE_DELETED NOT requested...";
            propertiesDidntFit -= PROP_SHOULD_BE_DELETED;
        }
    }
    if (propertyCount > 0) {
        int endOfEntityItemData = packetData->getUncompressedByteOffset();
        
        encodedPropertyFlags = propertyFlags;
        int newPropertyFlagsLength = encodedPropertyFlags.length();
        packetData->updatePriorBytes(propertyFlagsOffset, 
                (const unsigned char*)encodedPropertyFlags.constData(), encodedPropertyFlags.length());
        
        // if the size of the PropertyFlags shrunk, we need to shift everything down to front of packet.
        if (newPropertyFlagsLength < oldPropertyFlagsLength) {
            int oldSize = packetData->getUncompressedSize();

            const unsigned char* modelItemData = packetData->getUncompressedData(propertyFlagsOffset + oldPropertyFlagsLength);
            int modelItemDataLength = endOfEntityItemData - startOfEntityItemData;
            int newEntityItemDataStart = propertyFlagsOffset + newPropertyFlagsLength;
            packetData->updatePriorBytes(newEntityItemDataStart, modelItemData, modelItemDataLength);

            int newSize = oldSize - (oldPropertyFlagsLength - newPropertyFlagsLength);
            packetData->setUncompressedSize(newSize);

        } else {
            assert(newPropertyFlagsLength == oldPropertyFlagsLength); // should not have grown
        }
       
        packetData->endLevel(modelLevel);
    } else {
        packetData->discardLevel(modelLevel);
        appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
    }
    
    //qDebug() << "propertyFlags=";
    //propertyFlags.debugDumpBits();

    //qDebug() << "propertiesDidntFit=";
    //propertiesDidntFit.debugDumpBits();

    // If any part of the model items didn't fit, then the element is considered partial
    if (appendState != OctreeElement::COMPLETED) {
        // add this item into our list for the next appendElementData() pass
        modelTreeElementExtraEncodeData->includedItems.insert(getEntityItemID(), propertiesDidntFit);
    }

    return appendState;
}

int EntityItem::expectedBytes() {
    int expectedBytes = sizeof(uint32_t) // id
                + sizeof(float) // age
                + sizeof(quint64) // last updated
                + sizeof(quint64) // lasted edited
                + sizeof(float) // radius
                + sizeof(glm::vec3) // position
                + sizeof(rgbColor); // color
                // potentially more...
    return expectedBytes;
}

int EntityItem::oldVersionReadEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {

    int bytesRead = 0;
    if (bytesLeftToRead >= expectedBytes()) {
        int clockSkew = args.sourceNode ? args.sourceNode->getClockSkewUsec() : 0;

        const unsigned char* dataAt = data;

        // id
        memcpy(&_id, dataAt, sizeof(_id));
        dataAt += sizeof(_id);
        bytesRead += sizeof(_id);

        // _lastUpdated
        memcpy(&_lastUpdated, dataAt, sizeof(_lastUpdated));
        dataAt += sizeof(_lastUpdated);
        bytesRead += sizeof(_lastUpdated);
        _lastUpdated -= clockSkew;

        // _lastEdited
        memcpy(&_lastEdited, dataAt, sizeof(_lastEdited));
        dataAt += sizeof(_lastEdited);
        bytesRead += sizeof(_lastEdited);
        _lastEdited -= clockSkew;

        // radius
        memcpy(&_radius, dataAt, sizeof(_radius));
        dataAt += sizeof(_radius);
        bytesRead += sizeof(_radius);

        // position
        memcpy(&_position, dataAt, sizeof(_position));
        dataAt += sizeof(_position);
        bytesRead += sizeof(_position);

        // color
        memcpy(_color, dataAt, sizeof(_color));
        dataAt += sizeof(_color);
        bytesRead += sizeof(_color);

        // shouldBeDeleted
        memcpy(&_shouldBeDeleted, dataAt, sizeof(_shouldBeDeleted));
        dataAt += sizeof(_shouldBeDeleted);
        bytesRead += sizeof(_shouldBeDeleted);

        // modelURL
        uint16_t modelURLLength;
        memcpy(&modelURLLength, dataAt, sizeof(modelURLLength));
        dataAt += sizeof(modelURLLength);
        bytesRead += sizeof(modelURLLength);
        QString modelURLString((const char*)dataAt);
        setModelURL(modelURLString);
        dataAt += modelURLLength;
        bytesRead += modelURLLength;

        // modelRotation
        int bytes = unpackOrientationQuatFromBytes(dataAt, _rotation);
        dataAt += bytes;
        bytesRead += bytes;

        if (args.bitstreamVersion >= VERSION_ENTITIES_HAVE_ANIMATION) {
            // animationURL
            uint16_t animationURLLength;
            memcpy(&animationURLLength, dataAt, sizeof(animationURLLength));
            dataAt += sizeof(animationURLLength);
            bytesRead += sizeof(animationURLLength);
            QString animationURLString((const char*)dataAt);
            setAnimationURL(animationURLString);
            dataAt += animationURLLength;
            bytesRead += animationURLLength;

            // animationIsPlaying
            memcpy(&_animationIsPlaying, dataAt, sizeof(_animationIsPlaying));
            dataAt += sizeof(_animationIsPlaying);
            bytesRead += sizeof(_animationIsPlaying);

            // animationFrameIndex
            memcpy(&_animationFrameIndex, dataAt, sizeof(_animationFrameIndex));
            dataAt += sizeof(_animationFrameIndex);
            bytesRead += sizeof(_animationFrameIndex);

            // animationFPS
            memcpy(&_animationFPS, dataAt, sizeof(_animationFPS));
            dataAt += sizeof(_animationFPS);
            bytesRead += sizeof(_animationFPS);
        }
    }
    return bytesRead;
}

EntityItemID EntityItem::readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                        ReadBitstreamToTreeParams& args) {
    EntityItemID result;
    if (bytesLeftToRead >= sizeof(uint32_t)) {
        // id
        QByteArray encodedID((const char*)data, bytesLeftToRead);
        ByteCountCoded<quint32> idCoder = encodedID;
        quint32 id = idCoder;
        result.id = id;
        result.isKnownID = true;
        result.creatorTokenID = UNKNOWN_ENTITY_TOKEN;
    }
    return result;
}

int EntityItem::readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {

    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
        return oldVersionReadEntityDataFromBuffer(data, bytesLeftToRead, args);
    }

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27; // TODO: this is not correct, we don't yet have 16 byte IDs

    int bytesRead = 0;
    if (bytesLeftToRead >= MINIMUM_HEADER_BYTES) {

        int originalLength = bytesLeftToRead;
        QByteArray originalDataBuffer((const char*)data, originalLength);

        int clockSkew = args.sourceNode ? args.sourceNode->getClockSkewUsec() : 0;

        const unsigned char* dataAt = data;

        // id
        QByteArray encodedID = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> idCoder = encodedID;
        encodedID = idCoder; // determine true length
        dataAt += encodedID.size();
        bytesRead += encodedID.size();
        _id = idCoder;
        _creatorTokenID = UNKNOWN_ENTITY_TOKEN; // if we know the id, then we don't care about the creator token
        _newlyCreated = false;

        // type
        QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> typeCoder = encodedType;
        encodedType = typeCoder; // determine true length
        dataAt += encodedType.size();
        bytesRead += encodedType.size();
        _type = typeCoder;

        // _lastEdited
        memcpy(&_lastEdited, dataAt, sizeof(_lastEdited));
        dataAt += sizeof(_lastEdited);
        bytesRead += sizeof(_lastEdited);
        _lastEdited -= clockSkew;

        // last updated is stored as ByteCountCoded delta from lastEdited
        QByteArray encodedUpdateDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
        quint64 updateDelta = updateDeltaCoder;
        _lastUpdated = _lastEdited + updateDelta; // don't adjust for clock skew since we already did that for _lastEdited
       
        encodedUpdateDelta = updateDeltaCoder; // determine true length
        dataAt += encodedUpdateDelta.size();
        bytesRead += encodedUpdateDelta.size();

        // Property Flags
        QByteArray encodedPropertyFlags = originalDataBuffer.mid(bytesRead); // maximum possible size
        EntityPropertyFlags propertyFlags = encodedPropertyFlags;
        encodedUpdateDelta = updateDeltaCoder; // determine true length
        dataAt += propertyFlags.getEncodedLength();
        bytesRead += propertyFlags.getEncodedLength();

        // PROP_POSITION
        if (propertyFlags.getHasProperty(PROP_POSITION)) {
            memcpy(&_position, dataAt, sizeof(_position));
            dataAt += sizeof(_position);
            bytesRead += sizeof(_position);
        }
        
        // PROP_RADIUS
        if (propertyFlags.getHasProperty(PROP_RADIUS)) {
            memcpy(&_radius, dataAt, sizeof(_radius));
            dataAt += sizeof(_radius);
            bytesRead += sizeof(_radius);
        }

        // PROP_MODEL_URL
        if (propertyFlags.getHasProperty(PROP_MODEL_URL)) {
        
            // TODO: fix to new format...
            uint16_t modelURLLength;
            memcpy(&modelURLLength, dataAt, sizeof(modelURLLength));
            dataAt += sizeof(modelURLLength);
            bytesRead += sizeof(modelURLLength);
            QString modelURLString((const char*)dataAt);
            setModelURL(modelURLString);
            dataAt += modelURLLength;
            bytesRead += modelURLLength;
        }

        // PROP_ROTATION
        if (propertyFlags.getHasProperty(PROP_ROTATION)) {
            int bytes = unpackOrientationQuatFromBytes(dataAt, _rotation);
            dataAt += bytes;
            bytesRead += bytes;
        }
        
        // PROP_COLOR
        if (propertyFlags.getHasProperty(PROP_COLOR)) {
            memcpy(_color, dataAt, sizeof(_color));
            dataAt += sizeof(_color);
            bytesRead += sizeof(_color);
        }
        
        // PROP_SCRIPT
        //     script would go here...
        
        // PROP_ANIMATION_URL
        if (propertyFlags.getHasProperty(PROP_ANIMATION_URL)) {
            // animationURL
            uint16_t animationURLLength;
            memcpy(&animationURLLength, dataAt, sizeof(animationURLLength));
            dataAt += sizeof(animationURLLength);
            bytesRead += sizeof(animationURLLength);
            QString animationURLString((const char*)dataAt);
            setAnimationURL(animationURLString);
            dataAt += animationURLLength;
            bytesRead += animationURLLength;
        }        

        // PROP_ANIMATION_FPS
        if (propertyFlags.getHasProperty(PROP_ANIMATION_FPS)) {
            memcpy(&_animationFPS, dataAt, sizeof(_animationFPS));
            dataAt += sizeof(_animationFPS);
            bytesRead += sizeof(_animationFPS);
        }

        // PROP_ANIMATION_FRAME_INDEX
        if (propertyFlags.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
            memcpy(&_animationFrameIndex, dataAt, sizeof(_animationFrameIndex));
            dataAt += sizeof(_animationFrameIndex);
            bytesRead += sizeof(_animationFrameIndex);
        }

        // PROP_ANIMATION_PLAYING
        if (propertyFlags.getHasProperty(PROP_ANIMATION_PLAYING)) {
            memcpy(&_animationIsPlaying, dataAt, sizeof(_animationIsPlaying));
            dataAt += sizeof(_animationIsPlaying);
            bytesRead += sizeof(_animationIsPlaying);
        }

        // PROP_SHOULD_BE_DELETED
        if (propertyFlags.getHasProperty(PROP_SHOULD_BE_DELETED)) {
            memcpy(&_shouldBeDeleted, dataAt, sizeof(_shouldBeDeleted));
            dataAt += sizeof(_shouldBeDeleted);
            bytesRead += sizeof(_shouldBeDeleted);
        }
    }
    return bytesRead;
}

EntityItem EntityItem::fromEditPacket(const unsigned char* data, int length, int& processedBytes, EntityTree* tree, bool& valid) {
    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "EntityItem EntityItem::fromEditPacket() length=" << length;
    }

    EntityItem newEntityItem; // id and _lastUpdated will get set here...
    const unsigned char* dataAt = data;
    processedBytes = 0;

    // the first part of the data is our octcode...
    int octets = numberOfThreeBitSectionsInCode(data);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);

    if (wantDebug) {
        qDebug() << "EntityItem EntityItem::fromEditPacket() lengthOfOctcode=" << lengthOfOctcode;
    }

    // we don't actually do anything with this octcode...
    dataAt += lengthOfOctcode;
    processedBytes += lengthOfOctcode;

    // id
    uint32_t editID;
    memcpy(&editID, dataAt, sizeof(editID));
    dataAt += sizeof(editID);
    processedBytes += sizeof(editID);

    if (wantDebug) {
        qDebug() << "EntityItem EntityItem::fromEditPacket() editID=" << editID;
    }

    bool isNewEntityItem = (editID == NEW_ENTITY);

    // special case for handling "new" modelItems
    if (isNewEntityItem) {
        // If this is a NEW_ENTITY, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        uint32_t creatorTokenID;
        memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
        dataAt += sizeof(creatorTokenID);
        processedBytes += sizeof(creatorTokenID);

        newEntityItem.setCreatorTokenID(creatorTokenID);
        newEntityItem._newlyCreated = true;
        valid = true;
    } else {
        // look up the existing entityItem
        const EntityItem* existingEntityItem = tree->findEntityByID(editID, true);

        // copy existing properties before over-writing with new properties
        if (existingEntityItem) {
            newEntityItem = *existingEntityItem;
            valid = true;
        } else {
            // the user attempted to edit a entityItem that doesn't exist
            qDebug() << "user attempted to edit a entityItem that doesn't exist... editID=" << editID;
            tree->debugDumpMap();
            valid = false;
            
            // NOTE: Even though we know item is not valid, we still need to parse the rest
            // of the edit packet so that we don't end up out of sync on our bitstream
            // fall through....
        }
        newEntityItem._id = editID;
        newEntityItem._newlyCreated = false;
    }
    
    // lastEdited
    memcpy(&newEntityItem._lastEdited, dataAt, sizeof(newEntityItem._lastEdited));
    dataAt += sizeof(newEntityItem._lastEdited);
    processedBytes += sizeof(newEntityItem._lastEdited);

    // All of the remaining items are optional, and may or may not be included based on their included values in the
    // properties included bits
    uint16_t packetContainsBits = 0;
    if (!isNewEntityItem) {
        memcpy(&packetContainsBits, dataAt, sizeof(packetContainsBits));
        dataAt += sizeof(packetContainsBits);
        processedBytes += sizeof(packetContainsBits);

        // only applies to editing of existing models
        if (!packetContainsBits) {
            //qDebug() << "edit packet didn't contain any information ignore it...";
            valid = false;
            return newEntityItem;
        }
    }

    // radius
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_RADIUS) == ENTITY_PACKET_CONTAINS_RADIUS)) {
        memcpy(&newEntityItem._radius, dataAt, sizeof(newEntityItem._radius));
        dataAt += sizeof(newEntityItem._radius);
        processedBytes += sizeof(newEntityItem._radius);
    }

    // position
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_POSITION) == ENTITY_PACKET_CONTAINS_POSITION)) {
        memcpy(&newEntityItem._position, dataAt, sizeof(newEntityItem._position));
        dataAt += sizeof(newEntityItem._position);
        processedBytes += sizeof(newEntityItem._position);
    }

    // color
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_COLOR) == ENTITY_PACKET_CONTAINS_COLOR)) {
        memcpy(newEntityItem._color, dataAt, sizeof(newEntityItem._color));
        dataAt += sizeof(newEntityItem._color);
        processedBytes += sizeof(newEntityItem._color);
    }

    // shouldBeDeleted
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_SHOULDDIE) == ENTITY_PACKET_CONTAINS_SHOULDDIE)) {
        memcpy(&newEntityItem._shouldBeDeleted, dataAt, sizeof(newEntityItem._shouldBeDeleted));
        dataAt += sizeof(newEntityItem._shouldBeDeleted);
        processedBytes += sizeof(newEntityItem._shouldBeDeleted);
    }

    // modelURL
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_MODEL_URL) == ENTITY_PACKET_CONTAINS_MODEL_URL)) {
        uint16_t modelURLLength;
        memcpy(&modelURLLength, dataAt, sizeof(modelURLLength));
        dataAt += sizeof(modelURLLength);
        processedBytes += sizeof(modelURLLength);
        QString tempString((const char*)dataAt);
        newEntityItem._modelURL = tempString;
        dataAt += modelURLLength;
        processedBytes += modelURLLength;
    }

    // modelRotation
    if (isNewEntityItem || ((packetContainsBits & 
                    ENTITY_PACKET_CONTAINS_ROTATION) == ENTITY_PACKET_CONTAINS_ROTATION)) {
        int bytes = unpackOrientationQuatFromBytes(dataAt, newEntityItem._rotation);
        dataAt += bytes;
        processedBytes += bytes;
    }

    // animationURL
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_ANIMATION_URL) == ENTITY_PACKET_CONTAINS_ANIMATION_URL)) {
        uint16_t animationURLLength;
        memcpy(&animationURLLength, dataAt, sizeof(animationURLLength));
        dataAt += sizeof(animationURLLength);
        processedBytes += sizeof(animationURLLength);
        QString tempString((const char*)dataAt);
        newEntityItem._animationURL = tempString;
        dataAt += animationURLLength;
        processedBytes += animationURLLength;
    }

    // animationIsPlaying
    if (isNewEntityItem || ((packetContainsBits & 
                    ENTITY_PACKET_CONTAINS_ANIMATION_PLAYING) == ENTITY_PACKET_CONTAINS_ANIMATION_PLAYING)) {
                    
        memcpy(&newEntityItem._animationIsPlaying, dataAt, sizeof(newEntityItem._animationIsPlaying));
        dataAt += sizeof(newEntityItem._animationIsPlaying);
        processedBytes += sizeof(newEntityItem._animationIsPlaying);
    }

    // animationFrameIndex
    if (isNewEntityItem || ((packetContainsBits & 
                    ENTITY_PACKET_CONTAINS_ANIMATION_FRAME) == ENTITY_PACKET_CONTAINS_ANIMATION_FRAME)) {
                    
        memcpy(&newEntityItem._animationFrameIndex, dataAt, sizeof(newEntityItem._animationFrameIndex));
        dataAt += sizeof(newEntityItem._animationFrameIndex);
        processedBytes += sizeof(newEntityItem._animationFrameIndex);
    }

    // animationFPS
    if (isNewEntityItem || ((packetContainsBits & 
                    ENTITY_PACKET_CONTAINS_ANIMATION_FPS) == ENTITY_PACKET_CONTAINS_ANIMATION_FPS)) {
                    
        memcpy(&newEntityItem._animationFPS, dataAt, sizeof(newEntityItem._animationFPS));
        dataAt += sizeof(newEntityItem._animationFPS);
        processedBytes += sizeof(newEntityItem._animationFPS);
    }

    const bool wantDebugging = false;
    if (wantDebugging) {
        qDebug("EntityItem::fromEditPacket()...");
        qDebug() << "   EntityItem id in packet:" << editID;
        newEntityItem.debugDump();
    }

    return newEntityItem;
}

void EntityItem::debugDump() const {
    qDebug("EntityItem id  :%u", _id);
    qDebug(" edited ago:%f", getEditedAgo());
    qDebug(" should die:%s", debug::valueOf(getShouldBeDeleted()));
    qDebug(" position:%f,%f,%f", _position.x, _position.y, _position.z);
    qDebug(" radius:%f", getRadius());
    qDebug(" color:%d,%d,%d", _color[0], _color[1], _color[2]);
    qDebug() << " modelURL:" << qPrintable(getModelURL());
}

bool EntityItem::encodeEntityEditMessageDetails(PacketType command, EntityItemID id, const EntityItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {

    bool success = true; // assume the best
    unsigned char* copyAt = bufferOut;
    sizeOut = 0;

    // get the octal code for the entityItem

    // this could be a problem if the caller doesn't include position....
    glm::vec3 rootPosition(0);
    float rootScale = 0.5f;
    unsigned char* octcode = pointToOctalCode(rootPosition.x, rootPosition.y, rootPosition.z, rootScale);

    // TODO: Consider this old code... including the correct octree for where the entityItem will go matters for 
    // entityItem servers with different jurisdictions, but for now, we'll send everything to the root, since the 
    // tree does the right thing...
    //
    //unsigned char* octcode = pointToOctalCode(details[i].position.x, details[i].position.y,
    //                                          details[i].position.z, details[i].radius);

    int octets = numberOfThreeBitSectionsInCode(octcode);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);

    // add it to our message
    memcpy(copyAt, octcode, lengthOfOctcode);
    copyAt += lengthOfOctcode;
    sizeOut += lengthOfOctcode;

    // Now add our edit content details...
    bool isNewEntityItem = (id.id == NEW_ENTITY);

    // id
    memcpy(copyAt, &id.id, sizeof(id.id));
    copyAt += sizeof(id.id);
    sizeOut += sizeof(id.id);

    // special case for handling "new" modelItems
    if (isNewEntityItem) {
        // If this is a NEW_ENTITY, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        memcpy(copyAt, &id.creatorTokenID, sizeof(id.creatorTokenID));
        copyAt += sizeof(id.creatorTokenID);
        sizeOut += sizeof(id.creatorTokenID);
    }
    
    // lastEdited
    quint64 lastEdited = properties.getLastEdited();
    memcpy(copyAt, &lastEdited, sizeof(lastEdited));
    copyAt += sizeof(lastEdited);
    sizeOut += sizeof(lastEdited);
    
    // For new modelItems, all remaining items are mandatory, for an edited entityItem, All of the remaining items are
    // optional, and may or may not be included based on their included values in the properties included bits
    uint16_t packetContainsBits = properties.getChangedBits();
    if (!isNewEntityItem) {
        memcpy(copyAt, &packetContainsBits, sizeof(packetContainsBits));
        copyAt += sizeof(packetContainsBits);
        sizeOut += sizeof(packetContainsBits);
    }

    // radius
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_RADIUS) == ENTITY_PACKET_CONTAINS_RADIUS)) {
        float radius = properties.getRadius() / (float) TREE_SCALE;
        memcpy(copyAt, &radius, sizeof(radius));
        copyAt += sizeof(radius);
        sizeOut += sizeof(radius);
    }

    // position
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_POSITION) == ENTITY_PACKET_CONTAINS_POSITION)) {
        glm::vec3 position = properties.getPosition() / (float)TREE_SCALE;
        memcpy(copyAt, &position, sizeof(position));
        copyAt += sizeof(position);
        sizeOut += sizeof(position);
    }

    // color
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_COLOR) == ENTITY_PACKET_CONTAINS_COLOR)) {
        rgbColor color = { properties.getColor().red, properties.getColor().green, properties.getColor().blue };
        memcpy(copyAt, color, sizeof(color));
        copyAt += sizeof(color);
        sizeOut += sizeof(color);
    }

    // shoulDie
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_SHOULDDIE) == ENTITY_PACKET_CONTAINS_SHOULDDIE)) {
        bool shouldBeDeleted = properties.getShouldBeDeleted();
        memcpy(copyAt, &shouldBeDeleted, sizeof(shouldBeDeleted));
        copyAt += sizeof(shouldBeDeleted);
        sizeOut += sizeof(shouldBeDeleted);
    }

    // modelURL
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_MODEL_URL) == ENTITY_PACKET_CONTAINS_MODEL_URL)) {
        uint16_t urlLength = properties.getModelURL().size() + 1;
        memcpy(copyAt, &urlLength, sizeof(urlLength));
        copyAt += sizeof(urlLength);
        sizeOut += sizeof(urlLength);
        memcpy(copyAt, qPrintable(properties.getModelURL()), urlLength);
        copyAt += urlLength;
        sizeOut += urlLength;
    }

    // modelRotation
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_ROTATION) == ENTITY_PACKET_CONTAINS_ROTATION)) {
        int bytes = packOrientationQuatToBytes(copyAt, properties.getRotation());
        copyAt += bytes;
        sizeOut += bytes;
    }

    // animationURL
    if (isNewEntityItem || ((packetContainsBits & ENTITY_PACKET_CONTAINS_ANIMATION_URL) == ENTITY_PACKET_CONTAINS_ANIMATION_URL)) {
        uint16_t urlLength = properties.getAnimationURL().size() + 1;
        memcpy(copyAt, &urlLength, sizeof(urlLength));
        copyAt += sizeof(urlLength);
        sizeOut += sizeof(urlLength);
        memcpy(copyAt, qPrintable(properties.getAnimationURL()), urlLength);
        copyAt += urlLength;
        sizeOut += urlLength;
    }

    // animationIsPlaying
    if (isNewEntityItem || ((packetContainsBits & 
                    ENTITY_PACKET_CONTAINS_ANIMATION_PLAYING) == ENTITY_PACKET_CONTAINS_ANIMATION_PLAYING)) {
                    
        bool animationIsPlaying = properties.getAnimationIsPlaying();
        memcpy(copyAt, &animationIsPlaying, sizeof(animationIsPlaying));
        copyAt += sizeof(animationIsPlaying);
        sizeOut += sizeof(animationIsPlaying);
    }

    // animationFrameIndex
    if (isNewEntityItem || ((packetContainsBits & 
                    ENTITY_PACKET_CONTAINS_ANIMATION_FRAME) == ENTITY_PACKET_CONTAINS_ANIMATION_FRAME)) {
                    
        float animationFrameIndex = properties.getAnimationFrameIndex();
        memcpy(copyAt, &animationFrameIndex, sizeof(animationFrameIndex));
        copyAt += sizeof(animationFrameIndex);
        sizeOut += sizeof(animationFrameIndex);
    }

    // animationFPS
    if (isNewEntityItem || ((packetContainsBits & 
                    ENTITY_PACKET_CONTAINS_ANIMATION_FPS) == ENTITY_PACKET_CONTAINS_ANIMATION_FPS)) {
                    
        float animationFPS = properties.getAnimationFPS();
        memcpy(copyAt, &animationFPS, sizeof(animationFPS));
        copyAt += sizeof(animationFPS);
        sizeOut += sizeof(animationFPS);
    }

    bool wantDebugging = false;
    if (wantDebugging) {
        qDebug("encodeEntityItemEditMessageDetails()....");
        qDebug("EntityItem id  :%u", id.id);
        qDebug(" nextID:%u", _nextID);
    }

    // cleanup
    delete[] octcode;
    
    return success;
}

// adjust any internal timestamps to fix clock skew for this server
void EntityItem::adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew) {
    unsigned char* dataAt = codeColorBuffer;
    int octets = numberOfThreeBitSectionsInCode(dataAt);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);
    dataAt += lengthOfOctcode;

    // id
    uint32_t id;
    memcpy(&id, dataAt, sizeof(id));
    dataAt += sizeof(id);
    // special case for handling "new" modelItems
    if (id == NEW_ENTITY) {
        // If this is a NEW_ENTITY, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        dataAt += sizeof(uint32_t);
    }

    // lastEdited
    quint64 lastEditedInLocalTime;
    memcpy(&lastEditedInLocalTime, dataAt, sizeof(lastEditedInLocalTime));
    quint64 lastEditedInServerTime = lastEditedInLocalTime + clockSkew;
    memcpy(dataAt, &lastEditedInServerTime, sizeof(lastEditedInServerTime));
    const bool wantDebug = false;
    if (wantDebug) {
        qDebug("EntityItem::adjustEditPacketForClockSkew()...");
        qDebug() << "     lastEditedInLocalTime: " << lastEditedInLocalTime;
        qDebug() << "                 clockSkew: " << clockSkew;
        qDebug() << "    lastEditedInServerTime: " << lastEditedInServerTime;
    }
}


QMap<QString, AnimationPointer> EntityItem::_loadedAnimations; // TODO: improve cleanup by leveraging the AnimationPointer(s)
AnimationCache EntityItem::_animationCache;

// This class/instance will cleanup the animations once unloaded.
class EntityAnimationsBookkeeper {
public:
    ~EntityAnimationsBookkeeper() {
        EntityItem::cleanupLoadedAnimations();
    }
};

EntityAnimationsBookkeeper modelAnimationsBookkeeperInstance;

void EntityItem::cleanupLoadedAnimations() {
    foreach(AnimationPointer animation, _loadedAnimations) {
        animation.clear();
    }
    _loadedAnimations.clear();
}

Animation* EntityItem::getAnimation(const QString& url) {
    AnimationPointer animation;
    
    // if we don't already have this model then create it and initialize it
    if (_loadedAnimations.find(url) == _loadedAnimations.end()) {
        animation = _animationCache.getAnimation(url);
        _loadedAnimations[url] = animation;
    } else {
        animation = _loadedAnimations[url];
    }
    return animation.data();
}

void EntityItem::mapJoints(const QStringList& modelJointNames) {
    // if we don't have animation, or we're already joint mapped then bail early
    if (!hasAnimation() || _jointMappingCompleted) {
        return;
    }

    Animation* myAnimation = getAnimation(_animationURL);
    
    if (!_jointMappingCompleted) {
        QStringList animationJointNames = myAnimation->getJointNames();

        if (modelJointNames.size() > 0 && animationJointNames.size() > 0) {
            _jointMapping.resize(modelJointNames.size());
            for (int i = 0; i < modelJointNames.size(); i++) {
                _jointMapping[i] = animationJointNames.indexOf(modelJointNames[i]);
            }
            _jointMappingCompleted = true;
        }
    }
}

QVector<glm::quat> EntityItem::getAnimationFrame() {
    QVector<glm::quat> frameData;
    if (hasAnimation() && _jointMappingCompleted) {
        Animation* myAnimation = getAnimation(_animationURL);
        QVector<FBXAnimationFrame> frames = myAnimation->getFrames();
        int frameCount = frames.size();

        if (frameCount > 0) {
            int animationFrameIndex = (int)glm::floor(_animationFrameIndex) % frameCount;
            QVector<glm::quat> rotations = frames[animationFrameIndex].rotations;
            frameData.resize(_jointMapping.size());
            for (int j = 0; j < _jointMapping.size(); j++) {
                int rotationIndex = _jointMapping[j];
                if (rotationIndex != -1 && rotationIndex < rotations.size()) {
                    frameData[j] = rotations[rotationIndex];
                }
            }
        }
    }
    return frameData;
}

void EntityItem::update(const quint64& updateTime) {
    _lastUpdated = updateTime;
    setShouldBeDeleted(getShouldBeDeleted());

    quint64 now = usecTimestampNow();

    // only advance the frame index if we're playing
    if (getAnimationIsPlaying()) {

        float deltaTime = (float)(now - _lastAnimated) / (float)USECS_PER_SECOND;
        
        const bool wantDebugging = false;
        if (wantDebugging) {
            qDebug() << "EntityItem::update() now=" << now;
            qDebug() << "             updateTime=" << updateTime;
            qDebug() << "          _lastAnimated=" << _lastAnimated;
            qDebug() << "              deltaTime=" << deltaTime;
        }
        _lastAnimated = now;
        _animationFrameIndex += deltaTime * _animationFPS;

        if (wantDebugging) {
            qDebug() << "   _animationFrameIndex=" << _animationFrameIndex;
        }

    } else {
        _lastAnimated = now;
    }
}

void EntityItem::copyChangedProperties(const EntityItem& other) {
    *this = other;
}

EntityItemProperties EntityItem::getProperties() const {
    EntityItemProperties properties;
    properties.copyFromEntityItem(*this);
    return properties;
}

void EntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    properties.copyToEntityItem(*this, forceCopy);
}

EntityItemProperties::EntityItemProperties() :
    _position(0),
    _color(),
    _radius(ENTITY_DEFAULT_RADIUS),
    _shouldBeDeleted(false),
    _modelURL(""),
    _rotation(ENTITY_DEFAULT_ROTATION),
    _animationURL(""),
    _animationIsPlaying(false),
    _animationFrameIndex(0.0),
    _animationFPS(ENTITY_DEFAULT_ANIMATION_FPS),
    _glowLevel(0.0f),

    _id(UNKNOWN_ENTITY_ID),
    _idSet(false),
    _lastEdited(usecTimestampNow()),

    _positionChanged(false),
    _colorChanged(false),
    _radiusChanged(false),
    _shouldBeDeletedChanged(false),
    _modelURLChanged(false),
    _rotationChanged(false),
    _animationURLChanged(false),
    _animationIsPlayingChanged(false),
    _animationFrameIndexChanged(false),
    _animationFPSChanged(false),
    _glowLevelChanged(false),
    _defaultSettings(true)
{
}

void EntityItemProperties::debugDump() const {
    qDebug() << "EntityItemProperties...";
    qDebug() << "   _id=" << _id;
    qDebug() << "   _idSet=" << _idSet;
    qDebug() << "   _position=" << _position.x << "," << _position.y << "," << _position.z;
    qDebug() << "   _radius=" << _radius;
    qDebug() << "   _modelURL=" << _modelURL;
    qDebug() << "   _animationURL=" << _animationURL;
}


uint16_t EntityItemProperties::getChangedBits() const {
    uint16_t changedBits = 0;
    if (_radiusChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_RADIUS;
    }

    if (_positionChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_POSITION;
    }

    if (_colorChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_COLOR;
    }

    if (_shouldBeDeletedChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_SHOULDDIE;
    }

    if (_modelURLChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_MODEL_URL;
    }

    if (_rotationChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_ROTATION;
    }

    if (_animationURLChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_ANIMATION_URL;
    }

    if (_animationIsPlayingChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_ANIMATION_PLAYING;
    }

    if (_animationFrameIndexChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_ANIMATION_FRAME;
    }

    if (_animationFPSChanged) {
        changedBits += ENTITY_PACKET_CONTAINS_ANIMATION_FPS;
    }

    return changedBits;
}


QScriptValue EntityItemProperties::copyToScriptValue(QScriptEngine* engine) const {
    QScriptValue properties = engine->newObject();

    QScriptValue position = vec3toScriptValue(engine, _position);
    properties.setProperty("position", position);

    QScriptValue color = xColorToScriptValue(engine, _color);
    properties.setProperty("color", color);

    properties.setProperty("radius", _radius);

    properties.setProperty("shouldBeDeleted", _shouldBeDeleted);

    properties.setProperty("modelURL", _modelURL);

    QScriptValue modelRotation = quatToScriptValue(engine, _rotation);
    properties.setProperty("modelRotation", modelRotation);

    properties.setProperty("animationURL", _animationURL);
    properties.setProperty("animationIsPlaying", _animationIsPlaying);
    properties.setProperty("animationFrameIndex", _animationFrameIndex);
    properties.setProperty("animationFPS", _animationFPS);
    properties.setProperty("glowLevel", _glowLevel);

    if (_idSet) {
        properties.setProperty("id", _id);
        properties.setProperty("isKnownID", (_id != UNKNOWN_ENTITY_ID));
    }

    // Sitting properties support
    QScriptValue sittingPoints = engine->newObject();
    for (int i = 0; i < _sittingPoints.size(); ++i) {
        QScriptValue sittingPoint = engine->newObject();
        sittingPoint.setProperty("name", _sittingPoints[i].name);
        sittingPoint.setProperty("position", vec3toScriptValue(engine, _sittingPoints[i].position));
        sittingPoint.setProperty("rotation", quatToScriptValue(engine, _sittingPoints[i].rotation));
        sittingPoints.setProperty(i, sittingPoint);
    }
    sittingPoints.setProperty("length", _sittingPoints.size());
    properties.setProperty("sittingPoints", sittingPoints);

    return properties;
}

void EntityItemProperties::copyFromScriptValue(const QScriptValue &object) {

    QScriptValue position = object.property("position");
    if (position.isValid()) {
        QScriptValue x = position.property("x");
        QScriptValue y = position.property("y");
        QScriptValue z = position.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            if (_defaultSettings || newPosition != _position) {
                _position = newPosition;
                _positionChanged = true;
            }
        }
    }

    QScriptValue color = object.property("color");
    if (color.isValid()) {
        QScriptValue red = color.property("red");
        QScriptValue green = color.property("green");
        QScriptValue blue = color.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            xColor newColor;
            newColor.red = red.toVariant().toInt();
            newColor.green = green.toVariant().toInt();
            newColor.blue = blue.toVariant().toInt();
            if (_defaultSettings || (newColor.red != _color.red ||
                newColor.green != _color.green ||
                newColor.blue != _color.blue)) {
                _color = newColor;
                _colorChanged = true;
            }
        }
    }

    QScriptValue radius = object.property("radius");
    if (radius.isValid()) {
        float newRadius;
        newRadius = radius.toVariant().toFloat();
        if (_defaultSettings || newRadius != _radius) {
            _radius = newRadius;
            _radiusChanged = true;
        }
    }

    QScriptValue shouldBeDeleted = object.property("shouldBeDeleted");
    if (shouldBeDeleted.isValid()) {
        bool newShouldBeDeleted;
        newShouldBeDeleted = shouldBeDeleted.toVariant().toBool();
        if (_defaultSettings || newShouldBeDeleted != _shouldBeDeleted) {
            _shouldBeDeleted = newShouldBeDeleted;
            _shouldBeDeletedChanged = true;
        }
    }

    QScriptValue modelURL = object.property("modelURL");
    if (modelURL.isValid()) {
        QString newModelURL;
        newModelURL = modelURL.toVariant().toString();
        if (_defaultSettings || newModelURL != _modelURL) {
            _modelURL = newModelURL;
            _modelURLChanged = true;
        }
    }

    QScriptValue modelRotation = object.property("modelRotation");
    if (modelRotation.isValid()) {
        QScriptValue x = modelRotation.property("x");
        QScriptValue y = modelRotation.property("y");
        QScriptValue z = modelRotation.property("z");
        QScriptValue w = modelRotation.property("w");
        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            if (_defaultSettings || newRotation != _rotation) {
                _rotation = newRotation;
                _rotationChanged = true;
            }
        }
    }

    QScriptValue animationURL = object.property("animationURL");
    if (animationURL.isValid()) {
        QString newAnimationURL;
        newAnimationURL = animationURL.toVariant().toString();
        if (_defaultSettings || newAnimationURL != _animationURL) {
            _animationURL = newAnimationURL;
            _animationURLChanged = true;
        }
    }

    QScriptValue animationIsPlaying = object.property("animationIsPlaying");
    if (animationIsPlaying.isValid()) {
        bool newIsAnimationPlaying;
        newIsAnimationPlaying = animationIsPlaying.toVariant().toBool();
        if (_defaultSettings || newIsAnimationPlaying != _animationIsPlaying) {
            _animationIsPlaying = newIsAnimationPlaying;
            _animationIsPlayingChanged = true;
        }
    }
    
    QScriptValue animationFrameIndex = object.property("animationFrameIndex");
    if (animationFrameIndex.isValid()) {
        float newFrameIndex;
        newFrameIndex = animationFrameIndex.toVariant().toFloat();
        if (_defaultSettings || newFrameIndex != _animationFrameIndex) {
            _animationFrameIndex = newFrameIndex;
            _animationFrameIndexChanged = true;
        }
    }
    
    QScriptValue animationFPS = object.property("animationFPS");
    if (animationFPS.isValid()) {
        float newFPS;
        newFPS = animationFPS.toVariant().toFloat();
        if (_defaultSettings || newFPS != _animationFPS) {
            _animationFPS = newFPS;
            _animationFPSChanged = true;
        }
    }
    
    QScriptValue glowLevel = object.property("glowLevel");
    if (glowLevel.isValid()) {
        float newGlowLevel;
        newGlowLevel = glowLevel.toVariant().toFloat();
        if (_defaultSettings || newGlowLevel != _glowLevel) {
            _glowLevel = newGlowLevel;
            _glowLevelChanged = true;
        }
    }

    _lastEdited = usecTimestampNow();
}

void EntityItemProperties::copyToEntityItem(EntityItem& entityItem, bool forceCopy) const {
    bool somethingChanged = false;
    if (_positionChanged || forceCopy) {
        entityItem.setPosition(_position / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_colorChanged || forceCopy) {
        entityItem.setColor(_color);
        somethingChanged = true;
    }

    if (_radiusChanged || forceCopy) {
        entityItem.setRadius(_radius / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_shouldBeDeletedChanged || forceCopy) {
        entityItem.setShouldBeDeleted(_shouldBeDeleted);
        somethingChanged = true;
    }

    if (_modelURLChanged || forceCopy) {
        entityItem.setModelURL(_modelURL);
        somethingChanged = true;
    }

    if (_rotationChanged || forceCopy) {
        entityItem.setRotation(_rotation);
        somethingChanged = true;
    }

    if (_animationURLChanged || forceCopy) {
        entityItem.setAnimationURL(_animationURL);
        somethingChanged = true;
    }

    if (_animationIsPlayingChanged || forceCopy) {
        entityItem.setAnimationIsPlaying(_animationIsPlaying);
        somethingChanged = true;
    }

    if (_animationFrameIndexChanged || forceCopy) {
        entityItem.setAnimationFrameIndex(_animationFrameIndex);
        somethingChanged = true;
    }
    
    if (_animationFPSChanged || forceCopy) {
        entityItem.setAnimationFPS(_animationFPS);
        somethingChanged = true;
    }
    
    if (_glowLevelChanged || forceCopy) {
        entityItem.setGlowLevel(_glowLevel);
        somethingChanged = true;
    }

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - _lastEdited;
            qDebug() << "EntityItemProperties::copyToEntityItem() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " _lastEdited=" << _lastEdited;
        }
        entityItem.setLastEdited(_lastEdited);
    }
}

void EntityItemProperties::copyFromEntityItem(const EntityItem& entityItem) {
    _position = entityItem.getPosition() * (float) TREE_SCALE;
    _color = entityItem.getXColor();
    _radius = entityItem.getRadius() * (float) TREE_SCALE;
    _shouldBeDeleted = entityItem.getShouldBeDeleted();
    _modelURL = entityItem.getModelURL();
    _rotation = entityItem.getRotation();
    _animationURL = entityItem.getAnimationURL();
    _animationIsPlaying = entityItem.getAnimationIsPlaying();
    _animationFrameIndex = entityItem.getAnimationFrameIndex();
    _animationFPS = entityItem.getAnimationFPS();
    _glowLevel = entityItem.getGlowLevel();
    _sittingPoints = entityItem.getSittingPoints(); // sitting support

    _id = entityItem.getID();
    _idSet = true;

    _positionChanged = false;
    _colorChanged = false;
    _radiusChanged = false;
    
    _shouldBeDeletedChanged = false;
    _modelURLChanged = false;
    _rotationChanged = false;
    _animationURLChanged = false;
    _animationIsPlayingChanged = false;
    _animationFrameIndexChanged = false;
    _animationFPSChanged = false;
    _glowLevelChanged = false;
    _defaultSettings = false;
}

QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties) {
    return properties.copyToScriptValue(engine);
}

void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties) {
    properties.copyFromScriptValue(object);
}


QScriptValue EntityItemIDtoScriptValue(QScriptEngine* engine, const EntityItemID& id) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("id", id.id);
    obj.setProperty("creatorTokenID", id.creatorTokenID);
    obj.setProperty("isKnownID", id.isKnownID);
    return obj;
}

void EntityItemIDfromScriptValue(const QScriptValue &object, EntityItemID& id) {
    id.id = object.property("id").toVariant().toUInt();
    id.creatorTokenID = object.property("creatorTokenID").toVariant().toUInt();
    id.isKnownID = object.property("isKnownID").toVariant().toBool();
}




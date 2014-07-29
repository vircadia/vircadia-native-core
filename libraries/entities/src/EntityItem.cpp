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

EntityItem::EntityItem() {
qDebug() << "EntityItem::EntityItem()....";
    _type = EntityTypes::Base;
    _lastEdited = 0;
    _lastUpdated = 0;
    rgbColor noColor = { 0, 0, 0 };
    init(glm::vec3(0,0,0), 0, noColor, NEW_ENTITY);
}

void EntityItem::initFromEntityItemID(const EntityItemID& entityItemID) {
    _id = entityItemID.id;
    _creatorTokenID = entityItemID.creatorTokenID;

    // init values with defaults before calling setProperties
    //uint64_t now = usecTimestampNow();
    _lastEdited = 0;
    _lastUpdated = 0;

    _position = glm::vec3(0,0,0);
    _radius = 0;
    _rotation = ENTITY_DEFAULT_ROTATION;
    _shouldBeDeleted = false;
    

#ifdef HIDE_SUBCLASS_METHODS
    rgbColor noColor = { 0, 0, 0 };
    memcpy(_color, noColor, sizeof(_color));
    _modelURL = ENTITY_DEFAULT_MODEL_URL;

    // animation related
    _animationURL = ENTITY_DEFAULT_ANIMATION_URL;
    _animationIsPlaying = false;
    _animationFrameIndex = 0.0f;
    _animationFPS = ENTITY_DEFAULT_ANIMATION_FPS;
    _glowLevel = 0.0f;

    _jointMappingCompleted = false;
    _lastAnimated = 0;
#endif
}

EntityItem::EntityItem(const EntityItemID& entityItemID) {
    //qDebug() << "EntityItem::EntityItem(const EntityItemID& entityItemID)....";
    _type = EntityTypes::Base;
    initFromEntityItemID(entityItemID);
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) {
    //qDebug() << "EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties)....";
    _type = EntityTypes::Base;
    _lastEdited = 0;
    _lastUpdated = 0;
    initFromEntityItemID(entityItemID);
    //qDebug() << "EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties).... _lastEdited=" << _lastEdited;
    setProperties(properties, true); // force copy
    //qDebug() << "EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties).... after setProperties() _lastEdited=" << _lastEdited;
}

EntityItem::~EntityItem() {
}

void EntityItem::init(glm::vec3 position, float radius, rgbColor color, uint32_t id) {
    
    // TODO: is this what we want???
    /*
    if (id == NEW_ENTITY) {
        _id = _nextID;
        _nextID++;
    } else {
        _id = id;
    }
    */
    
    _id = id;
    
    quint64 now = usecTimestampNow();
    _lastEdited = now;
    _lastUpdated = now;
    _position = position;
    _radius = radius;
    _rotation = ENTITY_DEFAULT_ROTATION;
    _shouldBeDeleted = false;


#ifdef HIDE_SUBCLASS_METHODS
    memcpy(_color, color, sizeof(_color));
    _modelURL = ENTITY_DEFAULT_MODEL_URL;
    // animation related
    _animationURL = ENTITY_DEFAULT_ANIMATION_URL;
    _animationIsPlaying = false;
    _animationFrameIndex = 0.0f;
    _animationFPS = ENTITY_DEFAULT_ANIMATION_FPS;
    _glowLevel = 0.0f;
    _jointMappingCompleted = false;
    _lastAnimated = now;
#endif
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

        // PROP_SCRIPT
        //     script would go here...
        

#ifdef HIDE_SUBCLASS_METHODS    
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

#endif //def HIDE_SUBCLASS_METHODS    
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
#ifdef HIDE_SUBCLASS_METHODS
        memcpy(_color, dataAt, sizeof(_color));
        dataAt += sizeof(_color);
        bytesRead += sizeof(_color);
#endif

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
#ifdef HIDE_SUBCLASS_METHODS
        setModelURL(modelURLString);
#endif
        dataAt += modelURLLength;
        bytesRead += modelURLLength;

        // rotation
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
#ifdef HIDE_SUBCLASS_METHODS
            setAnimationURL(animationURLString);
#endif
            dataAt += animationURLLength;
            bytesRead += animationURLLength;

#ifdef HIDE_SUBCLASS_METHODS
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
#endif
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
        quint32 type = typeCoder;
        _type = (EntityTypes::EntityType_t)type;

// XXXBHG: is this a good place to handle the last edited time client vs server??

// If the edit time encoded in the packet is NEWER than our known edit time... 
//     then we WANT to over-write our local data.
// If the edit time encoded in the packet is OLDER than our known edit time...
//     then we WANT to preserve our local data. (NOTE: what if I change color, and you change position?? last one wins!)

        bool overwriteLocalData = true; // assume the new content overwrites our local data
        
        quint64 lastEditedFromBuffer = 0;

        // _lastEdited
        memcpy(&lastEditedFromBuffer, dataAt, sizeof(lastEditedFromBuffer));
        dataAt += sizeof(lastEditedFromBuffer);
        bytesRead += sizeof(lastEditedFromBuffer);
        lastEditedFromBuffer -= clockSkew;
        
        // If we've changed our local tree more recently than the new data from this packet
        // then we will not be changing our values, instead we just read and skip the data
        if (_lastEdited > lastEditedFromBuffer) {
            overwriteLocalData = false;
            qDebug() << "IGNORING old data from server!!! **************** _lastEdited=" << _lastEdited 
                        << "lastEditedFromBuffer=" << lastEditedFromBuffer << "now=" << usecTimestampNow();
        } else {

            qDebug() << "USING NEW data from server!!! **************** OLD _lastEdited=" << _lastEdited 
                        << "lastEditedFromBuffer=" << lastEditedFromBuffer << "now=" << usecTimestampNow();

            _lastEdited = lastEditedFromBuffer;
        }

        // last updated is stored as ByteCountCoded delta from lastEdited
        QByteArray encodedUpdateDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
        quint64 updateDelta = updateDeltaCoder;
        if (overwriteLocalData) {
            _lastUpdated = _lastEdited + updateDelta; // don't adjust for clock skew since we already did that for _lastEdited
        }
        encodedUpdateDelta = updateDeltaCoder; // determine true length
        dataAt += encodedUpdateDelta.size();
        bytesRead += encodedUpdateDelta.size();

        // Property Flags
        QByteArray encodedPropertyFlags = originalDataBuffer.mid(bytesRead); // maximum possible size
        EntityPropertyFlags propertyFlags = encodedPropertyFlags;
        dataAt += propertyFlags.getEncodedLength();
        bytesRead += propertyFlags.getEncodedLength();

        // PROP_POSITION
        if (propertyFlags.getHasProperty(PROP_POSITION)) {
            glm::vec3 positionFromBuffer;
            memcpy(&positionFromBuffer, dataAt, sizeof(positionFromBuffer));
            dataAt += sizeof(positionFromBuffer);
            bytesRead += sizeof(positionFromBuffer);
            if (overwriteLocalData) {
                _position = positionFromBuffer;
            }
        }
        
        // PROP_RADIUS
        if (propertyFlags.getHasProperty(PROP_RADIUS)) {
            float radiusFromBuffer;
            memcpy(&radiusFromBuffer, dataAt, sizeof(radiusFromBuffer));
            dataAt += sizeof(radiusFromBuffer);
            bytesRead += sizeof(radiusFromBuffer);
            if (overwriteLocalData) {
                _radius = radiusFromBuffer;
            }
        }

        // PROP_ROTATION
        if (propertyFlags.getHasProperty(PROP_ROTATION)) {
            glm::quat rotation;
            int bytes = unpackOrientationQuatFromBytes(dataAt, rotation);
            dataAt += bytes;
            bytesRead += bytes;
            if (overwriteLocalData) {
                _rotation = rotation;
            }
        }

        // PROP_SHOULD_BE_DELETED
        if (propertyFlags.getHasProperty(PROP_SHOULD_BE_DELETED)) {
            bool shouldBeDeleted;
            memcpy(&shouldBeDeleted, dataAt, sizeof(shouldBeDeleted));
            dataAt += sizeof(shouldBeDeleted);
            bytesRead += sizeof(shouldBeDeleted);
            if (overwriteLocalData) {
                _shouldBeDeleted = shouldBeDeleted;
            }
        }

        // PROP_SCRIPT
        //     script would go here...
        
        
#ifdef HIDE_SUBCLASS_METHODS    
        // PROP_COLOR
        if (propertyFlags.getHasProperty(PROP_COLOR)) {
            rgbColor color;
            if (overwriteLocalData) {
                memcpy(_color, dataAt, sizeof(_color));
            }
            dataAt += sizeof(color);
            bytesRead += sizeof(color);
        }

        // PROP_MODEL_URL
        if (propertyFlags.getHasProperty(PROP_MODEL_URL)) {
        
            // TODO: fix to new format...
            uint16_t modelURLLength;
            memcpy(&modelURLLength, dataAt, sizeof(modelURLLength));
            dataAt += sizeof(modelURLLength);
            bytesRead += sizeof(modelURLLength);
            QString modelURLString((const char*)dataAt);
            dataAt += modelURLLength;
            bytesRead += modelURLLength;
            if (overwriteLocalData) {
                setModelURL(modelURLString);
            }
        }

        // PROP_ANIMATION_URL
        if (propertyFlags.getHasProperty(PROP_ANIMATION_URL)) {
            // animationURL
            uint16_t animationURLLength;
            memcpy(&animationURLLength, dataAt, sizeof(animationURLLength));
            dataAt += sizeof(animationURLLength);
            bytesRead += sizeof(animationURLLength);
            QString animationURLString((const char*)dataAt);
            dataAt += animationURLLength;
            bytesRead += animationURLLength;
            if (overwriteLocalData) {
                setAnimationURL(animationURLString);
            }
        }        

        // PROP_ANIMATION_FPS
        if (propertyFlags.getHasProperty(PROP_ANIMATION_FPS)) {
            float animationFPS;
            memcpy(&animationFPS, dataAt, sizeof(animationFPS));
            dataAt += sizeof(animationFPS);
            bytesRead += sizeof(animationFPS);
            if (overwriteLocalData) {
                _animationFPS = animationFPS;
            }
        }

        // PROP_ANIMATION_FRAME_INDEX
        if (propertyFlags.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
            float animationFrameIndex;
            memcpy(&animationFrameIndex, dataAt, sizeof(animationFrameIndex));
            dataAt += sizeof(animationFrameIndex);
            bytesRead += sizeof(animationFrameIndex);
            if (overwriteLocalData) {
                _animationFrameIndex = animationFrameIndex;
            }
        }

        // PROP_ANIMATION_PLAYING
        if (propertyFlags.getHasProperty(PROP_ANIMATION_PLAYING)) {
            bool animationIsPlaying;
            memcpy(&animationIsPlaying, dataAt, sizeof(animationIsPlaying));
            dataAt += sizeof(animationIsPlaying);
            bytesRead += sizeof(animationIsPlaying);
            if (overwriteLocalData) {
                _animationIsPlaying = animationIsPlaying;
            }
        }
#endif
    }
    return bytesRead;
}

void EntityItem::debugDump() const {
    qDebug() << "EntityItem id:" << getEntityItemID();
    qDebug(" edited ago:%f", getEditedAgo());
    qDebug(" should die:%s", debug::valueOf(getShouldBeDeleted()));
    qDebug(" position:%f,%f,%f", _position.x, _position.y, _position.z);
    qDebug(" radius:%f", getRadius());

#ifdef HIDE_SUBCLASS_METHODS
    qDebug(" color:%d,%d,%d", _color[0], _color[1], _color[2]);
    if (!getModelURL().isEmpty()) {
        qDebug() << " modelURL:" << qPrintable(getModelURL());
    } else {
        qDebug() << " modelURL: NONE";
    }
#endif
}



//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//Change this to use property flags...
//How do we also change this to support spanning multiple MTUs...
//Need to output the encode structure like handling packets over the wire...
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

bool EntityItem::encodeEntityEditMessageDetails(PacketType command, EntityItemID id, const EntityItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {

    OctreePacketData packetData(false, sizeIn); // create a packetData object to add out packet details too.

    bool success = true; // assume the best
    OctreeElement::AppendState appendState = OctreeElement::COMPLETED; // assume the best
    sizeOut = 0;

    // TODO: We need to review how jurisdictions should be handled for entities. (The old Models and Particles code
    // didn't do anything special for jurisdictions, so we're keeping that same behavior here.)
    //
    // Always include the root octcode. This is only because the OctreeEditPacketSender will check these octcodes
    // to determine which server to send the changes to in the case of multiple jurisdictions. The root will be sent
    // to all servers.
    glm::vec3 rootPosition(0);
    float rootScale = 0.5f;
    unsigned char* octcode = pointToOctalCode(rootPosition.x, rootPosition.y, rootPosition.z, rootScale);

    success = packetData.startSubTree(octcode);
    delete[] octcode;
    
    // assuming we have rome to fit our octalCode, proceed...
    if (success) {

        // Now add our edit content details...
        bool isNewEntityItem = (id.id == NEW_ENTITY);

        // id
        // encode our ID as a byte count coded byte stream
        ByteCountCoded<quint32> idCoder = id.id;
        QByteArray encodedID = idCoder;

        // encode our ID as a byte count coded byte stream
        ByteCountCoded<quint32> tokenCoder;
        QByteArray encodedToken;

        // special case for handling "new" modelItems
        if (isNewEntityItem) {
            // encode our creator token as a byte count coded byte stream
            tokenCoder = id.creatorTokenID;
            encodedToken = tokenCoder;
        }

        // encode our type as a byte count coded byte stream
        ByteCountCoded<quint32> typeCoder = (quint32)properties.getType();
        QByteArray encodedType = typeCoder;

        quint64 updateDelta = 0; // this is an edit so by definition, it's update is in sync
        ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
        QByteArray encodedUpdateDelta = updateDeltaCoder;
        EntityPropertyFlags propertyFlags(PROP_LAST_ITEM);
        EntityPropertyFlags requestedProperties = properties.getChangedProperties();
        EntityPropertyFlags propertiesDidntFit = requestedProperties;

        // TODO: we need to handle the multi-pass form of this, similar to how we handle entity data
        //
        // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
        // then our modelTreeElementExtraEncodeData should include data about which properties we need to append.
        //if (modelTreeElementExtraEncodeData && modelTreeElementExtraEncodeData->includedItems.contains(getEntityItemID())) {
        //    requestedProperties = modelTreeElementExtraEncodeData->includedItems.value(getEntityItemID());
        //}

        //qDebug() << "requestedProperties=";
        //requestedProperties.debugDumpBits();

        LevelDetails entityLevel = packetData.startLevel();

        // Last Edited quint64 always first, before any other details, which allows us easy access to adjusting this
        // timestamp for clock skew
        bool successLastEditedFits = packetData.appendValue(properties.getLastEdited());
    
        bool successIDFits = packetData.appendValue(encodedID);
        if (isNewEntityItem && successIDFits) {
            successIDFits = packetData.appendValue(encodedToken);
        }
        bool successTypeFits = packetData.appendValue(encodedType);

        // TODO: Should we get rid of this in this in edit packets, since this has to always be 0?
        bool successLastUpdatedFits = packetData.appendValue(encodedUpdateDelta);
    
        int propertyFlagsOffset = packetData.getUncompressedByteOffset();
        QByteArray encodedPropertyFlags = propertyFlags;
        int oldPropertyFlagsLength = encodedPropertyFlags.length();
        bool successPropertyFlagsFits = packetData.appendValue(encodedPropertyFlags);
        int propertyCount = 0;

        bool headerFits = successIDFits && successTypeFits && successLastEditedFits 
                                  && successLastUpdatedFits && successPropertyFlagsFits;

        int startOfEntityItemData = packetData.getUncompressedByteOffset();

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
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendPosition(properties.getPosition());
                if (successPropertyFits) {
                    propertyFlags |= PROP_POSITION;
                    propertiesDidntFit -= PROP_POSITION;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_POSITION didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_POSITION NOT requested...";
                propertiesDidntFit -= PROP_POSITION;
            }

            // PROP_RADIUS
            if (requestedProperties.getHasProperty(PROP_RADIUS)) {
                //qDebug() << "PROP_RADIUS requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getRadius());
                if (successPropertyFits) {
                    propertyFlags |= PROP_RADIUS;
                    propertiesDidntFit -= PROP_RADIUS;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_RADIUS didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_RADIUS NOT requested...";
                propertiesDidntFit -= PROP_RADIUS;
            }

            // PROP_ROTATION
            if (requestedProperties.getHasProperty(PROP_ROTATION)) {
                //qDebug() << "PROP_ROTATION requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getRotation());
                if (successPropertyFits) {
                    propertyFlags |= PROP_ROTATION;
                    propertiesDidntFit -= PROP_ROTATION;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_ROTATION didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_ROTATION NOT requested...";
                propertiesDidntFit -= PROP_ROTATION;
            }

            // PROP_SHOULD_BE_DELETED
            if (requestedProperties.getHasProperty(PROP_SHOULD_BE_DELETED)) {
                //qDebug() << "PROP_SHOULD_BE_DELETED requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getShouldBeDeleted());
                if (successPropertyFits) {
                    propertyFlags |= PROP_SHOULD_BE_DELETED;
                    propertiesDidntFit -= PROP_SHOULD_BE_DELETED;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_SHOULD_BE_DELETED didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_SHOULD_BE_DELETED NOT requested...";
                propertiesDidntFit -= PROP_SHOULD_BE_DELETED;
            }

            // PROP_SCRIPT
            //     script would go here...
        

#if 0 // def HIDE_SUBCLASS_METHODS
            // PROP_COLOR
            if (requestedProperties.getHasProperty(PROP_COLOR)) {
                //qDebug() << "PROP_COLOR requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendColor(properties.getColor());
                if (successPropertyFits) {
                    propertyFlags |= PROP_COLOR;
                    propertiesDidntFit -= PROP_COLOR;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_COLOR didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_COLOR NOT requested...";
                propertiesDidntFit -= PROP_COLOR;
            }

            // PROP_MODEL_URL
            if (requestedProperties.getHasProperty(PROP_MODEL_URL)) {
                //qDebug() << "PROP_MODEL_URL requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getModelURL());
                if (successPropertyFits) {
                    propertyFlags |= PROP_MODEL_URL;
                    propertiesDidntFit -= PROP_MODEL_URL;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_MODEL_URL didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_MODEL_URL NOT requested...";
                propertiesDidntFit -= PROP_MODEL_URL;
            }

            // PROP_ANIMATION_URL
            if (requestedProperties.getHasProperty(PROP_ANIMATION_URL)) {
                //qDebug() << "PROP_ANIMATION_URL requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getAnimationURL());
                if (successPropertyFits) {
                    propertyFlags |= PROP_ANIMATION_URL;
                    propertiesDidntFit -= PROP_ANIMATION_URL;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_ANIMATION_URL didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_ANIMATION_URL NOT requested...";
                propertiesDidntFit -= PROP_ANIMATION_URL;
            }

            // PROP_ANIMATION_FPS
            if (requestedProperties.getHasProperty(PROP_ANIMATION_FPS)) {
                //qDebug() << "PROP_ANIMATION_FPS requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getAnimationFPS());
                if (successPropertyFits) {
                    propertyFlags |= PROP_ANIMATION_FPS;
                    propertiesDidntFit -= PROP_ANIMATION_FPS;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_ANIMATION_FPS didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_ANIMATION_FPS NOT requested...";
                propertiesDidntFit -= PROP_ANIMATION_FPS;
            }

            // PROP_ANIMATION_FRAME_INDEX
            if (requestedProperties.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
                //qDebug() << "PROP_ANIMATION_FRAME_INDEX requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getAnimationFrameIndex());
                if (successPropertyFits) {
                    propertyFlags |= PROP_ANIMATION_FRAME_INDEX;
                    propertiesDidntFit -= PROP_ANIMATION_FRAME_INDEX;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_ANIMATION_FRAME_INDEX didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_ANIMATION_FRAME_INDEX NOT requested...";
                propertiesDidntFit -= PROP_ANIMATION_FRAME_INDEX;
            }

            // PROP_ANIMATION_PLAYING
            if (requestedProperties.getHasProperty(PROP_ANIMATION_PLAYING)) {
                //qDebug() << "PROP_ANIMATION_PLAYING requested...";
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getAnimationIsPlaying());
                if (successPropertyFits) {
                    propertyFlags |= PROP_ANIMATION_PLAYING;
                    propertiesDidntFit -= PROP_ANIMATION_PLAYING;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    //qDebug() << "PROP_ANIMATION_PLAYING didn't fit...";
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                //qDebug() << "PROP_ANIMATION_PLAYING NOT requested...";
                propertiesDidntFit -= PROP_ANIMATION_PLAYING;
            }

#endif //def HIDE_SUBCLASS_METHODS
            
        }
        if (propertyCount > 0) {
            int endOfEntityItemData = packetData.getUncompressedByteOffset();
        
            encodedPropertyFlags = propertyFlags;
            int newPropertyFlagsLength = encodedPropertyFlags.length();
            packetData.updatePriorBytes(propertyFlagsOffset, 
                    (const unsigned char*)encodedPropertyFlags.constData(), encodedPropertyFlags.length());
        
            // if the size of the PropertyFlags shrunk, we need to shift everything down to front of packet.
            if (newPropertyFlagsLength < oldPropertyFlagsLength) {
                int oldSize = packetData.getUncompressedSize();

                const unsigned char* modelItemData = packetData.getUncompressedData(propertyFlagsOffset + oldPropertyFlagsLength);
                int modelItemDataLength = endOfEntityItemData - startOfEntityItemData;
                int newEntityItemDataStart = propertyFlagsOffset + newPropertyFlagsLength;
                packetData.updatePriorBytes(newEntityItemDataStart, modelItemData, modelItemDataLength);

                int newSize = oldSize - (oldPropertyFlagsLength - newPropertyFlagsLength);
                packetData.setUncompressedSize(newSize);

            } else {
                assert(newPropertyFlagsLength == oldPropertyFlagsLength); // should not have grown
            }
       
            packetData.endLevel(entityLevel);
        } else {
            packetData.discardLevel(entityLevel);
            appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
        }
    
        //qDebug() << "propertyFlags=";
        //propertyFlags.debugDumpBits();

        //qDebug() << "propertiesDidntFit=";
        //propertiesDidntFit.debugDumpBits();

        // If any part of the model items didn't fit, then the element is considered partial
        if (appendState != OctreeElement::COMPLETED) {


            // TODO: handle mechanism for handling partial fitting data!
            // add this item into our list for the next appendElementData() pass
            //modelTreeElementExtraEncodeData->includedItems.insert(getEntityItemID(), propertiesDidntFit);

            // for now, if it's not complete, it's not successful
            success = false;
        }
    }
    
    if (success) {
        packetData.endSubTree();
        const unsigned char* finalizedData = packetData.getFinalizedData();
        int  finalizedSize = packetData.getFinalizedSize();
        memcpy(bufferOut, finalizedData, finalizedSize);
        sizeOut = finalizedSize;
        
        bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "encodeEntityEditMessageDetails().... ";
            outputBufferBits(finalizedData, finalizedSize);
        }
        
    } else {
        packetData.discardSubTree();
        sizeOut = 0;
    }
    return success;
}


// adjust any internal timestamps to fix clock skew for this server
void EntityItem::adjustEditPacketForClockSkew(unsigned char* editPacketBuffer, ssize_t length, int clockSkew) {
    unsigned char* dataAt = editPacketBuffer;
    int octets = numberOfThreeBitSectionsInCode(dataAt);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);
    dataAt += lengthOfOctcode;

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

#ifdef HIDE_SUBCLASS_METHODS
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
#endif

void EntityItem::update(const quint64& updateTime) {
    _lastUpdated = updateTime;
    setShouldBeDeleted(getShouldBeDeleted());

    quint64 now = usecTimestampNow();

    // only advance the frame index if we're playing
#ifdef HIDE_SUBCLASS_METHODS
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
#endif
}

void EntityItem::copyChangedProperties(const EntityItem& other) {
    *this = other;
}

EntityItemProperties EntityItem::getProperties() const {
    EntityItemProperties properties;
    //properties.copyFromEntityItem(*this);
    
    
    properties._id = getID();
    properties._idSet = true;

    properties._type = getType();
    
    properties._position = getPosition() * (float) TREE_SCALE;
    properties._radius = getRadius() * (float) TREE_SCALE;
    properties._rotation = getRotation();
    properties._shouldBeDeleted = getShouldBeDeleted();

    properties._positionChanged = false;
    properties._radiusChanged = false;
    properties._rotationChanged = false;
    properties._shouldBeDeletedChanged = false;
    

#if 0 //def HIDE_SUBCLASS_METHODS
    properties._color = getXColor();
    properties._modelURL = getModelURL();
    properties._animationURL = getAnimationURL();
    properties._animationIsPlaying = getAnimationIsPlaying();
    properties._animationFrameIndex = getAnimationFrameIndex();
    properties._animationFPS = getAnimationFPS();
    properties._glowLevel = getGlowLevel();
    properties._sittingPoints = getSittingPoints(); // sitting support
    properties._colorChanged = false;
    properties._modelURLChanged = false;
    properties._animationURLChanged = false;
    properties._animationIsPlayingChanged = false;
    properties._animationFrameIndexChanged = false;
    properties._animationFPSChanged = false;
    properties._glowLevelChanged = false;
#endif

    properties._defaultSettings = false;
    
    return properties;
}

void EntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    bool somethingChanged = false;
    if (properties._positionChanged || forceCopy) {
        setPosition(properties._position / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (properties._radiusChanged || forceCopy) {
        setRadius(properties._radius / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (properties._rotationChanged || forceCopy) {
        setRotation(properties._rotation);
        somethingChanged = true;
    }

    if (properties._shouldBeDeletedChanged || forceCopy) {
        setShouldBeDeleted(properties._shouldBeDeleted);
        somethingChanged = true;
    }


#if 0 // def HIDE_SUBCLASS_METHODS
    if (properties._colorChanged || forceCopy) {
        setColor(properties._color);
        somethingChanged = true;
    }

    if (properties._modelURLChanged || forceCopy) {
        setModelURL(properties._modelURL);
        somethingChanged = true;
    }

    if (properties._animationURLChanged || forceCopy) {
        setAnimationURL(properties._animationURL);
        somethingChanged = true;
    }

    if (properties._animationIsPlayingChanged || forceCopy) {
        setAnimationIsPlaying(properties._animationIsPlaying);
        somethingChanged = true;
    }

    if (properties._animationFrameIndexChanged || forceCopy) {
        setAnimationFrameIndex(properties._animationFrameIndex);
        somethingChanged = true;
    }
    
    if (properties._animationFPSChanged || forceCopy) {
        setAnimationFPS(properties._animationFPS);
        somethingChanged = true;
    }
    
    if (properties._glowLevelChanged || forceCopy) {
        setGlowLevel(properties._glowLevel);
        somethingChanged = true;
    }
#endif

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - _lastEdited;
            qDebug() << "EntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " _lastEdited=" << _lastEdited;
        }
        setLastEdited(properties._lastEdited);
    }

}


//
//  ModelEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <ByteCountCoding.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "ModelEntityItem.h"

// our non-pure virtual subclass for now...
ModelEntityItem::ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) 
{ 
    qDebug() << "ModelEntityItem::ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties)...";
    _type = EntityTypes::Model;     

    qDebug() << "ModelEntityItem::ModelEntityItem() properties.getModelURL()=" << properties.getModelURL();
    
    qDebug() << "ModelEntityItem::ModelEntityItem() calling setProperties()";
    setProperties(properties);
    qDebug() << "ModelEntityItem::ModelEntityItem() getModelURL()=" << getModelURL();

}

EntityItemProperties ModelEntityItem::getProperties() const {
    qDebug() << "ModelEntityItem::getProperties()... <<<<<<<<<<<<<<<<  <<<<<<<<<<<<<<<<<<<<<<<<<";

    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

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

    qDebug() << "ModelEntityItem::getProperties() getModelURL()=" << getModelURL();

    return properties;
}

void ModelEntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    qDebug() << "ModelEntityItem::setProperties()...";
    qDebug() << "ModelEntityItem::ModelEntityItem() properties.getModelURL()=" << properties.getModelURL();
    bool somethingChanged = false;
    
    EntityItem::setProperties(properties, forceCopy); // set the properties in our base class

    if (properties._colorChanged || forceCopy) {
        setColor(properties._color);
        somethingChanged = true;
    }

    if (properties._modelURLChanged || forceCopy) {
        setModelURL(properties._modelURL);
        
qDebug() << "ModelEntityItem::setProperties() getModelURL()=" << getModelURL() << " ---- SETTING!!! --------";
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

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - _lastEdited;
            qDebug() << "ModelEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " _lastEdited=" << _lastEdited;
        }
        setLastEdited(properties._lastEdited);
    }
}



int ModelEntityItem::readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {


qDebug() << "ModelEntityItem::readEntityDataFromBuffer()... <<<<<<<<<<<<<<<<  <<<<<<<<<<<<<<<<<<<<<<<<<";

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
        

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: only handle these subclass items here, use the base class for the rest...
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        
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
        
qDebug() << "ModelEntityItem::readEntityDataFromBuffer()... modelURL=" << getModelURL();
        

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
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    }
    return bytesRead;
}

int ModelEntityItem::oldVersionReadEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {

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
        memcpy(&_color, dataAt, sizeof(_color));
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


OctreeElement::AppendState ModelEntityItem::appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                            EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData) const {



qDebug() << "ModelEntityItem::appendEntityData()... ********************************************";

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
    
    quint64 lastEdited = getLastEdited();
qDebug() << "ModelEntityItem::appendEntityData() ... lastEdited=" << lastEdited;
    
    bool successLastEditedFits = packetData->appendValue(lastEdited);
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

qDebug() << "ModelEntityItem::appendEntityData()... modelURL=" << getModelURL();

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
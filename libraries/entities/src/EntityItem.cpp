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

const float EntityItem::IMMORTAL = -1.0f; /// special lifetime which means the entity lives for ever. default lifetime
const float EntityItem::DEFAULT_GLOW_LEVEL = 0.0f;
const float EntityItem::DEFAULT_MASS = 1.0f;
const float EntityItem::DEFAULT_LIFETIME = EntityItem::IMMORTAL;
const float EntityItem::DEFAULT_DAMPING = 0.99f;
const glm::vec3 EntityItem::DEFAULT_VELOCITY = glm::vec3(0, 0, 0);
const glm::vec3 EntityItem::DEFAULT_GRAVITY = glm::vec3(0, (-9.8f / TREE_SCALE), 0);
const QString EntityItem::DEFAULT_SCRIPT = QString("");

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

    _glowLevel = DEFAULT_GLOW_LEVEL;
    _mass = DEFAULT_MASS;
    _velocity = DEFAULT_VELOCITY;
    _gravity = DEFAULT_GRAVITY;
    _damping = DEFAULT_DAMPING;
    _lifetime = DEFAULT_LIFETIME;
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) {
    //qDebug() << "EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties)....";
    _type = EntityTypes::Unknown;
    _lastEdited = 0;
    _lastUpdated = 0;
    initFromEntityItemID(entityItemID);
    setProperties(properties, true); // force copy
}

EntityItem::~EntityItem() {
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
    QByteArray encodedID = getID().toRfc4122();

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
qDebug() << "EntityItem::appendEntityData() ... lastEdited=" << lastEdited;
    
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
        
        // TO DO - put all the other default items here!!!!
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

// TODO: correct this to reflect changes...
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


int EntityItem::readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {

    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
        qDebug() << "EntityItem::readEntityDataFromBuffer()... ERROR CASE...args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU";
        return 0;
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
        QByteArray encodedID = originalDataBuffer.mid(bytesRead, NUM_BYTES_RFC4122_UUID); // maximum possible size
        _id = QUuid::fromRfc4122(encodedID);
        _creatorTokenID = UNKNOWN_ENTITY_TOKEN; // if we know the id, then we don't care about the creator token
        _newlyCreated = false;
        dataAt += encodedID.size();
        bytesRead += encodedID.size();
        
        /**
        ByteCountCoded<quint32> idCoder = encodedID;
        encodedID = idCoder; // determine true length
        dataAt += encodedID.size();
        bytesRead += encodedID.size();
        _id = idCoder;
        **/

        // type
        QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> typeCoder = encodedType;
        encodedType = typeCoder; // determine true length
        dataAt += encodedType.size();
        bytesRead += encodedType.size();
        quint32 type = typeCoder;
        _type = (EntityTypes::EntityType)type;

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
        
    }
    return bytesRead;
}

void EntityItem::debugDump() const {
    qDebug() << "EntityItem id:" << getEntityItemID();
    qDebug(" edited ago:%f", getEditedAgo());
    qDebug(" should die:%s", debug::valueOf(getShouldBeDeleted()));
    qDebug(" position:%f,%f,%f", _position.x, _position.y, _position.z);
    qDebug(" radius:%f", getRadius());
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
        /*
        ByteCountCoded<quint32> idCoder = id.id;
        QByteArray encodedID = idCoder;
        */
        QByteArray encodedID = id.id.toRfc4122(); // NUM_BYTES_RFC4122_UUID

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
        
qDebug() << "(quint32)properties.getType()=" << (quint32)properties.getType();

        QByteArray encodedType = typeCoder;

        quint64 updateDelta = 0; // this is an edit so by definition, it's update is in sync
        ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
        QByteArray encodedUpdateDelta = updateDeltaCoder;

//qDebug() << "EntityItem::encodeEntityEditMessageDetails() ... updateDelta=" << updateDelta;
//qDebug() << "EntityItem::encodeEntityEditMessageDetails() ... encodedUpdateDelta=" << encodedUpdateDelta;

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
        quint64 lastEdited = properties.getLastEdited();

qDebug() << "EntityItem::encodeEntityEditMessageDetails() ... lastEdited=" << lastEdited;

        bool successLastEditedFits = packetData.appendValue(lastEdited);
    
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
            
            // PROP_MASS,
            if (requestedProperties.getHasProperty(PROP_MASS)) {
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getMass());
                if (successPropertyFits) {
                    propertyFlags |= PROP_MASS;
                    propertiesDidntFit -= PROP_MASS;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                propertiesDidntFit -= PROP_MASS;
            }

            // PROP_VELOCITY,
            if (requestedProperties.getHasProperty(PROP_VELOCITY)) {
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getVelocity());
                if (successPropertyFits) {
                    propertyFlags |= PROP_VELOCITY;
                    propertiesDidntFit -= PROP_VELOCITY;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                propertiesDidntFit -= PROP_VELOCITY;
            }

            // PROP_GRAVITY,
            if (requestedProperties.getHasProperty(PROP_GRAVITY)) {
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getGravity());
                if (successPropertyFits) {
                    propertyFlags |= PROP_GRAVITY;
                    propertiesDidntFit -= PROP_GRAVITY;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                propertiesDidntFit -= PROP_GRAVITY;
            }

            // PROP_DAMPING,
            if (requestedProperties.getHasProperty(PROP_DAMPING)) {
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getDamping());
                if (successPropertyFits) {
                    propertyFlags |= PROP_DAMPING;
                    propertiesDidntFit -= PROP_DAMPING;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                propertiesDidntFit -= PROP_DAMPING;
            }

            // PROP_LIFETIME,
            if (requestedProperties.getHasProperty(PROP_LIFETIME)) {
                LevelDetails propertyLevel = packetData.startLevel();
                successPropertyFits = packetData.appendValue(properties.getLifetime());
                if (successPropertyFits) {
                    propertyFlags |= PROP_LIFETIME;
                    propertiesDidntFit -= PROP_LIFETIME;
                    propertyCount++;
                    packetData.endLevel(propertyLevel);
                } else {
                    packetData.discardLevel(propertyLevel);
                    appendState = OctreeElement::PARTIAL;
                }
            } else {
                propertiesDidntFit -= PROP_LIFETIME;
            }
            

            // PROP_SCRIPT
            //     script would go here...


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: move these??? how to handle this for subclass properties???

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

qDebug() << "EntityItem EntityItem::encodeEntityEditMessageDetails() model URL=" << properties.getModelURL();

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            
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

void EntityItem::update(const quint64& updateTime) {
    // nothing to do here... but will add gravity, etc...
}

EntityItem::SimuationState EntityItem::getSimulationState() const {
    return EntityItem::Static;  // change this once we support gravity, etc...
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


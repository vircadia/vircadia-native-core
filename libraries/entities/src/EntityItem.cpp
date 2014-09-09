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
#include <GLMHelpers.h>
#include <Octree.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()
#include <VoxelsScriptingInterface.h>
#include <VoxelDetail.h>

#include "EntityScriptingInterface.h"
#include "EntityItem.h"
#include "EntityTree.h"

const float EntityItem::IMMORTAL = -1.0f; /// special lifetime which means the entity lives for ever. default lifetime
const float EntityItem::DEFAULT_GLOW_LEVEL = 0.0f;
const float EntityItem::DEFAULT_MASS = 1.0f;
const float EntityItem::DEFAULT_LIFETIME = EntityItem::IMMORTAL;
const float EntityItem::DEFAULT_DAMPING = 0.99f;
const glm::vec3 EntityItem::NO_VELOCITY = glm::vec3(0, 0, 0);
const float EntityItem::EPSILON_VELOCITY_LENGTH = (1.0f / 10000.0f) / (float)TREE_SCALE; // really small
const glm::vec3 EntityItem::DEFAULT_VELOCITY = EntityItem::NO_VELOCITY;
const glm::vec3 EntityItem::NO_GRAVITY = glm::vec3(0, 0, 0);
const glm::vec3 EntityItem::REGULAR_GRAVITY = glm::vec3(0, (-9.8f / TREE_SCALE), 0);
const glm::vec3 EntityItem::DEFAULT_GRAVITY = EntityItem::NO_GRAVITY;
const QString EntityItem::DEFAULT_SCRIPT = QString("");
const glm::quat EntityItem::DEFAULT_ROTATION;
const glm::vec3 EntityItem::DEFAULT_DIMENSIONS = glm::vec3(0.1f, 0.1f, 0.1f);


void EntityItem::initFromEntityItemID(const EntityItemID& entityItemID) {
    _id = entityItemID.id;
    _creatorTokenID = entityItemID.creatorTokenID;

    // init values with defaults before calling setProperties
    //uint64_t now = usecTimestampNow();
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    
    _lastUpdated = 0;
    _created = 0; // TODO: when do we actually want to make this "now"

    _position = glm::vec3(0,0,0);
    _rotation = DEFAULT_ROTATION;
    _dimensions = DEFAULT_DIMENSIONS;

    _glowLevel = DEFAULT_GLOW_LEVEL;
    _mass = DEFAULT_MASS;
    _velocity = DEFAULT_VELOCITY;
    _gravity = DEFAULT_GRAVITY;
    _damping = DEFAULT_DAMPING;
    _lifetime = DEFAULT_LIFETIME;
}

EntityItem::EntityItem(const EntityItemID& entityItemID) {
    _type = EntityTypes::Unknown;
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    _lastUpdated = 0;
    _created = 0;
    initFromEntityItemID(entityItemID);
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) {
    _type = EntityTypes::Unknown;
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    _lastUpdated = 0;
    _created = properties.getCreated();
    initFromEntityItemID(entityItemID);
    setProperties(properties, true); // force copy
}

EntityPropertyFlags EntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_POSITION;
    requestedProperties += PROP_DIMENSIONS; // NOTE: PROP_RADIUS obsolete
    requestedProperties += PROP_ROTATION;
    requestedProperties += PROP_MASS;
    requestedProperties += PROP_VELOCITY;
    requestedProperties += PROP_GRAVITY;
    requestedProperties += PROP_DAMPING;
    requestedProperties += PROP_LIFETIME;
    requestedProperties += PROP_SCRIPT;
    // TODO: add PROP_REGISTRATION_POINT,
    // TODO: add PROP_ROTATIONAL_VELOCITY,
    // TODO: add PROP_VISIBLE,
    
    return requestedProperties;
}

OctreeElement::AppendState EntityItem::appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                            EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData) const {
                                            
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
    EntityPropertyFlags requestedProperties = getEntityProperties(params);
    EntityPropertyFlags propertiesDidntFit = requestedProperties;

    // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
    // then our entityTreeElementExtraEncodeData should include data about which properties we need to append.
    if (entityTreeElementExtraEncodeData && entityTreeElementExtraEncodeData->entities.contains(getEntityItemID())) {
        requestedProperties = entityTreeElementExtraEncodeData->entities.value(getEntityItemID());
    }

    LevelDetails entityLevel = packetData->startLevel();

    quint64 lastEdited = getLastEdited();

    const bool wantDebug = false;
    if (wantDebug) {
        float editedAgo = getEditedAgo();
        QString agoAsString = formatSecondsElapsed(editedAgo);
        qDebug() << "Writing entity " << getEntityItemID() << " to buffer, lastEdited =" << lastEdited 
                        << " ago=" << editedAgo << "seconds - " << agoAsString;
    }

    bool successIDFits = false;
    bool successTypeFits = false;
    bool successCreatedFits = false;
    bool successLastEditedFits = false;
    bool successLastUpdatedFits = false;
    bool successPropertyFlagsFits = false;
    int propertyFlagsOffset = 0;
    int oldPropertyFlagsLength = 0;
    QByteArray encodedPropertyFlags;
    int propertyCount = 0;

    successIDFits = packetData->appendValue(encodedID);
    if (successIDFits) {
        successTypeFits = packetData->appendValue(encodedType);
    }
    if (successTypeFits) {
        successCreatedFits = packetData->appendValue(_created);
    }
    if (successCreatedFits) {
        successLastEditedFits = packetData->appendValue(lastEdited);
    }
    if (successLastEditedFits) {
        successLastUpdatedFits = packetData->appendValue(encodedUpdateDelta);
    }
    
    if (successLastUpdatedFits) {
        propertyFlagsOffset = packetData->getUncompressedByteOffset();
        encodedPropertyFlags = propertyFlags;
        oldPropertyFlagsLength = encodedPropertyFlags.length();
        successPropertyFlagsFits = packetData->appendValue(encodedPropertyFlags);
    }

    bool headerFits = successIDFits && successTypeFits && successCreatedFits && successLastEditedFits 
                              && successLastUpdatedFits && successPropertyFlagsFits;

    int startOfEntityItemData = packetData->getUncompressedByteOffset();

    if (headerFits) {
        bool successPropertyFits;

        propertyFlags -= PROP_LAST_ITEM; // clear the last item for now, we may or may not set it as the actual item

        // These items would go here once supported....
        //      PROP_PAGED_PROPERTY,
        //      PROP_CUSTOM_PROPERTIES_INCLUDED,

        APPEND_ENTITY_PROPERTY(PROP_POSITION, appendPosition, getPosition());
        APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, appendValue, getDimensions()); // NOTE: PROP_RADIUS obsolete
        APPEND_ENTITY_PROPERTY(PROP_ROTATION, appendValue, getRotation());
        APPEND_ENTITY_PROPERTY(PROP_MASS, appendValue, getMass());
        APPEND_ENTITY_PROPERTY(PROP_VELOCITY, appendValue, getVelocity());
        APPEND_ENTITY_PROPERTY(PROP_GRAVITY, appendValue, getGravity());
        APPEND_ENTITY_PROPERTY(PROP_DAMPING, appendValue, getDamping());
        APPEND_ENTITY_PROPERTY(PROP_LIFETIME, appendValue, getLifetime());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT, appendValue, getScript());
        // TODO: add PROP_REGISTRATION_POINT,
        // TODO: add PROP_ROTATIONAL_VELOCITY,
        // TODO: add PROP_VISIBLE,

        appendSubclassData(packetData, params, entityTreeElementExtraEncodeData,
                                requestedProperties,
                                propertyFlags,
                                propertiesDidntFit,
                                propertyCount,
                                appendState);
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
       
        packetData->endLevel(entityLevel);
    } else {
        packetData->discardLevel(entityLevel);
        appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
    }
    
    // If any part of the model items didn't fit, then the element is considered partial
    if (appendState != OctreeElement::COMPLETED) {
        // add this item into our list for the next appendElementData() pass
        entityTreeElementExtraEncodeData->entities.insert(getEntityItemID(), propertiesDidntFit);
    }

    return appendState;
}

// TODO: My goal is to get rid of this concept completely. The old code (and some of the current code) used this
// result to calculate if a packet being sent to it was potentially bad or corrupt. I've adjusted this to now
// only consider the minimum header bytes as being required. But it would be preferable to completely eliminate
// this logic from the callers.
int EntityItem::expectedBytes() {
    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;
    return MINIMUM_HEADER_BYTES;
}


int EntityItem::readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
    bool wantDebug = false;

    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
    
        // NOTE: This shouldn't happen. The only versions of the bit stream that didn't support split mtu buffers should
        // be handled by the model subclass and shouldn't call this routine.
        qDebug() << "EntityItem::readEntityDataFromBuffer()... "
                        "ERROR CASE...args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU";
        return 0;
    }

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;

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
        
        // type
        QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> typeCoder = encodedType;
        encodedType = typeCoder; // determine true length
        dataAt += encodedType.size();
        bytesRead += encodedType.size();
        quint32 type = typeCoder;
        _type = (EntityTypes::EntityType)type;

        bool overwriteLocalData = true; // assume the new content overwrites our local data

        // _created
        quint64 createdFromBuffer = 0;
        memcpy(&createdFromBuffer, dataAt, sizeof(createdFromBuffer));
        dataAt += sizeof(createdFromBuffer);
        bytesRead += sizeof(createdFromBuffer);
        createdFromBuffer -= clockSkew;
        
        _created = createdFromBuffer; // TODO: do we ever want to discard this???

        if (wantDebug) {
            quint64 lastEdited = getLastEdited();
            float editedAgo = getEditedAgo();
            QString agoAsString = formatSecondsElapsed(editedAgo);
            QString ageAsString = formatSecondsElapsed(getAge());
            qDebug() << "Loading entity " << getEntityItemID() << " from buffer...";
            qDebug() << "    _created =" << _created;
            qDebug() << "    age=" << getAge() << "seconds - " << ageAsString;
            qDebug() << "    lastEdited =" << lastEdited;
            qDebug() << "    ago=" << editedAgo << "seconds - " << agoAsString;
        }
        
        quint64 now = usecTimestampNow();
        quint64 lastEditedFromBuffer = 0;
        quint64 lastEditedFromBufferAdjusted = 0;

        // TODO: we could make this encoded as a delta from _created
        // _lastEdited
        memcpy(&lastEditedFromBuffer, dataAt, sizeof(lastEditedFromBuffer));
        dataAt += sizeof(lastEditedFromBuffer);
        bytesRead += sizeof(lastEditedFromBuffer);
        lastEditedFromBufferAdjusted = lastEditedFromBuffer - clockSkew;
        
        bool fromSameServerEdit = (lastEditedFromBuffer == _lastEditedFromRemoteInRemoteTime);

        if (wantDebug) {
            qDebug() << "data from server **************** ";
            qDebug() << "      entityItemID=" << getEntityItemID();
            qDebug() << "      now=" << now;
            qDebug() << "      getLastEdited()=" << getLastEdited();
            qDebug() << "      lastEditedFromBuffer=" << lastEditedFromBuffer << " (BEFORE clockskew adjust)";
            qDebug() << "      clockSkew=" << clockSkew;
            qDebug() << "      lastEditedFromBufferAdjusted=" << lastEditedFromBufferAdjusted << " (AFTER clockskew adjust)";
            qDebug() << "      _lastEditedFromRemote=" << _lastEditedFromRemote 
                                        << " (our local time the last server edit we accepted)";
            qDebug() << "      _lastEditedFromRemoteInRemoteTime=" << _lastEditedFromRemoteInRemoteTime 
                                        << " (remote time the last server edit we accepted)";
            qDebug() << "      fromSameServerEdit=" << fromSameServerEdit;
        }

        bool ignoreServerPacket = false; // assume we're use this server packet
        
        // If this packet is from the same server edit as the last packet we accepted from the server
        // we probably want to use it.
        if (fromSameServerEdit) {
            // If this is from the same sever packet, then check against any local changes since we got
            // the most recent packet from this server time
            if (_lastEdited > _lastEditedFromRemote) {
                ignoreServerPacket = true;
            }
        } else {
            // If this isn't from the same sever packet, then honor our skew adjusted times...
            // If we've changed our local tree more recently than the new data from this packet
            // then we will not be changing our values, instead we just read and skip the data
            if (_lastEdited > lastEditedFromBufferAdjusted) {
                ignoreServerPacket = true;
            }
        }
        
        if (ignoreServerPacket) {
            overwriteLocalData = false;
            if (wantDebug) {
                qDebug() << "IGNORING old data from server!!! ****************";
            }
        } else {

            if (wantDebug) {
                qDebug() << "USING NEW data from server!!! ****************";
            }

            _lastEdited = lastEditedFromBufferAdjusted;
            _lastEditedFromRemote = now;
            _lastEditedFromRemoteInRemoteTime = lastEditedFromBuffer;
            
            somethingChangedNotification(); // notify derived classes that something has changed
        }

        // last updated is stored as ByteCountCoded delta from lastEdited
        QByteArray encodedUpdateDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
        quint64 updateDelta = updateDeltaCoder;
        if (overwriteLocalData) {
            _lastUpdated = lastEditedFromBufferAdjusted + updateDelta; // don't adjust for clock skew since we already did that for _lastEdited
            if (wantDebug) {
                qDebug() << "_lastUpdated=" << _lastUpdated;
                qDebug() << "_lastEdited=" << _lastEdited;
                qDebug() << "lastEditedFromBufferAdjusted=" << lastEditedFromBufferAdjusted;
            }
        }
        encodedUpdateDelta = updateDeltaCoder; // determine true length
        dataAt += encodedUpdateDelta.size();
        bytesRead += encodedUpdateDelta.size();

        // Property Flags
        QByteArray encodedPropertyFlags = originalDataBuffer.mid(bytesRead); // maximum possible size
        EntityPropertyFlags propertyFlags = encodedPropertyFlags;
        dataAt += propertyFlags.getEncodedLength();
        bytesRead += propertyFlags.getEncodedLength();
        
        READ_ENTITY_PROPERTY(PROP_POSITION, glm::vec3, _position);

        // Old bitstreams had PROP_RADIUS, new bitstreams have PROP_DIMENSIONS
        if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_DIMENSIONS) {
            if (propertyFlags.getHasProperty(PROP_RADIUS)) {
                float fromBuffer;
                memcpy(&fromBuffer, dataAt, sizeof(fromBuffer));
                dataAt += sizeof(fromBuffer);
                bytesRead += sizeof(fromBuffer);
                if (overwriteLocalData) {
                    setRadiusInMeters(fromBuffer);
                }
            }
        } else {
            READ_ENTITY_PROPERTY(PROP_DIMENSIONS, glm::vec3, _dimensions);
        }
        READ_ENTITY_PROPERTY_QUAT(PROP_ROTATION, _rotation);
        READ_ENTITY_PROPERTY(PROP_MASS, float, _mass);
        READ_ENTITY_PROPERTY(PROP_VELOCITY, glm::vec3, _velocity);
        READ_ENTITY_PROPERTY(PROP_GRAVITY, glm::vec3, _gravity);
        READ_ENTITY_PROPERTY(PROP_DAMPING, float, _damping);
        READ_ENTITY_PROPERTY(PROP_LIFETIME, float, _lifetime);
        READ_ENTITY_PROPERTY_STRING(PROP_SCRIPT,setScript);
        // TODO: add PROP_REGISTRATION_POINT,
        // TODO: add PROP_ROTATIONAL_VELOCITY,
        // TODO: add PROP_VISIBLE,

        bytesRead += readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, propertyFlags, overwriteLocalData);

    }
    return bytesRead;
}

void EntityItem::debugDump() const {
    qDebug() << "EntityItem id:" << getEntityItemID();
    qDebug(" edited ago:%f", getEditedAgo());
    qDebug(" position:%f,%f,%f", _position.x, _position.y, _position.z);
    qDebug() << " dimensions:" << _dimensions;
}

// adjust any internal timestamps to fix clock skew for this server
void EntityItem::adjustEditPacketForClockSkew(unsigned char* editPacketBuffer, size_t length, int clockSkew) {
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

float EntityItem::getDistanceToBottomOfEntity() const {
    // TODO: change this to support registration point
    return _dimensions.y / 2.0f;
}

bool EntityItem::isRestingOnSurface() const { 
    // TODO: change this to support registration point
    return _position.y <= getDistanceToBottomOfEntity()
            && _velocity.y >= -EPSILON && _velocity.y <= EPSILON
            && _gravity.y < 0.0f;
}

void EntityItem::update(const quint64& updateTime) {
    bool wantDebug = false;

    float timeElapsed = (float)(updateTime - _lastUpdated) / (float)(USECS_PER_SECOND);

    if (wantDebug) {
        qDebug() << "********** EntityItem::update()";
        qDebug() << "    updateTime=" << updateTime;
        qDebug() << "    _lastUpdated=" << _lastUpdated;
        qDebug() << "    timeElapsed=" << timeElapsed;
    }

    _lastUpdated = updateTime;

    if (wantDebug) {
        qDebug() << "********** EntityItem::update() .... SETTING _lastUpdated=" << _lastUpdated;
    }

    if (hasVelocity() || hasGravity()) {
        glm::vec3 position = getPosition();
        glm::vec3 velocity = getVelocity();

        if (wantDebug) {        
            qDebug() << "EntityItem::update()....";
            qDebug() << "    timeElapsed:" << timeElapsed;
            qDebug() << "    old AACube:" << getAACube();
            qDebug() << "    old position:" << position;
            qDebug() << "    old velocity:" << velocity;
        }
        
        position += velocity * timeElapsed;

        // handle bounces off the ground... We bounce at the distance to the bottom of our entity
        if (position.y <= getDistanceToBottomOfEntity()) {
            velocity = velocity * glm::vec3(1,-1,1);

            // if we've slowed considerably, then just stop moving
            if (glm::length(velocity) <= EPSILON_VELOCITY_LENGTH) {
                velocity = NO_VELOCITY;
            }
            
            position.y = getDistanceToBottomOfEntity();
        }

        // handle gravity....
        if (hasGravity() && !isRestingOnSurface()) {
            velocity += getGravity() * timeElapsed;
        }

        // handle resting on surface case, this is definitely a bit of a hack, and it only works on the
        // "ground" plane of the domain, but for now it
        if (hasGravity() && isRestingOnSurface()) {
            velocity.y = 0.0f;
            position.y = getDistanceToBottomOfEntity();
        }

        // handle damping
        glm::vec3 dampingResistance = velocity * getDamping();
        if (wantDebug) {        
            qDebug() << "    getDamping():" << getDamping();
            qDebug() << "    dampingResistance:" << dampingResistance;
            qDebug() << "    dampingResistance * timeElapsed:" << dampingResistance * timeElapsed;
        }
        velocity -= dampingResistance * timeElapsed;

        if (wantDebug) {        
            qDebug() << "    velocity AFTER dampingResistance:" << velocity;
        }
        
        // round velocity to zero if it's close enough...
        if (glm::length(velocity) <= EPSILON_VELOCITY_LENGTH) {
            velocity = NO_VELOCITY;
        }
        
        if (wantDebug) {        
            qDebug() << "    new position:" << position;
            qDebug() << "    new velocity:" << velocity;
        }
        setPosition(position);
        setVelocity(velocity);
        if (wantDebug) {        
            qDebug() << "    new AACube:" << getAACube();
        }
    }
}

EntityItem::SimulationState EntityItem::getSimulationState() const {
    if (hasVelocity() || (hasGravity() && !isRestingOnSurface())) {
        return EntityItem::Moving;
    }
    if (isMortal()) {
        return EntityItem::Mortal;
    }
    return EntityItem::Static;
}

bool EntityItem::lifetimeHasExpired() const { 
    return isMortal() && (getAge() > getLifetime()); 
}


void EntityItem::copyChangedProperties(const EntityItem& other) {
    *this = other;
}

EntityItemProperties EntityItem::getProperties() const {
    EntityItemProperties properties;
    
    properties._id = getID();
    properties._idSet = true;
    properties._created = _created;

    properties._type = getType();
    
    properties._position = getPosition() * (float) TREE_SCALE;
    properties.setDimensions(getDimensions() * (float) TREE_SCALE);
    properties._rotation = getRotation();

    properties._mass = getMass();
    properties._velocity = getVelocity() * (float) TREE_SCALE;
    properties._gravity = getGravity() * (float) TREE_SCALE;
    properties._damping = getDamping();
    properties._lifetime = getLifetime();
    properties._script = getScript();

    properties._positionChanged = false;
    properties._rotationChanged = false;
    properties._massChanged = false;
    properties._velocityChanged = false;
    properties._gravityChanged = false;
    properties._dampingChanged = false;
    properties._lifetimeChanged = false;
    properties._scriptChanged = false;

    properties._defaultSettings = false;
    
    return properties;
}

bool EntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    bool somethingChanged = false;

    // handle the setting of created timestamps for the basic new entity case
    if (forceCopy) {
        if (properties.getCreated() == UNKNOWN_CREATED_TIME) {
            _created = usecTimestampNow();
        } else if (properties.getCreated() != USE_EXISTING_CREATED_TIME) {
            _created = properties.getCreated();
        }
    }

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(position, setPositionInMeters);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dimensions, setDimensionsInMeters); // NOTE: radius is obsolete
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotation, setRotation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(mass, setMass);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(velocity, setVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gravity, setGravity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(damping, setDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifetime, setLifetime);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(script, setScript);
    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(registrationPoint, setRegistrationPoint);
    //SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotationalVelocity, setRotationalVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glowLevel, setGlowLevel);

    if (somethingChanged) {
        somethingChangedNotification(); // notify derived classes that something has changed
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qDebug() << "EntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    
    return somethingChanged;
}

float EntityItem::getSize() const { 
    return glm::length(_dimensions);
}

// TODO: Add support for registration point
glm::vec3 EntityItem::getMinimumPoint() const { 
    // This assumes the registration point is in the center, we need to update this when we really support
    // registration point
    return _position - (_dimensions / 2.0f); 
}

// TODO: Add support for registration point
glm::vec3 EntityItem::getMaximumPoint() const { 
    // This assumes the registration point is in the center, we need to update this when we really support
    // registration point
    return _position + (_dimensions / 2.0f); 
}

// NOTE: This should only be used in cases of old bitstreams which only contain radius data
//    0,0,0 --> maxDimension,maxDimension,maxDimension
//    ... has a corner to corner distance of glm::length(maxDimension,maxDimension,maxDimension)
//    ... radius = cornerToCornerLength / 2.0f
//    ... radius * 2.0f = cornerToCornerLength
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius * 2.0f = sqrt(3 x maxDimension ^ 2)
//    ... (radius * 2.0f) ^2 = 3 x maxDimension ^ 2
//    ... ((radius * 2.0f) ^2) / 3 = maxDimension ^ 2
//    ... sqrt(((radius * 2.0f) ^2) / 3) = maxDimension
//    ... sqrt((diameter ^2) / 3) = maxDimension
//    
void EntityItem::setRadius(float value) { 
    float diameter = value * 2.0f;
    float maxDimension = sqrt((diameter * diameter) / 3.0f);
    _dimensions = glm::vec3(maxDimension, maxDimension, maxDimension);
}

// TODO: get rid of all users of this function...
//    ... radius = cornerToCornerLength / 2.0f
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius = sqrt(3 x maxDimension ^ 2) / 2.0f;
float EntityItem::getRadius() const { 
    float length = glm::length(_dimensions);
    float radius = length / 2.0f;
    return radius;
}


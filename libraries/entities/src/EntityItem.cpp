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
const float EntityItem::DEFAULT_LOCAL_RENDER_ALPHA = 1.0f;
const float EntityItem::DEFAULT_MASS = 1.0f;
const float EntityItem::DEFAULT_LIFETIME = EntityItem::IMMORTAL;
const float EntityItem::DEFAULT_DAMPING = 0.5f;
const glm::vec3 EntityItem::NO_VELOCITY = glm::vec3(0, 0, 0);
const float EntityItem::EPSILON_VELOCITY_LENGTH = (1.0f / 1000.0f) / (float)TREE_SCALE; // really small: 1mm/second
const glm::vec3 EntityItem::DEFAULT_VELOCITY = EntityItem::NO_VELOCITY;
const glm::vec3 EntityItem::NO_GRAVITY = glm::vec3(0, 0, 0);
const glm::vec3 EntityItem::REGULAR_GRAVITY = glm::vec3(0, (-9.8f / TREE_SCALE), 0);
const glm::vec3 EntityItem::DEFAULT_GRAVITY = EntityItem::NO_GRAVITY;
const QString EntityItem::DEFAULT_SCRIPT = QString("");
const glm::quat EntityItem::DEFAULT_ROTATION;
const glm::vec3 EntityItem::DEFAULT_DIMENSIONS = glm::vec3(0.1f, 0.1f, 0.1f);
const glm::vec3 EntityItem::DEFAULT_REGISTRATION_POINT = glm::vec3(0.5f, 0.5f, 0.5f); // center
const glm::vec3 EntityItem::NO_ANGULAR_VELOCITY = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 EntityItem::DEFAULT_ANGULAR_VELOCITY = NO_ANGULAR_VELOCITY;
const float EntityItem::DEFAULT_ANGULAR_DAMPING = 0.5f;
const bool EntityItem::DEFAULT_VISIBLE = true;
const bool EntityItem::DEFAULT_IGNORE_FOR_COLLISIONS = false;
const bool EntityItem::DEFAULT_COLLISIONS_WILL_MOVE = false;
const bool EntityItem::DEFAULT_LOCKED = false;

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
    _changedOnServer = 0;

    _position = glm::vec3(0,0,0);
    _rotation = DEFAULT_ROTATION;
    _dimensions = DEFAULT_DIMENSIONS;
    _glowLevel = DEFAULT_GLOW_LEVEL;
    _localRenderAlpha = DEFAULT_LOCAL_RENDER_ALPHA;
    _mass = DEFAULT_MASS;
    _velocity = DEFAULT_VELOCITY;
    _gravity = DEFAULT_GRAVITY;
    _damping = DEFAULT_DAMPING;
    _lifetime = DEFAULT_LIFETIME;
    _registrationPoint = DEFAULT_REGISTRATION_POINT;
    _angularVelocity = DEFAULT_ANGULAR_VELOCITY;
    _angularDamping = DEFAULT_ANGULAR_DAMPING;
    _visible = DEFAULT_VISIBLE;
    _ignoreForCollisions = DEFAULT_IGNORE_FOR_COLLISIONS;
    _collisionsWillMove = DEFAULT_COLLISIONS_WILL_MOVE;
    
    recalculateCollisionShape();
}

EntityItem::EntityItem(const EntityItemID& entityItemID) {
    _type = EntityTypes::Unknown;
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    _lastUpdated = 0;
    _created = 0;
    _changedOnServer = 0;
    initFromEntityItemID(entityItemID);
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) {
    _type = EntityTypes::Unknown;
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    _lastUpdated = 0;
    _created = properties.getCreated();
    _changedOnServer = 0;
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
    requestedProperties += PROP_REGISTRATION_POINT;
    requestedProperties += PROP_ANGULAR_VELOCITY;
    requestedProperties += PROP_ANGULAR_DAMPING;
    requestedProperties += PROP_VISIBLE;
    requestedProperties += PROP_IGNORE_FOR_COLLISIONS;
    requestedProperties += PROP_COLLISIONS_WILL_MOVE;
    requestedProperties += PROP_LOCKED;
    
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

        if (wantDebug) {
            qDebug() << "    APPEND_ENTITY_PROPERTY() PROP_DIMENSIONS:" << getDimensions();
        }

        APPEND_ENTITY_PROPERTY(PROP_ROTATION, appendValue, getRotation());
        APPEND_ENTITY_PROPERTY(PROP_MASS, appendValue, getMass());
        APPEND_ENTITY_PROPERTY(PROP_VELOCITY, appendValue, getVelocity());
        APPEND_ENTITY_PROPERTY(PROP_GRAVITY, appendValue, getGravity());
        APPEND_ENTITY_PROPERTY(PROP_DAMPING, appendValue, getDamping());
        APPEND_ENTITY_PROPERTY(PROP_LIFETIME, appendValue, getLifetime());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT, appendValue, getScript());
        APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, appendValue, getRegistrationPoint());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, appendValue, getAngularVelocity());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, appendValue, getAngularDamping());
        APPEND_ENTITY_PROPERTY(PROP_VISIBLE, appendValue, getVisible());
        APPEND_ENTITY_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, appendValue, getIgnoreForCollisions());
        APPEND_ENTITY_PROPERTY(PROP_COLLISIONS_WILL_MOVE, appendValue, getCollisionsWillMove());
        APPEND_ENTITY_PROPERTY(PROP_LOCKED, appendValue, getLocked());

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
                    setRadius(fromBuffer);
                }

                if (wantDebug) {
                    qDebug() << "    readEntityDataFromBuffer() OLD FORMAT... found PROP_RADIUS";
                }

            }
        } else {
            READ_ENTITY_PROPERTY(PROP_DIMENSIONS, glm::vec3, _dimensions);
            if (wantDebug) {
                qDebug() << "    readEntityDataFromBuffer() NEW FORMAT... look for PROP_DIMENSIONS";
            }
        }
        
        if (wantDebug) {
            qDebug() << "    readEntityDataFromBuffer() _dimensions:" << getDimensionsInMeters() << " in meters";
        }
                
        READ_ENTITY_PROPERTY_QUAT(PROP_ROTATION, _rotation);
        READ_ENTITY_PROPERTY(PROP_MASS, float, _mass);
        READ_ENTITY_PROPERTY(PROP_VELOCITY, glm::vec3, _velocity);
        qDebug() << "    readEntityDataFromBuffer() _velocity:" << _velocity << "line:" << __LINE__;
        READ_ENTITY_PROPERTY(PROP_GRAVITY, glm::vec3, _gravity);
        READ_ENTITY_PROPERTY(PROP_DAMPING, float, _damping);
        READ_ENTITY_PROPERTY(PROP_LIFETIME, float, _lifetime);
        READ_ENTITY_PROPERTY_STRING(PROP_SCRIPT,setScript);
        READ_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, glm::vec3, _registrationPoint);
        READ_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, glm::vec3, _angularVelocity);
        READ_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, float, _angularDamping);
        READ_ENTITY_PROPERTY(PROP_VISIBLE, bool, _visible);
        READ_ENTITY_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, bool, _ignoreForCollisions);
        READ_ENTITY_PROPERTY(PROP_COLLISIONS_WILL_MOVE, bool, _collisionsWillMove);
        READ_ENTITY_PROPERTY(PROP_LOCKED, bool, _locked);

        if (wantDebug) {
            qDebug() << "    readEntityDataFromBuffer() _registrationPoint:" << _registrationPoint;
            qDebug() << "    readEntityDataFromBuffer() _visible:" << _visible;
            qDebug() << "    readEntityDataFromBuffer() _ignoreForCollisions:" << _ignoreForCollisions;
            qDebug() << "    readEntityDataFromBuffer() _collisionsWillMove:" << _collisionsWillMove;
        }

        bytesRead += readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, propertyFlags, overwriteLocalData);

        recalculateCollisionShape();
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

// TODO: we probably want to change this to make "down" be the direction of the entity's gravity vector
//       for now, this is always true DOWN even if entity has non-down gravity.
// TODO: the old code had "&& _velocity.y >= -EPSILON && _velocity.y <= EPSILON" --- what was I thinking?
bool EntityItem::isRestingOnSurface() const { 
    glm::vec3 downwardVelocity = glm::vec3(0.0f, _velocity.y, 0.0f);

    return _position.y <= getDistanceToBottomOfEntity()
            && (glm::length(downwardVelocity) <= EPSILON_VELOCITY_LENGTH)
            && _gravity.y < 0.0f;
}

void EntityItem::update(const quint64& updateTime) {
    bool wantDebug = false;
    
    if (_lastUpdated == 0) {
        _lastUpdated = updateTime;
    }

    float timeElapsed = (float)(updateTime - _lastUpdated) / (float)(USECS_PER_SECOND);

    if (wantDebug) {
        qDebug() << "********** EntityItem::update()";
        qDebug() << "    entity ID=" << getEntityItemID();
        qDebug() << "    updateTime=" << updateTime;
        qDebug() << "    _lastUpdated=" << _lastUpdated;
        qDebug() << "    timeElapsed=" << timeElapsed;
        qDebug() << "    hasVelocity=" << hasVelocity();
        qDebug() << "    hasGravity=" << hasGravity();
        qDebug() << "    isRestingOnSurface=" << isRestingOnSurface();
        qDebug() << "    hasAngularVelocity=" << hasAngularVelocity();
        qDebug() << "    getAngularVelocity=" << getAngularVelocity();
        qDebug() << "    isMortal=" << isMortal();
        qDebug() << "    getAge()=" << getAge();
        qDebug() << "    getLifetime()=" << getLifetime();


        if (hasVelocity() || (hasGravity() && !isRestingOnSurface())) {
            qDebug() << "    MOVING...=";
            qDebug() << "        hasVelocity=" << hasVelocity();
            qDebug() << "        hasGravity=" << hasGravity();
            qDebug() << "        isRestingOnSurface=" << isRestingOnSurface();
            qDebug() << "        hasAngularVelocity=" << hasAngularVelocity();
            qDebug() << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (hasAngularVelocity()) {
            qDebug() << "    CHANGING...=";
            qDebug() << "        hasAngularVelocity=" << hasAngularVelocity();
            qDebug() << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (isMortal()) {
            qDebug() << "    MORTAL...=";
            qDebug() << "        isMortal=" << isMortal();
            qDebug() << "        getAge()=" << getAge();
            qDebug() << "        getLifetime()=" << getLifetime();
        }
    }

    _lastUpdated = updateTime;

    if (wantDebug) {
        qDebug() << "     ********** EntityItem::update() .... SETTING _lastUpdated=" << _lastUpdated;
    }

    if (hasAngularVelocity()) {
        glm::quat rotation = getRotation();
        glm::vec3 angularVelocity = glm::radians(getAngularVelocity());
        float angularSpeed = glm::length(angularVelocity);
        
        if (angularSpeed < EPSILON_VELOCITY_LENGTH) {
            setAngularVelocity(NO_ANGULAR_VELOCITY);
        } else {
            float angle = timeElapsed * angularSpeed;
            glm::quat  dQ = glm::angleAxis(angle, glm::normalize(angularVelocity));
            rotation = dQ * rotation;
            setRotation(rotation);

            // handle damping for angular velocity
            if (getAngularDamping() > 0.0f) {
                glm::vec3 dampingResistance = getAngularVelocity() * getAngularDamping();
                glm::vec3 newAngularVelocity = getAngularVelocity() - (dampingResistance * timeElapsed);
                setAngularVelocity(newAngularVelocity);
                if (wantDebug) {        
                    qDebug() << "    getDamping():" << getDamping();
                    qDebug() << "    dampingResistance:" << dampingResistance;
                    qDebug() << "    newAngularVelocity:" << newAngularVelocity;
                }
            }
        }
    }

    if (hasVelocity() || hasGravity()) {
        glm::vec3 position = getPosition();
        glm::vec3 velocity = getVelocity();
        glm::vec3 newPosition = position + (velocity * timeElapsed);

        if (wantDebug) {        
            qDebug() << "  EntityItem::update()....";
            qDebug() << "    timeElapsed:" << timeElapsed;
            qDebug() << "    old AACube:" << getMaximumAACube();
            qDebug() << "    old position:" << position;
            qDebug() << "    old velocity:" << velocity;
            qDebug() << "    old getAABox:" << getAABox();
            qDebug() << "    getDistanceToBottomOfEntity():" << getDistanceToBottomOfEntity() * (float)TREE_SCALE << " in meters";
            qDebug() << "    newPosition:" << newPosition;
            qDebug() << "    glm::distance(newPosition, position):" << glm::distance(newPosition, position);
        }
        
        position = newPosition;

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

        // handle damping for velocity
        glm::vec3 dampingResistance = velocity * getDamping();
        if (wantDebug) {        
            qDebug() << "    getDamping():" << getDamping();
            qDebug() << "    dampingResistance:" << dampingResistance;
            qDebug() << "    dampingResistance * timeElapsed:" << dampingResistance * timeElapsed;
        }
        velocity -= dampingResistance * timeElapsed;

        if (wantDebug) {        
            qDebug() << "    velocity AFTER dampingResistance:" << velocity;
            qDebug() << "    glm::length(velocity):" << glm::length(velocity);
            qDebug() << "    EPSILON_VELOCITY_LENGTH:" << EPSILON_VELOCITY_LENGTH;
        }
        
        // round velocity to zero if it's close enough...
        if (glm::length(velocity) <= EPSILON_VELOCITY_LENGTH) {
            velocity = NO_VELOCITY;
        }

        setPosition(position); // this will automatically recalculate our collision shape
        setVelocity(velocity);
        
        if (wantDebug) {        
            qDebug() << "    new position:" << position;
            qDebug() << "    new velocity:" << velocity;
            qDebug() << "    new AACube:" << getMaximumAACube();
            qDebug() << "    old getAABox:" << getAABox();
        }
    }
}

EntityItem::SimulationState EntityItem::getSimulationState() const {
    if (hasVelocity() || (hasGravity() && !isRestingOnSurface())) {
        return EntityItem::Moving;
    }
    if (hasAngularVelocity()) {
        return EntityItem::Changing;
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
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(position, getPositionInMeters);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dimensions, getDimensionsInMeters); // NOTE: radius is obsolete
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(rotation, getRotation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(mass, getMass);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(velocity, getVelocityInMeters);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(gravity, getGravityInMeters);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(damping, getDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lifetime, getLifetime);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(script, getScript);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(registrationPoint, getRegistrationPoint);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularVelocity, getAngularVelocity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularDamping, getAngularDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevel);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localRenderAlpha, getLocalRenderAlpha);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(visible, getVisible);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ignoreForCollisions, getIgnoreForCollisions);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(collisionsWillMove, getCollisionsWillMove);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(locked, getLocked);

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

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(position, setPositionInMeters); // this will call recalculate collision shape if needed
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dimensions, setDimensionsInMeters); // NOTE: radius is obsolete
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotation, setRotation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(mass, setMass);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(velocity, setVelocityInMeters);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gravity, setGravityInMeters);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(damping, setDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifetime, setLifetime);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(script, setScript);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(registrationPoint, setRegistrationPoint);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularVelocity, setAngularVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularDamping, setAngularDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glowLevel, setGlowLevel);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localRenderAlpha, setLocalRenderAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(visible, setVisible);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ignoreForCollisions, setIgnoreForCollisions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionsWillMove, setCollisionsWillMove);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(locked, setLocked);

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


// TODO: is this really correct? how do we use size, does it need to handle rotation?
float EntityItem::getSize() const { 
    return glm::length(_dimensions);
}

float EntityItem::getDistanceToBottomOfEntity() const {
    glm::vec3 minimumPoint = getAABox().getMinimumPoint();
    return getPosition().y - minimumPoint.y;
}

// TODO: doesn't this need to handle rotation?
glm::vec3 EntityItem::getCenter() const {
    return _position + (_dimensions * (glm::vec3(0.5f,0.5f,0.5f) - _registrationPoint));
}

/// The maximum bounding cube for the entity, independent of it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
/// 
AACube EntityItem::getMaximumAACube() const { 
    // * we know that the position is the center of rotation
    glm::vec3 centerOfRotation = _position; // also where _registration point is

    // * we know that the registration point is the center of rotation
    // * we can calculate the length of the furthest extent from the registration point
    //   as the dimensions * max (registrationPoint, (1.0,1.0,1.0) - registrationPoint)
    glm::vec3 registrationPoint = (_dimensions * _registrationPoint);
    glm::vec3 registrationRemainder = (_dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint));
    glm::vec3 furthestExtentFromRegistration = glm::max(registrationPoint, registrationRemainder);
    
    // * we know that if you rotate in any direction you would create a sphere
    //   that has a radius of the length of furthest extent from registration point
    float radius = glm::length(furthestExtentFromRegistration);

    // * we know that the minimum bounding cube of this maximum possible sphere is
    //   (center - radius) to (center + radius)
    glm::vec3 minimumCorner = centerOfRotation - glm::vec3(radius, radius, radius);

    AACube boundingCube(minimumCorner, radius * 2.0f);
    return boundingCube;
}

/// The minimum bounding cube for the entity accounting for it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
/// 
AACube EntityItem::getMinimumAACube() const { 
    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;

    glm::vec3 unrotatedMinRelativeToEntity = glm::vec3(0.0f, 0.0f, 0.0f) - (_dimensions * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());

    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(_position);
    
    // the cube that best encompasses extents is...
    AABox box(rotatedExtentsRelativeToRegistrationPoint);
    glm::vec3 centerOfBox = box.calcCenter();
    float longestSide = box.getLargestDimension();
    float halfLongestSide = longestSide / 2.0f;
    glm::vec3 cornerOfCube = centerOfBox - glm::vec3(halfLongestSide, halfLongestSide, halfLongestSide);
    
    
    // old implementation... not correct!!!
    return AACube(cornerOfCube, longestSide);
}

AABox EntityItem::getAABox() const { 

    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;
    
    glm::vec3 unrotatedMinRelativeToEntity = glm::vec3(0.0f, 0.0f, 0.0f) - (_dimensions * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());
    
    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(_position);
    
    return AABox(rotatedExtentsRelativeToRegistrationPoint);
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

    bool wantDebug = false;    
    if (wantDebug) {
        qDebug() << "EntityItem::setRadius()...";
        qDebug() << "    radius:" << value;
        qDebug() << "    diameter:" << diameter;
        qDebug() << "    maxDimension:" << maxDimension;
        qDebug() << "    _dimensions:" << _dimensions;
    }
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

void EntityItem::recalculateCollisionShape() {
    AACube entityAACube = getMinimumAACube();
    entityAACube.scale(TREE_SCALE); // scale to meters
    _collisionShape.setTranslation(entityAACube.calcCenter());
    _collisionShape.setScale(entityAACube.getScale());
}



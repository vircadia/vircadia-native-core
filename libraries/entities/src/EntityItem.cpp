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

#include <glm/gtx/transform.hpp>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <Octree.h>
#include <PhysicsHelpers.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()

#include "EntityScriptingInterface.h"
#include "EntityItem.h"
#include "EntitiesLogging.h"
#include "EntityTree.h"

bool EntityItem::_sendPhysicsUpdates = true;

EntityItem::EntityItem(const EntityItemID& entityItemID) :
    _type(EntityTypes::Unknown),
    _id(entityItemID.id),
    _creatorTokenID(entityItemID.creatorTokenID),
    _newlyCreated(false),
    _lastSimulated(0),
    _lastUpdated(0),
    _lastEdited(0),
    _lastEditedFromRemote(0),
    _lastEditedFromRemoteInRemoteTime(0),
    _created(UNKNOWN_CREATED_TIME),
    _changedOnServer(0),
    _position(ENTITY_ITEM_ZERO_VEC3),
    _dimensions(ENTITY_ITEM_DEFAULT_DIMENSIONS),
    _rotation(ENTITY_ITEM_DEFAULT_ROTATION),
    _glowLevel(ENTITY_ITEM_DEFAULT_GLOW_LEVEL),
    _localRenderAlpha(ENTITY_ITEM_DEFAULT_LOCAL_RENDER_ALPHA),
    _density(ENTITY_ITEM_DEFAULT_DENSITY),
    _volumeMultiplier(1.0f),
    _velocity(ENTITY_ITEM_DEFAULT_VELOCITY),
    _gravity(ENTITY_ITEM_DEFAULT_GRAVITY),
    _acceleration(ENTITY_ITEM_DEFAULT_ACCELERATION),
    _damping(ENTITY_ITEM_DEFAULT_DAMPING),
    _lifetime(ENTITY_ITEM_DEFAULT_LIFETIME),
    _script(ENTITY_ITEM_DEFAULT_SCRIPT),
    _registrationPoint(ENTITY_ITEM_DEFAULT_REGISTRATION_POINT),
    _angularVelocity(ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY),
    _angularDamping(ENTITY_ITEM_DEFAULT_ANGULAR_DAMPING),
    _visible(ENTITY_ITEM_DEFAULT_VISIBLE),
    _ignoreForCollisions(ENTITY_ITEM_DEFAULT_IGNORE_FOR_COLLISIONS),
    _collisionsWillMove(ENTITY_ITEM_DEFAULT_COLLISIONS_WILL_MOVE),
    _locked(ENTITY_ITEM_DEFAULT_LOCKED),
    _userData(ENTITY_ITEM_DEFAULT_USER_DATA),
    _simulatorID(ENTITY_ITEM_DEFAULT_SIMULATOR_ID),
    _simulatorIDChangedTime(0),
    _marketplaceID(ENTITY_ITEM_DEFAULT_MARKETPLACE_ID),
    _name(ENTITY_ITEM_DEFAULT_NAME),
    _physicsInfo(NULL),
    _dirtyFlags(0),
    _element(NULL)
{
    quint64 now = usecTimestampNow();
    _lastSimulated = now;
    _lastUpdated = now;
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) : EntityItem(entityItemID)
{
    setProperties(properties);
}

EntityItem::~EntityItem() {
    // be sure to clean up _physicsInfo before calling this dtor
    assert(_physicsInfo == NULL);
    assert(_element == NULL);
}

EntityPropertyFlags EntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_POSITION;
    requestedProperties += PROP_DIMENSIONS; // NOTE: PROP_RADIUS obsolete
    requestedProperties += PROP_ROTATION;
    requestedProperties += PROP_DENSITY;
    requestedProperties += PROP_VELOCITY;
    requestedProperties += PROP_GRAVITY;
    requestedProperties += PROP_ACCELERATION;
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
    requestedProperties += PROP_USER_DATA;
    requestedProperties += PROP_MARKETPLACE_ID;
    requestedProperties += PROP_NAME;
    requestedProperties += PROP_SIMULATOR_ID;
    
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

    // last updated (animations, non-physics changes)
    quint64 updateDelta = getLastUpdated() <= getLastEdited() ? 0 : getLastUpdated() - getLastEdited();
    ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
    QByteArray encodedUpdateDelta = updateDeltaCoder;

    // last simulated (velocity, angular velocity, physics changes)
    quint64 simulatedDelta = getLastSimulated() <= getLastEdited() ? 0 : getLastSimulated() - getLastEdited();
    ByteCountCoded<quint64> simulatedDeltaCoder = simulatedDelta;
    QByteArray encodedSimulatedDelta = simulatedDeltaCoder;


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

    #ifdef WANT_DEBUG
        float editedAgo = getEditedAgo();
        QString agoAsString = formatSecondsElapsed(editedAgo);
        qCDebug(entities) << "Writing entity " << getEntityItemID() << " to buffer, lastEdited =" << lastEdited 
                        << " ago=" << editedAgo << "seconds - " << agoAsString;
    #endif

    bool successIDFits = false;
    bool successTypeFits = false;
    bool successCreatedFits = false;
    bool successLastEditedFits = false;
    bool successLastUpdatedFits = false;
    bool successLastSimulatedFits = false;
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
        successLastSimulatedFits = packetData->appendValue(encodedSimulatedDelta);
    }
    
    if (successLastSimulatedFits) {
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
        APPEND_ENTITY_PROPERTY(PROP_DENSITY, appendValue, getDensity());
        APPEND_ENTITY_PROPERTY(PROP_VELOCITY, appendValue, getVelocity());
        APPEND_ENTITY_PROPERTY(PROP_GRAVITY, appendValue, getGravity());
        APPEND_ENTITY_PROPERTY(PROP_ACCELERATION, appendValue, getAcceleration());
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
        APPEND_ENTITY_PROPERTY(PROP_USER_DATA, appendValue, getUserData());
        APPEND_ENTITY_PROPERTY(PROP_SIMULATOR_ID, appendValue, getSimulatorID());
        APPEND_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, appendValue, getMarketplaceID());
        APPEND_ENTITY_PROPERTY(PROP_NAME, appendValue, getName());

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

    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
    
        // NOTE: This shouldn't happen. The only versions of the bit stream that didn't support split mtu buffers should
        // be handled by the model subclass and shouldn't call this routine.
        qCDebug(entities) << "EntityItem::readEntityDataFromBuffer()... "
                        "ERROR CASE...args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU";
        return 0;
    }

    // if this bitstream indicates that this node is the simulation owner, ignore any physics-related updates.
    glm::vec3 savePosition = _position;
    glm::quat saveRotation = _rotation;
    glm::vec3 saveVelocity = _velocity;
    glm::vec3 saveAngularVelocity = _angularVelocity;
    glm::vec3 saveGravity = _gravity;
    glm::vec3 saveAcceleration = _acceleration;


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

        quint64 now = usecTimestampNow();
        if (_created == UNKNOWN_CREATED_TIME) {
            // we don't yet have a _created timestamp, so we accept this one
            createdFromBuffer -= clockSkew;
            if (createdFromBuffer > now || createdFromBuffer == UNKNOWN_CREATED_TIME) {
                createdFromBuffer = now;
            }
            _created = createdFromBuffer;
        }

        #ifdef WANT_DEBUG
            quint64 lastEdited = getLastEdited();
            float editedAgo = getEditedAgo();
            QString agoAsString = formatSecondsElapsed(editedAgo);
            QString ageAsString = formatSecondsElapsed(getAge());
            qCDebug(entities) << "------------------------------------------";
            qCDebug(entities) << "Loading entity " << getEntityItemID() << " from buffer...";
            qCDebug(entities) << "------------------------------------------";
            debugDump();
            qCDebug(entities) << "------------------------------------------";
            qCDebug(entities) << "    _created =" << _created;
            qCDebug(entities) << "    age=" << getAge() << "seconds - " << ageAsString;
            qCDebug(entities) << "    lastEdited =" << lastEdited;
            qCDebug(entities) << "    ago=" << editedAgo << "seconds - " << agoAsString;
        #endif
        
        quint64 lastEditedFromBuffer = 0;
        quint64 lastEditedFromBufferAdjusted = 0;

        // TODO: we could make this encoded as a delta from _created
        // _lastEdited
        memcpy(&lastEditedFromBuffer, dataAt, sizeof(lastEditedFromBuffer));
        dataAt += sizeof(lastEditedFromBuffer);
        bytesRead += sizeof(lastEditedFromBuffer);
        lastEditedFromBufferAdjusted = lastEditedFromBuffer - clockSkew;
        if (lastEditedFromBufferAdjusted > now) {
            lastEditedFromBufferAdjusted = now;
        }
        
        bool fromSameServerEdit = (lastEditedFromBuffer == _lastEditedFromRemoteInRemoteTime);

        #ifdef WANT_DEBUG
            qCDebug(entities) << "data from server **************** ";
            qCDebug(entities) << "                           entityItemID:" << getEntityItemID();
            qCDebug(entities) << "                                    now:" << now;
            qCDebug(entities) << "                          getLastEdited:" << debugTime(getLastEdited(), now);
            qCDebug(entities) << "                   lastEditedFromBuffer:" << debugTime(lastEditedFromBuffer, now);
            qCDebug(entities) << "                              clockSkew:" << debugTimeOnly(clockSkew);
            qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
            qCDebug(entities) << "                  _lastEditedFromRemote:" << debugTime(_lastEditedFromRemote, now);
            qCDebug(entities) << "      _lastEditedFromRemoteInRemoteTime:" << debugTime(_lastEditedFromRemoteInRemoteTime, now);
            qCDebug(entities) << "                     fromSameServerEdit:" << fromSameServerEdit;
        #endif

        bool ignoreServerPacket = false; // assume we'll use this server packet

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
            #ifdef WANT_DEBUG
                qCDebug(entities) << "IGNORING old data from server!!! ****************";
                debugDump();
            #endif
        } else {

            #ifdef WANT_DEBUG
                qCDebug(entities) << "USING NEW data from server!!! ****************";
                debugDump();
            #endif

            // don't allow _lastEdited to be in the future
            _lastEdited = lastEditedFromBufferAdjusted;
            _lastEditedFromRemote = now;
            _lastEditedFromRemoteInRemoteTime = lastEditedFromBuffer;
            
            // TODO: only send this notification if something ACTUALLY changed (hint, we haven't yet parsed 
            // the properties out of the bitstream (see below))
            somethingChangedNotification(); // notify derived classes that something has changed
        }

        // last updated is stored as ByteCountCoded delta from lastEdited
        QByteArray encodedUpdateDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
        quint64 updateDelta = updateDeltaCoder;
        if (overwriteLocalData) {
            _lastUpdated = lastEditedFromBufferAdjusted + updateDelta; // don't adjust for clock skew since we already did that
            #ifdef WANT_DEBUG
                qCDebug(entities) << "                           _lastUpdated:" << debugTime(_lastUpdated, now);
                qCDebug(entities) << "                            _lastEdited:" << debugTime(_lastEdited, now);
                qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
            #endif
        }
        encodedUpdateDelta = updateDeltaCoder; // determine true length
        dataAt += encodedUpdateDelta.size();
        bytesRead += encodedUpdateDelta.size();
        
        // Newer bitstreams will have a last simulated and a last updated value
        if (args.bitstreamVersion >= VERSION_ENTITIES_HAS_LAST_SIMULATED_TIME) {
            // last simulated is stored as ByteCountCoded delta from lastEdited
            QByteArray encodedSimulatedDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
            ByteCountCoded<quint64> simulatedDeltaCoder = encodedSimulatedDelta;
            quint64 simulatedDelta = simulatedDeltaCoder;
            if (overwriteLocalData) {
                _lastSimulated = lastEditedFromBufferAdjusted + simulatedDelta; // don't adjust for clock skew since we already did that
                #ifdef WANT_DEBUG
                    qCDebug(entities) << "                         _lastSimulated:" << debugTime(_lastSimulated, now);
                    qCDebug(entities) << "                            _lastEdited:" << debugTime(_lastEdited, now);
                    qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
                #endif
            }
            encodedSimulatedDelta = simulatedDeltaCoder; // determine true length
            dataAt += encodedSimulatedDelta.size();
            bytesRead += encodedSimulatedDelta.size();
        }
        
        #ifdef WANT_DEBUG
            if (overwriteLocalData) {
                qCDebug(entities) << "EntityItem::readEntityDataFromBuffer()... changed entity:" << getEntityItemID();
                qCDebug(entities) << "                          getLastEdited:" << debugTime(getLastEdited(), now);
                qCDebug(entities) << "                       getLastSimulated:" << debugTime(getLastSimulated(), now);
                qCDebug(entities) << "                         getLastUpdated:" << debugTime(getLastUpdated(), now);
            }
        #endif
        

        // Property Flags
        QByteArray encodedPropertyFlags = originalDataBuffer.mid(bytesRead); // maximum possible size
        EntityPropertyFlags propertyFlags = encodedPropertyFlags;
        dataAt += propertyFlags.getEncodedLength();
        bytesRead += propertyFlags.getEncodedLength();
        bool useMeters = (args.bitstreamVersion >= VERSION_ENTITIES_USE_METERS_AND_RADIANS);
        if (useMeters) {
            READ_ENTITY_PROPERTY_SETTER(PROP_POSITION, glm::vec3, updatePosition);
        } else {
            READ_ENTITY_PROPERTY_SETTER(PROP_POSITION, glm::vec3, updatePositionInDomainUnits);
        }

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
            }
        } else {
            if (useMeters) {
                READ_ENTITY_PROPERTY_SETTER(PROP_DIMENSIONS, glm::vec3, updateDimensions);
            } else {
                READ_ENTITY_PROPERTY_SETTER(PROP_DIMENSIONS, glm::vec3, updateDimensionsInDomainUnits);
            }
        }

        READ_ENTITY_PROPERTY_QUAT_SETTER(PROP_ROTATION, updateRotation);
        READ_ENTITY_PROPERTY_SETTER(PROP_DENSITY, float, updateDensity);
        if (useMeters) {
            READ_ENTITY_PROPERTY_SETTER(PROP_VELOCITY, glm::vec3, updateVelocity);
            READ_ENTITY_PROPERTY_SETTER(PROP_GRAVITY, glm::vec3, updateGravity);
        } else {
            READ_ENTITY_PROPERTY_SETTER(PROP_VELOCITY, glm::vec3, updateVelocityInDomainUnits);
            READ_ENTITY_PROPERTY_SETTER(PROP_GRAVITY, glm::vec3, updateGravityInDomainUnits);
        }
        if (args.bitstreamVersion >= VERSION_ENTITIES_HAVE_ACCELERATION) {
            READ_ENTITY_PROPERTY_SETTER(PROP_ACCELERATION, glm::vec3, updateAcceleration);
        }

        READ_ENTITY_PROPERTY(PROP_DAMPING, float, _damping);
        READ_ENTITY_PROPERTY_SETTER(PROP_LIFETIME, float, updateLifetime);
        READ_ENTITY_PROPERTY_STRING(PROP_SCRIPT, setScript);
        READ_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, glm::vec3, _registrationPoint);
        if (useMeters) {
            READ_ENTITY_PROPERTY_SETTER(PROP_ANGULAR_VELOCITY, glm::vec3, updateAngularVelocity);
        } else {
            READ_ENTITY_PROPERTY_SETTER(PROP_ANGULAR_VELOCITY, glm::vec3, updateAngularVelocityInDegrees);
        }
        READ_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, float, _angularDamping);
        READ_ENTITY_PROPERTY(PROP_VISIBLE, bool, _visible);
        READ_ENTITY_PROPERTY_SETTER(PROP_IGNORE_FOR_COLLISIONS, bool, updateIgnoreForCollisions);
        READ_ENTITY_PROPERTY_SETTER(PROP_COLLISIONS_WILL_MOVE, bool, updateCollisionsWillMove);
        READ_ENTITY_PROPERTY(PROP_LOCKED, bool, _locked);
        READ_ENTITY_PROPERTY_STRING(PROP_USER_DATA, setUserData);

        if (args.bitstreamVersion >= VERSION_ENTITIES_HAVE_ACCELERATION) {
            READ_ENTITY_PROPERTY_UUID(PROP_SIMULATOR_ID, setSimulatorID);
        }

        if (args.bitstreamVersion >= VERSION_ENTITIES_HAS_MARKETPLACE_ID) {
            READ_ENTITY_PROPERTY_STRING(PROP_MARKETPLACE_ID, setMarketplaceID);
        }

        READ_ENTITY_PROPERTY_STRING(PROP_NAME, setName);

        bytesRead += readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, propertyFlags, overwriteLocalData);

        ////////////////////////////////////
        // WARNING: Do not add stream content here after the subclass. Always add it before the subclass
        //
        // NOTE: we had a bad version of the stream that we added stream data after the subclass. We can attempt to recover 
        // by doing this parsing here... but it's not likely going to fully recover the content.
        //
        // TODO: Remove this conde once we've sufficiently migrated content past this damaged version
        if (args.bitstreamVersion == VERSION_ENTITIES_HAS_MARKETPLACE_ID_DAMAGED) {
            READ_ENTITY_PROPERTY_STRING(PROP_MARKETPLACE_ID, setMarketplaceID);
        }

        if (overwriteLocalData && (getDirtyFlags() & (EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY))) {
            // NOTE: This code is attempting to "repair" the old data we just got from the server to make it more
            // closely match where the entities should be if they'd stepped forward in time to "now". The server
            // is sending us data with a known "last simulated" time. That time is likely in the past, and therefore
            // this "new" data is actually slightly out of date. We calculate the time we need to skip forward and
            // use our simulation helper routine to get a best estimate of where the entity should be.
            const float MIN_TIME_SKIP = 0.0f;
            const float MAX_TIME_SKIP = 1.0f; // in seconds
            float skipTimeForward = glm::clamp((float)(now - _lastSimulated) / (float)(USECS_PER_SECOND), 
                                        MIN_TIME_SKIP, MAX_TIME_SKIP);
            if (skipTimeForward > 0.0f) {
                #ifdef WANT_DEBUG
                    qCDebug(entities) << "skipTimeForward:" << skipTimeForward;
                #endif

                // we want to extrapolate the motion forward to compensate for packet travel time, but
                // we don't want the side effect of flag setting.
                simulateKinematicMotion(skipTimeForward, false);
            }
            _lastSimulated = now;
        }
    }


    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid& myNodeID = nodeList->getSessionUUID();
    if (_simulatorID == myNodeID && !_simulatorID.isNull()) {
        // the packet that produced this bitstream originally came from physics simulations performed by
        // this node, so our version has to be newer than what the packet contained.
        _position = savePosition;
        _rotation = saveRotation;
        _velocity = saveVelocity;
        _angularVelocity = saveAngularVelocity;
        _gravity = saveGravity;
        _acceleration = saveAcceleration;
    }

    return bytesRead;
}

void EntityItem::debugDump() const {
    qCDebug(entities) << "EntityItem id:" << getEntityItemID();
    qCDebug(entities, " edited ago:%f", getEditedAgo());
    qCDebug(entities, " position:%f,%f,%f", _position.x, _position.y, _position.z);
    qCDebug(entities) << " dimensions:" << _dimensions;
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
    #ifdef WANT_DEBUG
        qCDebug(entities, "EntityItem::adjustEditPacketForClockSkew()...");
        qCDebug(entities) << "     lastEditedInLocalTime: " << lastEditedInLocalTime;
        qCDebug(entities) << "                 clockSkew: " << clockSkew;
        qCDebug(entities) << "    lastEditedInServerTime: " << lastEditedInServerTime;
    #endif
}

float EntityItem::computeMass() const { 
    return _density * _volumeMultiplier * _dimensions.x * _dimensions.y * _dimensions.z;
}

void EntityItem::setDensity(float density) {
    _density = glm::max(glm::min(density, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
}

void EntityItem::updateDensity(float density) {
    const float MIN_DENSITY_CHANGE_FACTOR = 0.001f; // 0.1 percent
    float newDensity = glm::max(glm::min(density, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    if (fabsf(_density - newDensity) / _density > MIN_DENSITY_CHANGE_FACTOR) {
        _density = newDensity;
        _dirtyFlags |= EntityItem::DIRTY_MASS;
    }
}

void EntityItem::setMass(float mass) {
    // Setting the mass actually changes the _density (at fixed volume), however
    // we must protect the density range to help maintain stability of physics simulation
    // therefore this method might not accept the mass that is supplied.

    float volume = _volumeMultiplier * _dimensions.x * _dimensions.y * _dimensions.z;

    // compute new density
    const float MIN_VOLUME = 1.0e-6f; // 0.001mm^3
    if (volume < 1.0e-6f) {
        // avoid divide by zero
        _density = glm::min(mass / MIN_VOLUME, ENTITY_ITEM_MAX_DENSITY);
    } else {
        _density = glm::max(glm::min(mass / volume, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    }
}

const float DEFAULT_ENTITY_RESTITUTION = 0.5f;
const float DEFAULT_ENTITY_FRICTION = 0.5f;                                                                                

float EntityItem::getRestitution() const { 
    return DEFAULT_ENTITY_RESTITUTION; 
}

float EntityItem::getFriction() const { 
    return DEFAULT_ENTITY_FRICTION;
}

void EntityItem::simulate(const quint64& now) {
    if (_lastSimulated == 0) {
        _lastSimulated = now;
    }

    float timeElapsed = (float)(now - _lastSimulated) / (float)(USECS_PER_SECOND);

    #ifdef WANT_DEBUG
        qCDebug(entities) << "********** EntityItem::simulate()";
        qCDebug(entities) << "    entity ID=" << getEntityItemID();
        qCDebug(entities) << "    simulator ID=" << getSimulatorID();
        qCDebug(entities) << "    now=" << now;
        qCDebug(entities) << "    _lastSimulated=" << _lastSimulated;
        qCDebug(entities) << "    timeElapsed=" << timeElapsed;
        qCDebug(entities) << "    hasVelocity=" << hasVelocity();
        qCDebug(entities) << "    hasGravity=" << hasGravity();
        qCDebug(entities) << "    hasAcceleration=" << hasAcceleration();
        qCDebug(entities) << "    hasAngularVelocity=" << hasAngularVelocity();
        qCDebug(entities) << "    getAngularVelocity=" << getAngularVelocity();
        qCDebug(entities) << "    isMortal=" << isMortal();
        qCDebug(entities) << "    getAge()=" << getAge();
        qCDebug(entities) << "    getLifetime()=" << getLifetime();


        if (hasVelocity() || hasGravity()) {
            qCDebug(entities) << "    MOVING...=";
            qCDebug(entities) << "        hasVelocity=" << hasVelocity();
            qCDebug(entities) << "        hasGravity=" << hasGravity();
            qCDebug(entities) << "        hasAcceleration=" << hasAcceleration();
            qCDebug(entities) << "        hasAngularVelocity=" << hasAngularVelocity();
            qCDebug(entities) << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (hasAngularVelocity()) {
            qCDebug(entities) << "    CHANGING...=";
            qCDebug(entities) << "        hasAngularVelocity=" << hasAngularVelocity();
            qCDebug(entities) << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (isMortal()) {
            qCDebug(entities) << "    MORTAL...=";
            qCDebug(entities) << "        isMortal=" << isMortal();
            qCDebug(entities) << "        getAge()=" << getAge();
            qCDebug(entities) << "        getLifetime()=" << getLifetime();
        }
        qCDebug(entities) << "     ********** EntityItem::simulate() .... SETTING _lastSimulated=" << _lastSimulated;
    #endif

    simulateKinematicMotion(timeElapsed);
    _lastSimulated = now;
}

void EntityItem::simulateKinematicMotion(float timeElapsed, bool setFlags) {
    if (hasAngularVelocity()) {
        // angular damping
        if (_angularDamping > 0.0f) {
            _angularVelocity *= powf(1.0f - _angularDamping, timeElapsed);
            #ifdef WANT_DEBUG
                qCDebug(entities) << "    angularDamping :" << _angularDamping;
                qCDebug(entities) << "    newAngularVelocity:" << _angularVelocity;
            #endif
        }

        float angularSpeed = glm::length(_angularVelocity);
        
        const float EPSILON_ANGULAR_VELOCITY_LENGTH = 0.0017453f; // 0.0017453 rad/sec = 0.1f degrees/sec
        if (angularSpeed < EPSILON_ANGULAR_VELOCITY_LENGTH) {
            if (setFlags && angularSpeed > 0.0f) {
                _dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE;
            }
            _angularVelocity = ENTITY_ITEM_ZERO_VEC3;
        } else {
            // for improved agreement with the way Bullet integrates rotations we use an approximation 
            // and break the integration into bullet-sized substeps 
            glm::quat rotation = getRotation();
            float dt = timeElapsed;
            while (dt > PHYSICS_ENGINE_FIXED_SUBSTEP) {
                glm::quat  dQ = computeBulletRotationStep(_angularVelocity, PHYSICS_ENGINE_FIXED_SUBSTEP);
                rotation = glm::normalize(dQ * rotation);
                dt -= PHYSICS_ENGINE_FIXED_SUBSTEP;
            }
            // NOTE: this final partial substep can drift away from a real Bullet simulation however 
            // it only becomes significant for rapidly rotating objects
            // (e.g. around PI/4 radians per substep, or 7.5 rotations/sec at 60 substeps/sec).
            glm::quat  dQ = computeBulletRotationStep(_angularVelocity, dt);
            rotation = glm::normalize(dQ * rotation);

            setRotation(rotation);
        }
    }

    if (hasVelocity()) {
        // linear damping
        glm::vec3 velocity = getVelocity();
        if (_damping > 0.0f) {
            velocity *= powf(1.0f - _damping, timeElapsed);
            #ifdef WANT_DEBUG
                qCDebug(entities) << "    damping:" << _damping;
                qCDebug(entities) << "    velocity AFTER dampingResistance:" << velocity;
                qCDebug(entities) << "    glm::length(velocity):" << glm::length(velocity);
            #endif
        }

        // integrate position forward
        glm::vec3 position = getPosition();
        glm::vec3 newPosition = position + (velocity * timeElapsed);

        #ifdef WANT_DEBUG
            qCDebug(entities) << "  EntityItem::simulate()....";
            qCDebug(entities) << "    timeElapsed:" << timeElapsed;
            qCDebug(entities) << "    old AACube:" << getMaximumAACube();
            qCDebug(entities) << "    old position:" << position;
            qCDebug(entities) << "    old velocity:" << velocity;
            qCDebug(entities) << "    old getAABox:" << getAABox();
            qCDebug(entities) << "    newPosition:" << newPosition;
            qCDebug(entities) << "    glm::distance(newPosition, position):" << glm::distance(newPosition, position);
        #endif
        
        position = newPosition;

        // apply effective acceleration, which will be the same as gravity if the Entity isn't at rest.
        if (hasAcceleration()) {
            velocity += getAcceleration() * timeElapsed;
        }

        float speed = glm::length(velocity);
        const float EPSILON_LINEAR_VELOCITY_LENGTH = 0.001f; // 1mm/sec
        if (speed < EPSILON_LINEAR_VELOCITY_LENGTH) {
            setVelocity(ENTITY_ITEM_ZERO_VEC3);
            if (setFlags && speed > 0.0f) {
                _dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE;
            }
        } else {
            setPosition(position);
            setVelocity(velocity);
        }

        #ifdef WANT_DEBUG
            qCDebug(entities) << "    new position:" << position;
            qCDebug(entities) << "    new velocity:" << velocity;
            qCDebug(entities) << "    new AACube:" << getMaximumAACube();
            qCDebug(entities) << "    old getAABox:" << getAABox();
        #endif
    }
}

bool EntityItem::isMoving() const {
    return hasVelocity() || hasAngularVelocity();
}

glm::mat4 EntityItem::getEntityToWorldMatrix() const {
    glm::mat4 translation = glm::translate(getPosition());
    glm::mat4 rotation = glm::mat4_cast(getRotation());
    glm::mat4 scale = glm::scale(getDimensions());
    glm::mat4 registration = glm::translate(ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint());
    return translation * rotation * scale * registration;
}

glm::mat4 EntityItem::getWorldToEntityMatrix() const {
    return glm::inverse(getEntityToWorldMatrix());
}

glm::vec3 EntityItem::entityToWorld(const glm::vec3& point) const {
    return glm::vec3(getEntityToWorldMatrix() * glm::vec4(point, 1.0f));
}

glm::vec3 EntityItem::worldToEntity(const glm::vec3& point) const {
    return glm::vec3(getWorldToEntityMatrix() * glm::vec4(point, 1.0f));
}

bool EntityItem::lifetimeHasExpired() const {
    return isMortal() && (getAge() > getLifetime()); 
}

quint64 EntityItem::getExpiry() const {
    return _created + (quint64)(_lifetime * (float)USECS_PER_SECOND);
}

EntityItemProperties EntityItem::getProperties() const {
    EntityItemProperties properties;
    properties._id = getID();
    properties._idSet = true;
    properties._created = _created;

    properties._type = getType();
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(position, getPosition);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dimensions, getDimensions); // NOTE: radius is obsolete
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(rotation, getRotation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(density, getDensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(velocity, getVelocity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(gravity, getGravity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(acceleration, getAcceleration);
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
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(userData, getUserData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(simulatorID, getSimulatorID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(marketplaceID, getMarketplaceID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(name, getName);

    properties._defaultSettings = false;
    
    return properties;
}

bool EntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(position, updatePosition); // this will call recalculate collision shape if needed
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dimensions, updateDimensions); // NOTE: radius is obsolete
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotation, updateRotation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(density, updateDensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(velocity, updateVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gravity, updateGravity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(acceleration, updateAcceleration);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(damping, updateDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifetime, updateLifetime);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(script, setScript);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(registrationPoint, setRegistrationPoint);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularVelocity, updateAngularVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularDamping, updateAngularDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glowLevel, setGlowLevel);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localRenderAlpha, setLocalRenderAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(visible, setVisible);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ignoreForCollisions, updateIgnoreForCollisions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionsWillMove, updateCollisionsWillMove);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(locked, setLocked);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(userData, setUserData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(simulatorID, setSimulatorID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(marketplaceID, setMarketplaceID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(name, setName);

    if (somethingChanged) {
        somethingChangedNotification(); // notify derived classes that something has changed
        uint64_t now = usecTimestampNow();
        #ifdef WANT_DEBUG
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "EntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        #endif
        if (_created != UNKNOWN_CREATED_TIME) {
            setLastEdited(now);
        }
        if (getDirtyFlags() & (EntityItem::DIRTY_POSITION | EntityItem::DIRTY_VELOCITY)) {
            // TODO: Andrew & Brad to discuss. Is this correct? Maybe it is. Need to think through all cases.
            _lastSimulated = now;
        }
    }

    // timestamps
    quint64 timestamp = properties.getCreated();
    if (_created == UNKNOWN_CREATED_TIME && timestamp != UNKNOWN_CREATED_TIME) {
        quint64 now = usecTimestampNow();
        if (timestamp > now) {
            timestamp = now;
        }
        _created = timestamp;

        timestamp = properties.getLastEdited();
        if (timestamp > now) {
            timestamp = now;
        } else if (timestamp < _created) {
            timestamp = _created;
        }
        _lastEdited = timestamp;
    }

    return somethingChanged;
}

void EntityItem::recordCreationTime() {
    assert(_created == UNKNOWN_CREATED_TIME);
    _created = usecTimestampNow();
    _lastEdited = _created;
    _lastUpdated = _created;
    _lastSimulated = _created;
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

    glm::vec3 unrotatedMinRelativeToEntity = - (_dimensions * _registrationPoint);
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
    
    glm::vec3 unrotatedMinRelativeToEntity = - (_dimensions * _registrationPoint);
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
}

// TODO: get rid of all users of this function...
//    ... radius = cornerToCornerLength / 2.0f
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius = sqrt(3 x maxDimension ^ 2) / 2.0f;
float EntityItem::getRadius() const { 
    return 0.5f * glm::length(_dimensions);
}

bool EntityItem::contains(const glm::vec3& point) const {
    if (getShapeType() == SHAPE_TYPE_COMPOUND) {
        return getAABox().contains(point);
    } else {
        ShapeInfo info;
        info.setParams(getShapeType(), glm::vec3(0.5f));
        return info.contains(worldToEntity(point));
    }
}

void EntityItem::computeShapeInfo(ShapeInfo& info) {
    info.setParams(getShapeType(), 0.5f * getDimensions());
}

const float MIN_POSITION_DELTA = 0.0001f;
const float MIN_DIMENSIONS_DELTA = 0.0005f;
const float MIN_ALIGNMENT_DOT = 0.999999f;
const float MIN_VELOCITY_DELTA = 0.01f;
const float MIN_DAMPING_DELTA = 0.001f;
const float MIN_GRAVITY_DELTA = 0.001f;
const float MIN_ACCELERATION_DELTA = 0.001f;
const float MIN_SPIN_DELTA = 0.0003f;

void EntityItem::updatePositionInDomainUnits(const glm::vec3& value) { 
    glm::vec3 position = value * (float)TREE_SCALE;
    updatePosition(position);
}

void EntityItem::updatePosition(const glm::vec3& value) { 
    if (value != _position) {
        auto distance = glm::distance(_position, value);
        _dirtyFlags |= (distance > MIN_POSITION_DELTA) ? EntityItem::DIRTY_POSITION : EntityItem::DIRTY_PHYSICS_NO_WAKE;
        _position = value;
    }
}

void EntityItem::updateDimensionsInDomainUnits(const glm::vec3& value) { 
    glm::vec3 dimensions = value * (float)TREE_SCALE;
    updateDimensions(dimensions);
}

void EntityItem::updateDimensions(const glm::vec3& value) { 
    if (glm::distance(_dimensions, value) > MIN_DIMENSIONS_DELTA) {
        _dimensions = value;
        _dirtyFlags |= (EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS);
    }
}

void EntityItem::updateRotation(const glm::quat& rotation) { 
    if (rotation != _rotation) {
        auto alignmentDot = glm::abs(glm::dot(_rotation, rotation));
        _dirtyFlags |= (alignmentDot < MIN_ALIGNMENT_DOT) ? EntityItem::DIRTY_POSITION : EntityItem::DIRTY_PHYSICS_NO_WAKE;
        _rotation = rotation;
    }
}

void EntityItem::updateMass(float mass) {
    // Setting the mass actually changes the _density (at fixed volume), however
    // we must protect the density range to help maintain stability of physics simulation
    // therefore this method might not accept the mass that is supplied.

    float volume = _volumeMultiplier * _dimensions.x * _dimensions.y * _dimensions.z;

    // compute new density
    float newDensity = _density;
    const float MIN_VOLUME = 1.0e-6f; // 0.001mm^3
    if (volume < 1.0e-6f) {
        // avoid divide by zero
        newDensity = glm::min(mass / MIN_VOLUME, ENTITY_ITEM_MAX_DENSITY);
    } else {
        newDensity = glm::max(glm::min(mass / volume, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    }

    const float MIN_DENSITY_CHANGE_FACTOR = 0.001f; // 0.1 percent
    if (fabsf(_density - newDensity) / _density > MIN_DENSITY_CHANGE_FACTOR) {
        _density = newDensity;
        _dirtyFlags |= EntityItem::DIRTY_MASS;
    }
}

void EntityItem::updateVelocityInDomainUnits(const glm::vec3& value) {
    glm::vec3 velocity = value * (float)TREE_SCALE;
    updateVelocity(velocity);
}

void EntityItem::updateVelocity(const glm::vec3& value) { 
    if (glm::distance(_velocity, value) > MIN_VELOCITY_DELTA) {
        if (glm::length(value) < MIN_VELOCITY_DELTA) {
            _velocity = ENTITY_ITEM_ZERO_VEC3;
        } else {
            _velocity = value;
        }
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateDamping(float value) { 
    if (fabsf(_damping - value) > MIN_DAMPING_DELTA) {
        _damping = glm::clamp(value, 0.0f, 1.0f);
        _dirtyFlags |= EntityItem::DIRTY_MATERIAL;
    }
}

void EntityItem::updateGravityInDomainUnits(const glm::vec3& value) { 
    glm::vec3 gravity = value * (float) TREE_SCALE;
    updateGravity(gravity);
}

void EntityItem::updateGravity(const glm::vec3& value) { 
    if (glm::distance(_gravity, value) > MIN_GRAVITY_DELTA) {
        _gravity = value;
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateAcceleration(const glm::vec3& value) { 
    if (glm::distance(_acceleration, value) > MIN_ACCELERATION_DELTA) {
        _acceleration = value;
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateAngularVelocity(const glm::vec3& value) { 
    auto distance = glm::distance(_angularVelocity, value);
    if (distance > MIN_SPIN_DELTA) {
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
        if (glm::length(value) < MIN_SPIN_DELTA) {
            _angularVelocity = ENTITY_ITEM_ZERO_VEC3;
        } else {
            _angularVelocity = value;
        }
    }
}

void EntityItem::updateAngularDamping(float value) { 
    if (fabsf(_angularDamping - value) > MIN_DAMPING_DELTA) {
        _angularDamping = glm::clamp(value, 0.0f, 1.0f);
        _dirtyFlags |= EntityItem::DIRTY_MATERIAL;
    }
}

void EntityItem::updateIgnoreForCollisions(bool value) { 
    if (_ignoreForCollisions != value) {
        _ignoreForCollisions = value; 
        _dirtyFlags |= EntityItem::DIRTY_COLLISION_GROUP;
    }
}

void EntityItem::updateCollisionsWillMove(bool value) { 
    if (_collisionsWillMove != value) {
        _collisionsWillMove = value; 
        _dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE;
    }
}

void EntityItem::updateLifetime(float value) {
    if (_lifetime != value) {
        _lifetime = value;
        _dirtyFlags |= EntityItem::DIRTY_LIFETIME;
    }
}

void EntityItem::setSimulatorID(const QUuid& value) {
    if (_simulatorID != value) {
        _simulatorID = value;
        _simulatorIDChangedTime = usecTimestampNow();
    }
}

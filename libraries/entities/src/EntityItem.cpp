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

#include "EntityItem.h"

#include <QtCore/QObject>
#include <QtEndian>

#include <glm/gtx/transform.hpp>

#include <BufferParser.h>
#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <Octree.h>
#include <PhysicsHelpers.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()
#include <SoundCache.h>
#include <LogHandler.h>

#include "EntityScriptingInterface.h"
#include "EntitiesLogging.h"
#include "EntityTree.h"
#include "EntitySimulation.h"
#include "EntityActionFactoryInterface.h"


int EntityItem::_maxActionsDataSize = 800;
quint64 EntityItem::_rememberDeletedActionTime = 20 * USECS_PER_SECOND;
std::function<bool()> EntityItem::_entitiesShouldFadeFunction = [](){ return true; };

EntityItem::EntityItem(const EntityItemID& entityItemID) :
    SpatiallyNestable(NestableType::Entity, entityItemID),
    _type(EntityTypes::Unknown),
    _lastSimulated(0),
    _lastUpdated(0),
    _lastEdited(0),
    _lastEditedBy(ENTITY_ITEM_DEFAULT_LAST_EDITED_BY),
    _lastEditedFromRemote(0),
    _lastEditedFromRemoteInRemoteTime(0),
    _created(UNKNOWN_CREATED_TIME),
    _changedOnServer(0),
    _localRenderAlpha(ENTITY_ITEM_DEFAULT_LOCAL_RENDER_ALPHA),
    _density(ENTITY_ITEM_DEFAULT_DENSITY),
    _volumeMultiplier(1.0f),
    _gravity(ENTITY_ITEM_DEFAULT_GRAVITY),
    _acceleration(ENTITY_ITEM_DEFAULT_ACCELERATION),
    _damping(ENTITY_ITEM_DEFAULT_DAMPING),
    _restitution(ENTITY_ITEM_DEFAULT_RESTITUTION),
    _friction(ENTITY_ITEM_DEFAULT_FRICTION),
    _lifetime(ENTITY_ITEM_DEFAULT_LIFETIME),
    _script(ENTITY_ITEM_DEFAULT_SCRIPT),
    _scriptTimestamp(ENTITY_ITEM_DEFAULT_SCRIPT_TIMESTAMP),
    _collisionSoundURL(ENTITY_ITEM_DEFAULT_COLLISION_SOUND_URL),
    _registrationPoint(ENTITY_ITEM_DEFAULT_REGISTRATION_POINT),
    _angularDamping(ENTITY_ITEM_DEFAULT_ANGULAR_DAMPING),
    _visible(ENTITY_ITEM_DEFAULT_VISIBLE),
    _collisionless(ENTITY_ITEM_DEFAULT_COLLISIONLESS),
    _collisionMask(ENTITY_COLLISION_MASK_DEFAULT),
    _dynamic(ENTITY_ITEM_DEFAULT_DYNAMIC),
    _locked(ENTITY_ITEM_DEFAULT_LOCKED),
    _userData(ENTITY_ITEM_DEFAULT_USER_DATA),
    _simulationOwner(),
    _marketplaceID(ENTITY_ITEM_DEFAULT_MARKETPLACE_ID),
    _name(ENTITY_ITEM_DEFAULT_NAME),
    _href(""),
    _description(""),
    _dirtyFlags(0),
    _element(nullptr),
    _physicsInfo(nullptr),
    _simulated(false)
{
    setLocalVelocity(ENTITY_ITEM_DEFAULT_VELOCITY);
    setLocalAngularVelocity(ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY);
    // explicitly set transform parts to set dirty flags used by batch rendering
    setScale(ENTITY_ITEM_DEFAULT_DIMENSIONS);
    quint64 now = usecTimestampNow();
    _lastSimulated = now;
    _lastUpdated = now;
}

EntityItem::~EntityItem() {
    // clear out any left-over actions
    EntityTreePointer entityTree = _element ? _element->getTree() : nullptr;
    EntitySimulationPointer simulation = entityTree ? entityTree->getSimulation() : nullptr;
    if (simulation) {
        clearActions(simulation);
    }

    // these pointers MUST be correct at delete, else we probably have a dangling backpointer
    // to this EntityItem in the corresponding data structure.
    assert(!_simulated);
    assert(!_element);
    assert(!_physicsInfo);
}

EntityPropertyFlags EntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_SIMULATION_OWNER;
    requestedProperties += PROP_POSITION;
    requestedProperties += PROP_ROTATION;
    requestedProperties += PROP_VELOCITY;
    requestedProperties += PROP_ANGULAR_VELOCITY;
    requestedProperties += PROP_ACCELERATION;

    requestedProperties += PROP_DIMENSIONS; // NOTE: PROP_RADIUS obsolete
    requestedProperties += PROP_DENSITY;
    requestedProperties += PROP_GRAVITY;
    requestedProperties += PROP_DAMPING;
    requestedProperties += PROP_RESTITUTION;
    requestedProperties += PROP_FRICTION;
    requestedProperties += PROP_LIFETIME;
    requestedProperties += PROP_SCRIPT;
    requestedProperties += PROP_SCRIPT_TIMESTAMP;
    requestedProperties += PROP_COLLISION_SOUND_URL;
    requestedProperties += PROP_REGISTRATION_POINT;
    requestedProperties += PROP_ANGULAR_DAMPING;
    requestedProperties += PROP_VISIBLE;
    requestedProperties += PROP_COLLISIONLESS;
    requestedProperties += PROP_COLLISION_MASK;
    requestedProperties += PROP_DYNAMIC;
    requestedProperties += PROP_LOCKED;
    requestedProperties += PROP_USER_DATA;
    requestedProperties += PROP_MARKETPLACE_ID;
    requestedProperties += PROP_NAME;
    requestedProperties += PROP_HREF;
    requestedProperties += PROP_DESCRIPTION;
    requestedProperties += PROP_ACTION_DATA;
    requestedProperties += PROP_PARENT_ID;
    requestedProperties += PROP_PARENT_JOINT_INDEX;
    requestedProperties += PROP_QUERY_AA_CUBE;

    requestedProperties += PROP_CLIENT_ONLY;
    requestedProperties += PROP_OWNING_AVATAR_ID;

    requestedProperties += PROP_LAST_EDITED_BY;

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

    successIDFits = packetData->appendRawData(encodedID);
    if (successIDFits) {
        successTypeFits = packetData->appendRawData(encodedType);
    }
    if (successTypeFits) {
        successCreatedFits = packetData->appendValue(_created);
    }
    if (successCreatedFits) {
        successLastEditedFits = packetData->appendValue(lastEdited);
    }
    if (successLastEditedFits) {
        successLastUpdatedFits = packetData->appendRawData(encodedUpdateDelta);
    }
    if (successLastUpdatedFits) {
        successLastSimulatedFits = packetData->appendRawData(encodedSimulatedDelta);
    }

    if (successLastSimulatedFits) {
        propertyFlagsOffset = packetData->getUncompressedByteOffset();
        encodedPropertyFlags = propertyFlags;
        oldPropertyFlagsLength = encodedPropertyFlags.length();
        successPropertyFlagsFits = packetData->appendRawData(encodedPropertyFlags);
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

        APPEND_ENTITY_PROPERTY(PROP_SIMULATION_OWNER, _simulationOwner.toByteArray());
        APPEND_ENTITY_PROPERTY(PROP_POSITION, getLocalPosition());
        APPEND_ENTITY_PROPERTY(PROP_ROTATION, getLocalOrientation());
        APPEND_ENTITY_PROPERTY(PROP_VELOCITY, getLocalVelocity());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, getLocalAngularVelocity());
        APPEND_ENTITY_PROPERTY(PROP_ACCELERATION, getAcceleration());

        APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, getDimensions()); // NOTE: PROP_RADIUS obsolete
        APPEND_ENTITY_PROPERTY(PROP_DENSITY, getDensity());
        APPEND_ENTITY_PROPERTY(PROP_GRAVITY, getGravity());
        APPEND_ENTITY_PROPERTY(PROP_DAMPING, getDamping());
        APPEND_ENTITY_PROPERTY(PROP_RESTITUTION, getRestitution());
        APPEND_ENTITY_PROPERTY(PROP_FRICTION, getFriction());
        APPEND_ENTITY_PROPERTY(PROP_LIFETIME, getLifetime());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT, getScript());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, getScriptTimestamp());
        APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, getRegistrationPoint());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, getAngularDamping());
        APPEND_ENTITY_PROPERTY(PROP_VISIBLE, getVisible());
        APPEND_ENTITY_PROPERTY(PROP_COLLISIONLESS, getCollisionless());
        APPEND_ENTITY_PROPERTY(PROP_COLLISION_MASK, getCollisionMask());
        APPEND_ENTITY_PROPERTY(PROP_DYNAMIC, getDynamic());
        APPEND_ENTITY_PROPERTY(PROP_LOCKED, getLocked());
        APPEND_ENTITY_PROPERTY(PROP_USER_DATA, getUserData());
        APPEND_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, getMarketplaceID());
        APPEND_ENTITY_PROPERTY(PROP_NAME, getName());
        APPEND_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, getCollisionSoundURL());
        APPEND_ENTITY_PROPERTY(PROP_HREF, getHref());
        APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, getDescription());
        APPEND_ENTITY_PROPERTY(PROP_ACTION_DATA, getActionData());
        APPEND_ENTITY_PROPERTY(PROP_PARENT_ID, getParentID());
        APPEND_ENTITY_PROPERTY(PROP_PARENT_JOINT_INDEX, getParentJointIndex());
        APPEND_ENTITY_PROPERTY(PROP_QUERY_AA_CUBE, getQueryAACube());
        APPEND_ENTITY_PROPERTY(PROP_LAST_EDITED_BY, getLastEditedBy());

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

    // if any part of our entity was sent, call trackSend
    if (appendState != OctreeElement::NONE) {
        params.trackSend(getID(), getLastEdited());
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


// clients use this method to unpack FULL updates from entity-server
int EntityItem::readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {

        // NOTE: This shouldn't happen. The only versions of the bit stream that didn't support split mtu buffers should
        // be handled by the model subclass and shouldn't call this routine.
        qCDebug(entities) << "EntityItem::readEntityDataFromBuffer()... "
                        "ERROR CASE...args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU";
        return 0;
    }
    setSourceUUID(args.sourceUUID);

    args.entitiesPerPacket++;

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;

    if (bytesLeftToRead < MINIMUM_HEADER_BYTES) {
        return 0;
    }

    qint64 clockSkew = args.sourceNode ? args.sourceNode->getClockSkewUsec() : 0;

    BufferParser parser(data, bytesLeftToRead);

#ifdef DEBUG
#define VALIDATE_ENTITY_ITEM_PARSER 1
#endif

#ifdef VALIDATE_ENTITY_ITEM_PARSER
    int bytesRead = 0;
    int originalLength = bytesLeftToRead;
    // TODO: figure out a way to avoid the big deep copy below.
    QByteArray originalDataBuffer((const char*)data, originalLength); // big deep copy!
    const unsigned char* dataAt = data;
#endif

    // id
    parser.readUuid(_id);
#ifdef VALIDATE_ENTITY_ITEM_PARSER
    {
        QByteArray encodedID = originalDataBuffer.mid(bytesRead, NUM_BYTES_RFC4122_UUID); // maximum possible size
        QUuid id = QUuid::fromRfc4122(encodedID);
        dataAt += encodedID.size();
        bytesRead += encodedID.size();
        Q_ASSERT(id == _id);
        Q_ASSERT(parser.offset() == (unsigned int) bytesRead);
    }
#endif

    // type
    parser.readCompressedCount<quint32>((quint32&)_type);
#ifdef VALIDATE_ENTITY_ITEM_PARSER
    QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
    ByteCountCoded<quint32> typeCoder = encodedType;
    encodedType = typeCoder; // determine true length
    dataAt += encodedType.size();
    bytesRead += encodedType.size();
    quint32 type = typeCoder;
    EntityTypes::EntityType oldType = (EntityTypes::EntityType)type;
    Q_ASSERT(oldType == _type);
    Q_ASSERT(parser.offset() == (unsigned int) bytesRead);
#endif

    bool overwriteLocalData = true; // assume the new content overwrites our local data
    quint64 now = usecTimestampNow();
    bool somethingChanged = false;

    // _created
    {
        quint64 createdFromBuffer = 0;
        parser.readValue(createdFromBuffer);
#ifdef VALIDATE_ENTITY_ITEM_PARSER
        {
            quint64 createdFromBuffer2 = 0;
            memcpy(&createdFromBuffer2, dataAt, sizeof(createdFromBuffer2));
            dataAt += sizeof(createdFromBuffer2);
            bytesRead += sizeof(createdFromBuffer2);
            Q_ASSERT(createdFromBuffer2 == createdFromBuffer);
            Q_ASSERT(parser.offset() == (unsigned int) bytesRead);
        }
#endif
        if (_created == UNKNOWN_CREATED_TIME) {
            // we don't yet have a _created timestamp, so we accept this one
            createdFromBuffer -= clockSkew;
            if (createdFromBuffer > now || createdFromBuffer == UNKNOWN_CREATED_TIME) {
                createdFromBuffer = now;
            }
            _created = createdFromBuffer;
        }
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

    // TODO: we could make this encoded as a delta from _created
    // _lastEdited
    parser.readValue(lastEditedFromBuffer);
#ifdef VALIDATE_ENTITY_ITEM_PARSER
    {
        quint64 lastEditedFromBuffer2 = 0;
        memcpy(&lastEditedFromBuffer2, dataAt, sizeof(lastEditedFromBuffer2));
        dataAt += sizeof(lastEditedFromBuffer2);
        bytesRead += sizeof(lastEditedFromBuffer2);
        Q_ASSERT(lastEditedFromBuffer2 == lastEditedFromBuffer);
        Q_ASSERT(parser.offset() == (unsigned int) bytesRead);
    }
#endif
    quint64 lastEditedFromBufferAdjusted = lastEditedFromBuffer == 0 ? 0 : lastEditedFromBuffer - clockSkew;
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
        qCDebug(entities) << "                              clockSkew:" << clockSkew;
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

    // before proceeding, check to see if this is an entity that we know has been deleted, which
    // might happen in the case of out-of-order and/or recorvered packets, if we've deleted the entity
    // we can confidently ignore this packet
    EntityTreePointer tree = getTree();
    if (tree && tree->isDeletedEntity(_id)) {
        #ifdef WANT_DEBUG
            qCDebug(entities) << "Received packet for previously deleted entity [" << _id << "] ignoring. "
                "(inside " << __FUNCTION__ << ")";
        #endif
        ignoreServerPacket = true;
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
    quint64 updateDelta;
    parser.readCompressedCount(updateDelta);
#ifdef VALIDATE_ENTITY_ITEM_PARSER
    {
        QByteArray encodedUpdateDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
        quint64 updateDelta2 = updateDeltaCoder;
        Q_ASSERT(updateDelta == updateDelta2);
        encodedUpdateDelta = updateDeltaCoder; // determine true length
        dataAt += encodedUpdateDelta.size();
        bytesRead += encodedUpdateDelta.size();
        Q_ASSERT(parser.offset() == (unsigned int) bytesRead);
    }
#endif

    if (overwriteLocalData) {
        _lastUpdated = lastEditedFromBufferAdjusted + updateDelta; // don't adjust for clock skew since we already did that
        #ifdef WANT_DEBUG
            qCDebug(entities) << "                           _lastUpdated:" << debugTime(_lastUpdated, now);
            qCDebug(entities) << "                            _lastEdited:" << debugTime(_lastEdited, now);
            qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
        #endif
    }

    // Newer bitstreams will have a last simulated and a last updated value
    quint64 lastSimulatedFromBufferAdjusted = now;
    if (args.bitstreamVersion >= VERSION_ENTITIES_HAS_LAST_SIMULATED_TIME) {
        // last simulated is stored as ByteCountCoded delta from lastEdited
        quint64 simulatedDelta;
        parser.readCompressedCount(simulatedDelta);
#ifdef VALIDATE_ENTITY_ITEM_PARSER
        {
            QByteArray encodedSimulatedDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
            ByteCountCoded<quint64> simulatedDeltaCoder = encodedSimulatedDelta;
            quint64 simulatedDelta2 = simulatedDeltaCoder;
            Q_ASSERT(simulatedDelta2 == simulatedDelta);
            encodedSimulatedDelta = simulatedDeltaCoder; // determine true length
            dataAt += encodedSimulatedDelta.size();
            bytesRead += encodedSimulatedDelta.size();
            Q_ASSERT(parser.offset() == (unsigned int) bytesRead);
        }
#endif

        if (overwriteLocalData) {
            lastSimulatedFromBufferAdjusted = lastEditedFromBufferAdjusted + simulatedDelta; // don't adjust for clock skew since we already did that
            if (lastSimulatedFromBufferAdjusted > now) {
                lastSimulatedFromBufferAdjusted = now;
            }
            #ifdef WANT_DEBUG
                qCDebug(entities) << "                            _lastEdited:" << debugTime(_lastEdited, now);
                qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
                qCDebug(entities) << "        lastSimulatedFromBufferAdjusted:" << debugTime(lastSimulatedFromBufferAdjusted, now);
            #endif
        }
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
    EntityPropertyFlags propertyFlags;
    parser.readFlags(propertyFlags);
#ifdef VALIDATE_ENTITY_ITEM_PARSER
    {
        QByteArray encodedPropertyFlags = originalDataBuffer.mid(bytesRead); // maximum possible size
        EntityPropertyFlags propertyFlags2 = encodedPropertyFlags;
        dataAt += propertyFlags.getEncodedLength();
        bytesRead += propertyFlags.getEncodedLength();
        Q_ASSERT(propertyFlags2 == propertyFlags);
        Q_ASSERT(parser.offset() == (unsigned int)bytesRead);
    }
#endif

#ifdef VALIDATE_ENTITY_ITEM_PARSER
    Q_ASSERT(parser.data() + parser.offset() == dataAt);
#else
    const unsigned char* dataAt = parser.data() + parser.offset();
    int bytesRead = (int)parser.offset();
#endif

    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid& myNodeID = nodeList->getSessionUUID();
    bool weOwnSimulation = _simulationOwner.matchesValidID(myNodeID);


    // pack SimulationOwner and terse update properties near each other

    // NOTE: the server is authoritative for changes to simOwnerID so we always unpack ownership data
    // even when we would otherwise ignore the rest of the packet.

    if (propertyFlags.getHasProperty(PROP_SIMULATION_OWNER)) {

        QByteArray simOwnerData;
        int bytes = OctreePacketData::unpackDataFromBytes(dataAt, simOwnerData);
        SimulationOwner newSimOwner;
        newSimOwner.fromByteArray(simOwnerData);
        dataAt += bytes;
        bytesRead += bytes;

        if (wantTerseEditLogging() && _simulationOwner != newSimOwner) {
            qCDebug(entities) << "sim ownership for" << getDebugName() << "is now" << newSimOwner;
        }
        if (weOwnSimulation) {
            if (newSimOwner.getID().isNull() && !_simulationOwner.pendingRelease(lastEditedFromBufferAdjusted)) {
                // entity-server is trying to clear our ownership (probably at our own request)
                // but we actually want to own it, therefore we ignore this clear event
                // and pretend that we own it (we assume we'll recover it soon)
            } else if (_simulationOwner.set(newSimOwner)) {
                _dirtyFlags |= Simulation::DIRTY_SIMULATOR_ID;
                somethingChanged = true;
                // recompute weOwnSimulation for later
                weOwnSimulation = _simulationOwner.matchesValidID(myNodeID);
            }
        } else if (newSimOwner.getID().isNull() && _simulationOwner.pendingTake(lastEditedFromBufferAdjusted)) {
            // entity-server is trying to clear someone  else's ownership
            // but we want to own it, therefore we ignore this clear event
            // and pretend that we own it (we assume we'll get it soon)
            weOwnSimulation = true;
            if (!_simulationOwner.isNull()) {
                // someone else really did own it
                _dirtyFlags |= Simulation::DIRTY_SIMULATOR_ID;
                somethingChanged = true;
                _simulationOwner.clearCurrentOwner();
            }
        } else if (_simulationOwner.set(newSimOwner)) {
            _dirtyFlags |= Simulation::DIRTY_SIMULATOR_ID;
            somethingChanged = true;
            // recompute weOwnSimulation for later
            weOwnSimulation = _simulationOwner.matchesValidID(myNodeID);
        }
    }
    {   // When we own the simulation we don't accept updates to the entity's transform/velocities
        // we also want to ignore any duplicate packets that have the same "recently updated" values
        // as a packet we've already recieved. This is because we want multiple edits of the same 
        // information to be idempotent, but if we applied new physics properties we'd resimulation
        // with small differences in results.

        // Because the regular streaming property "setters" only have access to the new value, we've
        // made these lambdas that can access other details about the previous updates to suppress
        // any duplicates.

        // Note: duplicate packets are expected and not wrong. They may be sent for any number of 
        // reasons and the contract is that the client handles them in an idempotent manner.
        auto lastEdited = lastEditedFromBufferAdjusted;
        auto customUpdatePositionFromNetwork = [this, lastEdited, overwriteLocalData, weOwnSimulation](glm::vec3 value){
            bool simulationChanged = lastEdited > _lastUpdatedPositionTimestamp;
            bool valueChanged = value != _lastUpdatedPositionValue;
            bool shouldUpdate = overwriteLocalData && !weOwnSimulation && simulationChanged && valueChanged;
            if (shouldUpdate) {
                updatePositionFromNetwork(value);
                _lastUpdatedPositionTimestamp = lastEdited;
                _lastUpdatedPositionValue = value;
            }
        };

        auto customUpdateRotationFromNetwork = [this, lastEdited, overwriteLocalData, weOwnSimulation](glm::quat value){
            bool simulationChanged = lastEdited > _lastUpdatedRotationTimestamp;
            bool valueChanged = value != _lastUpdatedRotationValue;
            bool shouldUpdate = overwriteLocalData && !weOwnSimulation && simulationChanged && valueChanged;
            if (shouldUpdate) {
                updateRotationFromNetwork(value);
                _lastUpdatedRotationTimestamp = lastEdited;
                _lastUpdatedRotationValue = value;
            }
        };

        auto customUpdateVelocityFromNetwork = [this, lastEdited, overwriteLocalData, weOwnSimulation](glm::vec3 value){
            bool simulationChanged = lastEdited > _lastUpdatedVelocityTimestamp;
            bool valueChanged = value != _lastUpdatedVelocityValue;
            bool shouldUpdate = overwriteLocalData && !weOwnSimulation && simulationChanged && valueChanged;
            if (shouldUpdate) {
                updateVelocityFromNetwork(value);
                _lastUpdatedVelocityTimestamp = lastEdited;
                _lastUpdatedVelocityValue = value;
            }
        };

        auto customUpdateAngularVelocityFromNetwork = [this, lastEdited, overwriteLocalData, weOwnSimulation](glm::vec3 value){
            bool simulationChanged = lastEdited > _lastUpdatedAngularVelocityTimestamp;
            bool valueChanged = value != _lastUpdatedAngularVelocityValue;
            bool shouldUpdate = overwriteLocalData && !weOwnSimulation && simulationChanged && valueChanged;
            if (shouldUpdate) {
                updateAngularVelocityFromNetwork(value);
                _lastUpdatedAngularVelocityTimestamp = lastEdited;
                _lastUpdatedAngularVelocityValue = value;
            }
        };

        auto customSetAcceleration = [this, lastEdited, overwriteLocalData, weOwnSimulation](glm::vec3 value){
            bool simulationChanged = lastEdited > _lastUpdatedAccelerationTimestamp;
            bool valueChanged = value != _lastUpdatedAccelerationValue;
            bool shouldUpdate = overwriteLocalData && !weOwnSimulation && simulationChanged && valueChanged;
            if (shouldUpdate) {
                setAcceleration(value);
                _lastUpdatedAccelerationTimestamp = lastEdited;
                _lastUpdatedAccelerationValue = value;
            }
        };

        READ_ENTITY_PROPERTY(PROP_POSITION, glm::vec3, customUpdatePositionFromNetwork);
        READ_ENTITY_PROPERTY(PROP_ROTATION, glm::quat, customUpdateRotationFromNetwork);
        READ_ENTITY_PROPERTY(PROP_VELOCITY, glm::vec3, customUpdateVelocityFromNetwork);
        READ_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, glm::vec3, customUpdateAngularVelocityFromNetwork);
        READ_ENTITY_PROPERTY(PROP_ACCELERATION, glm::vec3, customSetAcceleration);


    }

    READ_ENTITY_PROPERTY(PROP_DIMENSIONS, glm::vec3, updateDimensions);
    READ_ENTITY_PROPERTY(PROP_DENSITY, float, updateDensity);
    READ_ENTITY_PROPERTY(PROP_GRAVITY, glm::vec3, updateGravity);

    READ_ENTITY_PROPERTY(PROP_DAMPING, float, updateDamping);
    READ_ENTITY_PROPERTY(PROP_RESTITUTION, float, updateRestitution);
    READ_ENTITY_PROPERTY(PROP_FRICTION, float, updateFriction);
    READ_ENTITY_PROPERTY(PROP_LIFETIME, float, updateLifetime);
    READ_ENTITY_PROPERTY(PROP_SCRIPT, QString, setScript);
    READ_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, quint64, setScriptTimestamp);
    READ_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, glm::vec3, updateRegistrationPoint);

    READ_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, float, updateAngularDamping);
    READ_ENTITY_PROPERTY(PROP_VISIBLE, bool, setVisible);
    READ_ENTITY_PROPERTY(PROP_COLLISIONLESS, bool, updateCollisionless);
    READ_ENTITY_PROPERTY(PROP_COLLISION_MASK, uint8_t, updateCollisionMask);
    READ_ENTITY_PROPERTY(PROP_DYNAMIC, bool, updateDynamic);
    READ_ENTITY_PROPERTY(PROP_LOCKED, bool, setLocked);
    READ_ENTITY_PROPERTY(PROP_USER_DATA, QString, setUserData);

    if (args.bitstreamVersion >= VERSION_ENTITIES_HAS_MARKETPLACE_ID) {
        READ_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    }

    READ_ENTITY_PROPERTY(PROP_NAME, QString, setName);
    READ_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, QString, setCollisionSoundURL);
    READ_ENTITY_PROPERTY(PROP_HREF, QString, setHref);
    READ_ENTITY_PROPERTY(PROP_DESCRIPTION, QString, setDescription);
    READ_ENTITY_PROPERTY(PROP_ACTION_DATA, QByteArray, setActionData);

    {   // parentID and parentJointIndex are also protected by simulation ownership
        bool oldOverwrite = overwriteLocalData;
        overwriteLocalData = overwriteLocalData && !weOwnSimulation;
        READ_ENTITY_PROPERTY(PROP_PARENT_ID, QUuid, setParentID);
        READ_ENTITY_PROPERTY(PROP_PARENT_JOINT_INDEX, quint16, setParentJointIndex);
        overwriteLocalData = oldOverwrite;
    }

    READ_ENTITY_PROPERTY(PROP_QUERY_AA_CUBE, AACube, setQueryAACube);
    READ_ENTITY_PROPERTY(PROP_LAST_EDITED_BY, QUuid, setLastEditedBy);

    bytesRead += readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
                                                  propertyFlags, overwriteLocalData, somethingChanged);

    ////////////////////////////////////
    // WARNING: Do not add stream content here after the subclass. Always add it before the subclass
    //
    // NOTE: we had a bad version of the stream that we added stream data after the subclass. We can attempt to recover
    // by doing this parsing here... but it's not likely going to fully recover the content.
    //
    // TODO: Remove this code once we've sufficiently migrated content past this damaged version
    if (args.bitstreamVersion == VERSION_ENTITIES_HAS_MARKETPLACE_ID_DAMAGED) {
        READ_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    }

    if (overwriteLocalData && (getDirtyFlags() & (Simulation::DIRTY_TRANSFORM | Simulation::DIRTY_VELOCITIES))) {
        // NOTE: This code is attempting to "repair" the old data we just got from the server to make it more
        // closely match where the entities should be if they'd stepped forward in time to "now". The server
        // is sending us data with a known "last simulated" time. That time is likely in the past, and therefore
        // this "new" data is actually slightly out of date. We calculate the time we need to skip forward and
        // use our simulation helper routine to get a best estimate of where the entity should be.
        float skipTimeForward = (float)(now - lastSimulatedFromBufferAdjusted) / (float)(USECS_PER_SECOND);

        // we want to extrapolate the motion forward to compensate for packet travel time, but
        // we don't want the side effect of flag setting.
        stepKinematicMotion(skipTimeForward);
    }

    if (overwriteLocalData) {
        if (!_simulationOwner.matchesValidID(myNodeID)) {
            _lastSimulated = now;
        }
    }

    // Tracking for editing roundtrips here. We will tell our EntityTree that we just got incoming data about
    // and entity that was edited at some time in the past. The tree will determine how it wants to track this
    // information.
    if (_element && _element->getTree()) {
        _element->getTree()->trackIncomingEntityLastEdited(lastEditedFromBufferAdjusted, bytesRead);
    }


    return bytesRead;
}

void EntityItem::debugDump() const {
    auto position = getPosition();
    qCDebug(entities) << "EntityItem id:" << getEntityItemID();
    qCDebug(entities, " edited ago:%f", (double)getEditedAgo());
    qCDebug(entities, " position:%f,%f,%f", (double)position.x, (double)position.y, (double)position.z);
    qCDebug(entities) << " dimensions:" << getDimensions();
}

// adjust any internal timestamps to fix clock skew for this server
void EntityItem::adjustEditPacketForClockSkew(QByteArray& buffer, qint64 clockSkew) {
    unsigned char* dataAt = reinterpret_cast<unsigned char*>(buffer.data());
    int octets = numberOfThreeBitSectionsInCode(dataAt);
    int lengthOfOctcode = (int)bytesRequiredForCodeLength(octets);
    dataAt += lengthOfOctcode;

    // lastEdited
    quint64 lastEditedInLocalTime;
    memcpy(&lastEditedInLocalTime, dataAt, sizeof(lastEditedInLocalTime));
    quint64 lastEditedInServerTime = lastEditedInLocalTime > 0 ? lastEditedInLocalTime + clockSkew : 0;
    memcpy(dataAt, &lastEditedInServerTime, sizeof(lastEditedInServerTime));
    #ifdef WANT_DEBUG
        qCDebug(entities, "EntityItem::adjustEditPacketForClockSkew()...");
        qCDebug(entities) << "     lastEditedInLocalTime: " << lastEditedInLocalTime;
        qCDebug(entities) << "                 clockSkew: " << clockSkew;
        qCDebug(entities) << "    lastEditedInServerTime: " << lastEditedInServerTime;
    #endif
    //assert(lastEditedInLocalTime > (quint64)0);
}

float EntityItem::computeMass() const {
    glm::vec3 dimensions = getDimensions();
    return _density * _volumeMultiplier * dimensions.x * dimensions.y * dimensions.z;
}

void EntityItem::setDensity(float density) {
    _density = glm::max(glm::min(density, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
}

void EntityItem::updateDensity(float density) {
    float clampedDensity = glm::max(glm::min(density, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    if (_density != clampedDensity) {
        _density = clampedDensity;
        _dirtyFlags |= Simulation::DIRTY_MASS;
    }
}

void EntityItem::setMass(float mass) {
    // Setting the mass actually changes the _density (at fixed volume), however
    // we must protect the density range to help maintain stability of physics simulation
    // therefore this method might not accept the mass that is supplied.

    glm::vec3 dimensions = getDimensions();
    float volume = _volumeMultiplier * dimensions.x * dimensions.y * dimensions.z;

    // compute new density
    const float MIN_VOLUME = 1.0e-6f; // 0.001mm^3
    float newDensity = 1.0f;
    if (volume < 1.0e-6f) {
        // avoid divide by zero
        newDensity = glm::min(mass / MIN_VOLUME, ENTITY_ITEM_MAX_DENSITY);
    } else {
        newDensity = glm::max(glm::min(mass / volume, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    }
    if (_density != newDensity) {
        _density = newDensity;
        _dirtyFlags |= Simulation::DIRTY_MASS;
    }
}

void EntityItem::setHref(QString value) {
    auto href = value.toLower();
    if (! (value.toLower().startsWith("hifi://")) ) {
        return;
    }
    _href = value;
}

void EntityItem::setCollisionSoundURL(const QString& value) {
    if (_collisionSoundURL != value) {
        _collisionSoundURL = value;

        if (auto myTree = getTree()) {
            myTree->notifyNewCollisionSoundURL(_collisionSoundURL, getEntityItemID());
        }
    }
}

SharedSoundPointer EntityItem::getCollisionSound() {
    if (!_collisionSound) {
        _collisionSound = DependencyManager::get<SoundCache>()->getSound(_collisionSoundURL);
    }
    return _collisionSound;
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

    if (!stepKinematicMotion(timeElapsed)) {
        // this entity is no longer moving
        // flag it to transition from KINEMATIC to STATIC
        _dirtyFlags |= Simulation::DIRTY_MOTION_TYPE;
        setAcceleration(Vectors::ZERO);
    }
    _lastSimulated = now;
}

bool EntityItem::stepKinematicMotion(float timeElapsed) {
    // get all the data
    Transform transform;
    glm::vec3 linearVelocity;
    glm::vec3 angularVelocity;
    getLocalTransformAndVelocities(transform, linearVelocity, angularVelocity);

    // find out if it is moving
    bool isSpinning = (glm::length2(angularVelocity) > 0.0f);
    float linearSpeedSquared = glm::length2(linearVelocity);
    bool isTranslating = linearSpeedSquared > 0.0f;
    bool moving = isTranslating || isSpinning;
    if (!moving) {
        return false;
    }

    if (timeElapsed <= 0.0f) {
        // someone gave us a useless time value so bail early
        // but return 'true' because it is moving
        return true;
    }

    const float MAX_TIME_ELAPSED = 1.0f; // seconds
    if (timeElapsed > MAX_TIME_ELAPSED) {
        qCWarning(entities) << "kinematic timestep = " << timeElapsed << " truncated to " << MAX_TIME_ELAPSED;
    }
    timeElapsed = glm::min(timeElapsed, MAX_TIME_ELAPSED);

    if (isSpinning) {
        // angular damping
        if (_angularDamping > 0.0f) {
            angularVelocity *= powf(1.0f - _angularDamping, timeElapsed);
        }

        const float MIN_KINEMATIC_ANGULAR_SPEED_SQUARED =
            KINEMATIC_ANGULAR_SPEED_THRESHOLD * KINEMATIC_ANGULAR_SPEED_THRESHOLD;
        if (glm::length2(angularVelocity) < MIN_KINEMATIC_ANGULAR_SPEED_SQUARED) {
            angularVelocity = Vectors::ZERO;
        } else {
            // for improved agreement with the way Bullet integrates rotations we use an approximation
            // and break the integration into bullet-sized substeps
            glm::quat rotation = transform.getRotation();
            float dt = timeElapsed;
            while (dt > 0.0f) {
                glm::quat  dQ = computeBulletRotationStep(angularVelocity, glm::min(dt, PHYSICS_ENGINE_FIXED_SUBSTEP));
                rotation = glm::normalize(dQ * rotation);
                dt -= PHYSICS_ENGINE_FIXED_SUBSTEP;
            }
            transform.setRotation(rotation);
        }
    }

    glm::vec3 position = transform.getTranslation();
    const float MIN_KINEMATIC_LINEAR_SPEED_SQUARED =
        KINEMATIC_LINEAR_SPEED_THRESHOLD * KINEMATIC_LINEAR_SPEED_THRESHOLD;
    if (isTranslating) {
        glm::vec3 deltaVelocity = Vectors::ZERO;

        // linear damping
        if (_damping > 0.0f) {
            deltaVelocity = (powf(1.0f - _damping, timeElapsed) - 1.0f) * linearVelocity;
        }

        const float MIN_KINEMATIC_LINEAR_ACCELERATION_SQUARED = 1.0e-4f; // 0.01 m/sec^2
        if (glm::length2(_acceleration) > MIN_KINEMATIC_LINEAR_ACCELERATION_SQUARED) {
            // yes acceleration
            // acceleration is in world-frame but we need it in local-frame
            glm::vec3 linearAcceleration = _acceleration;
            bool success;
            Transform parentTransform = getParentTransform(success);
            if (success) {
                linearAcceleration = glm::inverse(parentTransform.getRotation()) * linearAcceleration;
            }
            deltaVelocity += linearAcceleration * timeElapsed;

            if (linearSpeedSquared < MIN_KINEMATIC_LINEAR_SPEED_SQUARED
                    && glm::length2(deltaVelocity) < MIN_KINEMATIC_LINEAR_SPEED_SQUARED
                    && glm::length2(linearVelocity + deltaVelocity) < MIN_KINEMATIC_LINEAR_SPEED_SQUARED) {
                linearVelocity = Vectors::ZERO;
            } else {
                // NOTE: we do NOT include the second-order acceleration term (0.5 * a * dt^2)
                // when computing the displacement because Bullet also ignores that term.  Yes,
                // this is an approximation and it works best when dt is small.
                position += timeElapsed * linearVelocity;
                linearVelocity += deltaVelocity;
            }
        } else {
            // no acceleration
            if (linearSpeedSquared < MIN_KINEMATIC_LINEAR_SPEED_SQUARED) {
                linearVelocity = Vectors::ZERO;
            } else {
                // NOTE: we don't use second-order acceleration term for linear displacement
                // because Bullet doesn't use it.
                position += timeElapsed * linearVelocity;
                linearVelocity += deltaVelocity;
            }
        }
    }

    transform.setTranslation(position);
    setLocalTransformAndVelocities(transform, linearVelocity, angularVelocity);

    return true;
}

bool EntityItem::isMoving() const {
    return hasVelocity() || hasAngularVelocity();
}

bool EntityItem::isMovingRelativeToParent() const {
    return hasLocalVelocity() || hasLocalAngularVelocity();
}

EntityTreePointer EntityItem::getTree() const {
    EntityTreeElementPointer containingElement = getElement();
    EntityTreePointer tree = containingElement ? containingElement->getTree() : nullptr;
    return tree;
}

SpatialParentTree* EntityItem::getParentTree() const {
    return getTree().get();
}

bool EntityItem::wantTerseEditLogging() const {
    EntityTreePointer tree = getTree();
    return tree ? tree->wantTerseEditLogging() : false;
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

EntityItemProperties EntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EncodeBitstreamParams params; // unknown
    EntityPropertyFlags propertyFlags = desiredProperties.isEmpty() ? getEntityProperties(params) : desiredProperties;
    EntityItemProperties properties(propertyFlags);
    properties._id = getID();
    properties._idSet = true;
    properties._created = _created;
    properties.setClientOnly(_clientOnly);
    properties.setOwningAvatarID(_owningAvatarID);

    properties._type = getType();

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(simulationOwner, getSimulationOwner);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(position, getLocalPosition);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dimensions, getDimensions); // NOTE: radius is obsolete
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(rotation, getLocalOrientation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(density, getDensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(velocity, getLocalVelocity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(gravity, getGravity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(acceleration, getAcceleration);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(damping, getDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(restitution, getRestitution);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(friction, getFriction);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(created, getCreated);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lifetime, getLifetime);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(script, getScript);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(scriptTimestamp, getScriptTimestamp);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(collisionSoundURL, getCollisionSoundURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(registrationPoint, getRegistrationPoint);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularVelocity, getLocalAngularVelocity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularDamping, getAngularDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localRenderAlpha, getLocalRenderAlpha);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(visible, getVisible);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(collisionless, getCollisionless);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(collisionMask, getCollisionMask);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dynamic, getDynamic);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(locked, getLocked);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(userData, getUserData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(marketplaceID, getMarketplaceID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(name, getName);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(href, getHref);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(description, getDescription);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(actionData, getActionData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(parentID, getParentID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(parentJointIndex, getParentJointIndex);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(queryAACube, getQueryAACube);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localPosition, getLocalPosition);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localRotation, getLocalOrientation);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(clientOnly, getClientOnly);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(owningAvatarID, getOwningAvatarID);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lastEditedBy, getLastEditedBy);

    properties._defaultSettings = false;

    return properties;
}

void EntityItem::getAllTerseUpdateProperties(EntityItemProperties& properties) const {
    // a TerseUpdate includes the transform and its derivatives
    if (!properties._positionChanged) {
        properties._position = getLocalPosition();
    }
    if (!properties._velocityChanged) {
        properties._velocity = getLocalVelocity();
    }
    if (!properties._rotationChanged) {
        properties._rotation = getLocalOrientation();
    }
    if (!properties._angularVelocityChanged) {
        properties._angularVelocity = getLocalAngularVelocity();
    }
    if (!properties._accelerationChanged) {
        properties._acceleration = _acceleration;
    }

    properties._positionChanged = true;
    properties._velocityChanged = true;
    properties._rotationChanged = true;
    properties._angularVelocityChanged = true;
    properties._accelerationChanged = true;
}

void EntityItem::pokeSimulationOwnership() {
    _dirtyFlags |= Simulation::DIRTY_SIMULATION_OWNERSHIP_FOR_POKE;
    auto nodeList = DependencyManager::get<NodeList>();
    if (_simulationOwner.matchesValidID(nodeList->getSessionUUID())) {
        // we already own it
        _simulationOwner.promotePriority(SCRIPT_POKE_SIMULATION_PRIORITY);
    } else {
        // we don't own it yet
        _simulationOwner.setPendingPriority(SCRIPT_POKE_SIMULATION_PRIORITY, usecTimestampNow());
    }
}

void EntityItem::grabSimulationOwnership() {
    _dirtyFlags |= Simulation::DIRTY_SIMULATION_OWNERSHIP_FOR_GRAB;
    auto nodeList = DependencyManager::get<NodeList>();
    if (_simulationOwner.matchesValidID(nodeList->getSessionUUID())) {
        // we already own it
        _simulationOwner.promotePriority(SCRIPT_POKE_SIMULATION_PRIORITY);
    } else {
        // we don't own it yet
        _simulationOwner.setPendingPriority(SCRIPT_GRAB_SIMULATION_PRIORITY, usecTimestampNow());
    }
}

bool EntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    // these affect TerseUpdate properties
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(simulationOwner, updateSimulationOwner);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(position, updatePosition);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotation, updateRotation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(velocity, updateVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularVelocity, updateAngularVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(acceleration, setAcceleration);

    // these (along with "position" above) affect tree structure
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dimensions, updateDimensions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(registrationPoint, updateRegistrationPoint);

    // these (along with all properties above) affect the simulation
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(density, updateDensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gravity, updateGravity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(damping, updateDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularDamping, updateAngularDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(restitution, updateRestitution);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(friction, updateFriction);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionless, updateCollisionless);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionMask, updateCollisionMask);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dynamic, updateDynamic);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(created, updateCreated);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifetime, updateLifetime);

    // non-simulation properties below
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(script, setScript);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(scriptTimestamp, setScriptTimestamp);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionSoundURL, setCollisionSoundURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localRenderAlpha, setLocalRenderAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(visible, setVisible);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(locked, setLocked);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(userData, setUserData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(marketplaceID, setMarketplaceID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(name, setName);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(href, setHref);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(description, setDescription);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(actionData, setActionData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(parentID, setParentID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(parentJointIndex, setParentJointIndex);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(queryAACube, setQueryAACube);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(clientOnly, setClientOnly);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(owningAvatarID, setOwningAvatarID);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lastEditedBy, setLastEditedBy);

    AACube saveQueryAACube = _queryAACube;
    checkAndAdjustQueryAACube();
    if (saveQueryAACube != _queryAACube) {
        somethingChanged = true;
    }

    // Now check the sub classes 
    somethingChanged |= setSubClassProperties(properties);

    // Finally notify if change detected
    if (somethingChanged) {
        uint64_t now = usecTimestampNow();
        #ifdef WANT_DEBUG
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "EntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        #endif
        setLastEdited(now);
        somethingChangedNotification(); // notify derived classes that something has changed
        if (getDirtyFlags() & (Simulation::DIRTY_TRANSFORM | Simulation::DIRTY_VELOCITIES)) {
            // anything that sets the transform or velocity must update _lastSimulated which is used
            // for kinematic extrapolation (e.g. we want to extrapolate forward from this moment
            // when position and/or velocity was changed).
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
    }

    return somethingChanged;
}

void EntityItem::recordCreationTime() {
    if (_created == UNKNOWN_CREATED_TIME) {
        _created = usecTimestampNow();
    }
    auto now = usecTimestampNow();
    _lastEdited = _created;
    _lastUpdated = now;
    _lastSimulated = now;
}

const Transform EntityItem::getTransformToCenter(bool& success) const {
    Transform result = getTransform(success);
    if (getRegistrationPoint() != ENTITY_ITEM_HALF_VEC3) { // If it is not already centered, translate to center
        result.postTranslate(ENTITY_ITEM_HALF_VEC3 - getRegistrationPoint()); // Position to center
    }
    return result;
}

void EntityItem::setDimensions(const glm::vec3& value) {
    if (value.x <= 0.0f || value.y <= 0.0f || value.z <= 0.0f) {
        return;
    }
    setScale(value);
}

/// The maximum bounding cube for the entity, independent of it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
///
AACube EntityItem::getMaximumAACube(bool& success) const {
    if (_recalcMaxAACube) {
        // * we know that the position is the center of rotation
        glm::vec3 centerOfRotation = getPosition(success); // also where _registration point is
        if (success) {
            _recalcMaxAACube = false;
            // * we know that the registration point is the center of rotation
            // * we can calculate the length of the furthest extent from the registration point
            //   as the dimensions * max (registrationPoint, (1.0,1.0,1.0) - registrationPoint)
            glm::vec3 dimensions = getDimensions();
            glm::vec3 registrationPoint = (dimensions * _registrationPoint);
            glm::vec3 registrationRemainder = (dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint));
            glm::vec3 furthestExtentFromRegistration = glm::max(registrationPoint, registrationRemainder);

            // * we know that if you rotate in any direction you would create a sphere
            //   that has a radius of the length of furthest extent from registration point
            float radius = glm::length(furthestExtentFromRegistration);

            // * we know that the minimum bounding cube of this maximum possible sphere is
            //   (center - radius) to (center + radius)
            glm::vec3 minimumCorner = centerOfRotation - glm::vec3(radius, radius, radius);

            _maxAACube = AACube(minimumCorner, radius * 2.0f);
        }
    } else {
        success = true;
    }
    return _maxAACube;
}

/// The minimum bounding cube for the entity accounting for it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
///
AACube EntityItem::getMinimumAACube(bool& success) const {
    if (_recalcMinAACube) {
        // position represents the position of the registration point.
        glm::vec3 position = getPosition(success);
        if (success) {
            _recalcMinAACube = false;
            glm::vec3 dimensions = getDimensions();
            glm::vec3 unrotatedMinRelativeToEntity = - (dimensions * _registrationPoint);
            glm::vec3 unrotatedMaxRelativeToEntity = dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint);
            Extents extents = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
            extents.rotate(getRotation());

            // shift the extents to be relative to the position/registration point
            extents.shiftBy(position);

            // the cube that best encompasses extents is...
            AABox box(extents);
            glm::vec3 centerOfBox = box.calcCenter();
            float longestSide = box.getLargestDimension();
            float halfLongestSide = longestSide / 2.0f;
            glm::vec3 cornerOfCube = centerOfBox - glm::vec3(halfLongestSide, halfLongestSide, halfLongestSide);

            _minAACube = AACube(cornerOfCube, longestSide);
        }
    } else {
        success = true;
    }
    return _minAACube;
}

AABox EntityItem::getAABox(bool& success) const {
    if (_recalcAABox) {
        // position represents the position of the registration point.
        glm::vec3 position = getPosition(success);
        if (success) {
            _recalcAABox = false;
            glm::vec3 dimensions = getDimensions();
            glm::vec3 unrotatedMinRelativeToEntity = - (dimensions * _registrationPoint);
            glm::vec3 unrotatedMaxRelativeToEntity = dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint);
            Extents extents = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
            extents.rotate(getRotation());

            // shift the extents to be relative to the position/registration point
            extents.shiftBy(position);

            _cachedAABox = AABox(extents);
        }
    } else {
        success = true;
    }
    return _cachedAABox;
}

AACube EntityItem::getQueryAACube(bool& success) const {
    AACube result = SpatiallyNestable::getQueryAACube(success);
    if (success) {
        return result;
    }
    // this is for when we've loaded an older json file that didn't have queryAACube properties.
    result = getMaximumAACube(success);
    if (success) {
        _queryAACube = result;
        _queryAACubeSet = true;
    }
    return result;
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
    setDimensions(glm::vec3(maxDimension, maxDimension, maxDimension));
}

// TODO: get rid of all users of this function...
//    ... radius = cornerToCornerLength / 2.0f
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius = sqrt(3 x maxDimension ^ 2) / 2.0f;
float EntityItem::getRadius() const {
    return 0.5f * glm::length(getDimensions());
}

void EntityItem::adjustShapeInfoByRegistration(ShapeInfo& info) const {
    if (_registrationPoint != ENTITY_ITEM_DEFAULT_REGISTRATION_POINT) {
        glm::mat4 scale = glm::scale(getDimensions());
        glm::mat4 registration = scale * glm::translate(ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint());
        glm::vec3 regTransVec = glm::vec3(registration[3]); // extract position component from matrix
        info.setOffset(regTransVec);
    }
}

bool EntityItem::contains(const glm::vec3& point) const {
    if (getShapeType() == SHAPE_TYPE_COMPOUND) {
        bool success;
        bool result = getAABox(success).contains(point);
        return result && success;
    } else {
        ShapeInfo info;
        info.setParams(getShapeType(), glm::vec3(0.5f));
        adjustShapeInfoByRegistration(info);
        return info.contains(worldToEntity(point));
    }
}

void EntityItem::computeShapeInfo(ShapeInfo& info) {
    info.setParams(getShapeType(), 0.5f * getDimensions());
    adjustShapeInfoByRegistration(info);
}

float EntityItem::getVolumeEstimate() const {
    glm::vec3 dimensions = getDimensions();
    return dimensions.x * dimensions.y * dimensions.z;
}

void EntityItem::updateRegistrationPoint(const glm::vec3& value) {
    if (value != _registrationPoint) {
        setRegistrationPoint(value);
        _dirtyFlags |= Simulation::DIRTY_SHAPE;
    }
}

void EntityItem::updatePosition(const glm::vec3& value) {
    if (getLocalPosition() != value) {
        setLocalPosition(value);
        _dirtyFlags |= Simulation::DIRTY_POSITION;
        forEachDescendant([&](SpatiallyNestablePointer object) {
            if (object->getNestableType() == NestableType::Entity) {
                EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                entity->_dirtyFlags |= Simulation::DIRTY_POSITION;
            }
        });
    }
}

void EntityItem::updatePositionFromNetwork(const glm::vec3& value) {
    if (shouldSuppressLocationEdits()) {
        return;
    }
    updatePosition(value);
}

void EntityItem::updateDimensions(const glm::vec3& value) {
    if (getDimensions() != value) {
        setDimensions(value);
        _dirtyFlags |= (Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS);
    }
}

void EntityItem::updateRotation(const glm::quat& rotation) {
    if (getLocalOrientation() != rotation) {
        setLocalOrientation(rotation);
        _dirtyFlags |= Simulation::DIRTY_ROTATION;
        forEachDescendant([&](SpatiallyNestablePointer object) {
            if (object->getNestableType() == NestableType::Entity) {
                EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                entity->_dirtyFlags |= Simulation::DIRTY_ROTATION;
                entity->_dirtyFlags |= Simulation::DIRTY_POSITION;
            }
        });
    }
}

void EntityItem::updateRotationFromNetwork(const glm::quat& rotation) {
    if (shouldSuppressLocationEdits()) {
        return;
    }
    updateRotation(rotation);
}

void EntityItem::updateMass(float mass) {
    // Setting the mass actually changes the _density (at fixed volume), however
    // we must protect the density range to help maintain stability of physics simulation
    // therefore this method might not accept the mass that is supplied.

    glm::vec3 dimensions = getDimensions();
    float volume = _volumeMultiplier * dimensions.x * dimensions.y * dimensions.z;

    // compute new density
    float newDensity = _density;
    const float MIN_VOLUME = 1.0e-6f; // 0.001mm^3
    if (volume < 1.0e-6f) {
        // avoid divide by zero
        newDensity = glm::min(mass / MIN_VOLUME, ENTITY_ITEM_MAX_DENSITY);
    } else {
        newDensity = glm::max(glm::min(mass / volume, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    }

    if (_density != newDensity) {
        _density = newDensity;
        _dirtyFlags |= Simulation::DIRTY_MASS;
    }
}

void EntityItem::updateVelocity(const glm::vec3& value) {
    glm::vec3 velocity = getLocalVelocity();
    if (velocity != value) {
        if (getShapeType() == SHAPE_TYPE_STATIC_MESH) {
            if (velocity != Vectors::ZERO) {
                setLocalVelocity(Vectors::ZERO);
            }
        } else {
            const float MIN_LINEAR_SPEED = 0.001f;
            if (glm::length(value) < MIN_LINEAR_SPEED) {
                velocity = ENTITY_ITEM_ZERO_VEC3;
            } else {
                velocity = value;
            }
            setLocalVelocity(velocity);
            _dirtyFlags |= Simulation::DIRTY_LINEAR_VELOCITY;
        }
    }
}

void EntityItem::updateVelocityFromNetwork(const glm::vec3& value) {
    if (shouldSuppressLocationEdits()) {
        return;
    }
    updateVelocity(value);
}

void EntityItem::updateDamping(float value) {
    auto clampedDamping = glm::clamp(value, 0.0f, 1.0f);
    if (_damping != clampedDamping) {
        _damping = clampedDamping;
        _dirtyFlags |= Simulation::DIRTY_MATERIAL;
    }
}

void EntityItem::updateGravity(const glm::vec3& value) {
    if (_gravity != value) {
        if (getShapeType() == SHAPE_TYPE_STATIC_MESH) {
            _gravity = Vectors::ZERO;
        } else {
            _gravity = value;
            _dirtyFlags |= Simulation::DIRTY_LINEAR_VELOCITY;
        }
    }
}

void EntityItem::updateAngularVelocity(const glm::vec3& value) {
    glm::vec3 angularVelocity = getLocalAngularVelocity();
    if (angularVelocity != value) {
        if (getShapeType() == SHAPE_TYPE_STATIC_MESH) {
            setLocalAngularVelocity(Vectors::ZERO);
        } else {
            const float MIN_ANGULAR_SPEED = 0.0002f;
            if (glm::length(value) < MIN_ANGULAR_SPEED) {
                angularVelocity = ENTITY_ITEM_ZERO_VEC3;
            } else {
                angularVelocity = value;
            }
            setLocalAngularVelocity(angularVelocity);
            _dirtyFlags |= Simulation::DIRTY_ANGULAR_VELOCITY;
        }
    }
}

void EntityItem::updateAngularVelocityFromNetwork(const glm::vec3& value) {
    if (shouldSuppressLocationEdits()) {
        return;
    }
    updateAngularVelocity(value);
}

void EntityItem::updateAngularDamping(float value) {
    auto clampedDamping = glm::clamp(value, 0.0f, 1.0f);
    if (_angularDamping != clampedDamping) {
        _angularDamping = clampedDamping;
        _dirtyFlags |= Simulation::DIRTY_MATERIAL;
    }
}

void EntityItem::updateCollisionless(bool value) {
    if (_collisionless != value) {
        _collisionless = value;
        _dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP;
    }
}

void EntityItem::updateCollisionMask(uint8_t value) {
    if ((_collisionMask & ENTITY_COLLISION_MASK_DEFAULT) != (value & ENTITY_COLLISION_MASK_DEFAULT)) {
        _collisionMask = (value & ENTITY_COLLISION_MASK_DEFAULT);
        _dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP;
    }
}

void EntityItem::updateDynamic(bool value) {
    if (getDynamic() != value) {
        // dynamic and STATIC_MESH are incompatible so we check for that case
        if (value && getShapeType() == SHAPE_TYPE_STATIC_MESH) {
            if (_dynamic) {
                _dynamic = false;
                _dirtyFlags |= Simulation::DIRTY_MOTION_TYPE;
            }
        } else {
            _dynamic = value;
            _dirtyFlags |= Simulation::DIRTY_MOTION_TYPE;
        }
    }
}

void EntityItem::updateRestitution(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_RESTITUTION, value), ENTITY_ITEM_MIN_RESTITUTION);
    if (_restitution != clampedValue) {
        _restitution = clampedValue;
        _dirtyFlags |= Simulation::DIRTY_MATERIAL;
    }
}

void EntityItem::updateFriction(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_FRICTION, value), ENTITY_ITEM_MIN_FRICTION);
    if (_friction != clampedValue) {
        _friction = clampedValue;
        _dirtyFlags |= Simulation::DIRTY_MATERIAL;
    }
}

void EntityItem::setRestitution(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_RESTITUTION, value), ENTITY_ITEM_MIN_RESTITUTION);
    _restitution = clampedValue;
}

void EntityItem::setFriction(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_FRICTION, value), ENTITY_ITEM_MIN_FRICTION);
    _friction = clampedValue;
}

void EntityItem::updateLifetime(float value) {
    if (_lifetime != value) {
        _lifetime = value;
        _dirtyFlags |= Simulation::DIRTY_LIFETIME;
    }
}

void EntityItem::updateCreated(uint64_t value) {
    if (_created != value) {
        _created = value;
        _dirtyFlags |= Simulation::DIRTY_LIFETIME;
    }
}

void EntityItem::computeCollisionGroupAndFinalMask(int16_t& group, int16_t& mask) const {
    // TODO: detect attachment status and adopt group of wearer
    if (_collisionless) {
        group = BULLET_COLLISION_GROUP_COLLISIONLESS;
        mask = 0;
    } else {
        if (getDynamic()) {
            group = BULLET_COLLISION_GROUP_DYNAMIC;
        } else if (isMovingRelativeToParent() || hasActions()) {
            group = BULLET_COLLISION_GROUP_KINEMATIC;
        } else {
            group = BULLET_COLLISION_GROUP_STATIC;
        }

        uint8_t userMask = getCollisionMask();
        if ((bool)(userMask & USER_COLLISION_GROUP_MY_AVATAR) !=
                (bool)(userMask & USER_COLLISION_GROUP_OTHER_AVATAR)) {
            // asymmetric avatar collision mask bits
            if (!getSimulatorID().isNull() && (!getSimulatorID().isNull()) && getSimulatorID() != Physics::getSessionUUID()) {
                // someone else owns the simulation, so we toggle the avatar bits (swap interpretation)
                userMask ^= USER_COLLISION_MASK_AVATARS | ~userMask;
            }
        }
        mask = Physics::getDefaultCollisionMask(group) & (int16_t)(userMask);
    }
}

void EntityItem::setSimulationOwner(const QUuid& id, quint8 priority) {
    if (wantTerseEditLogging() && (id != _simulationOwner.getID() || priority != _simulationOwner.getPriority())) {
        qCDebug(entities) << "sim ownership for" << getDebugName() << "is now" << id << priority;
    }
    _simulationOwner.set(id, priority);
}

void EntityItem::setSimulationOwner(const SimulationOwner& owner) {
    if (wantTerseEditLogging() && _simulationOwner != owner) {
        qCDebug(entities) << "sim ownership for" << getDebugName() << "is now" << owner;
    }

    _simulationOwner.set(owner);
}

void EntityItem::updateSimulationOwner(const SimulationOwner& owner) {
    if (wantTerseEditLogging() && _simulationOwner != owner) {
        qCDebug(entities) << "sim ownership for" << getDebugName() << "is now" << owner;
    }

    if (_simulationOwner.set(owner)) {
        _dirtyFlags |= Simulation::DIRTY_SIMULATOR_ID;
    }
}

void EntityItem::clearSimulationOwnership() {
    if (wantTerseEditLogging() && !_simulationOwner.isNull()) {
        qCDebug(entities) << "sim ownership for" << getDebugName() << "is now null";
    }

    _simulationOwner.clear();
    // don't bother setting the DIRTY_SIMULATOR_ID flag because clearSimulationOwnership()
    // is only ever called on the entity-server and the flags are only used client-side
    //_dirtyFlags |= Simulation::DIRTY_SIMULATOR_ID;

}

void EntityItem::setPendingOwnershipPriority(quint8 priority, const quint64& timestamp) {
    _simulationOwner.setPendingPriority(priority, timestamp);
}

QString EntityItem::actionsToDebugString() {
    QString result;
    QVector<QByteArray> serializedActions;
    QHash<QUuid, EntityActionPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        const QUuid id = i.key();
        EntityActionPointer action = _objectActions[id];
        EntityActionType actionType = action->getType();
        result += QString("") + actionType + ":" + action->getID().toString() + " ";
        i++;
    }
    return result;
}

bool EntityItem::addAction(EntitySimulationPointer simulation, EntityActionPointer action) {
    bool result;
    withWriteLock([&] {
        checkWaitingToRemove(simulation);

        result = addActionInternal(simulation, action);
        if (result) {
            action->setIsMine(true);
            _actionDataDirty = true;
        } else {
            removeActionInternal(action->getID());
        }
    });

    return result;
}

bool EntityItem::addActionInternal(EntitySimulationPointer simulation, EntityActionPointer action) {
    assert(action);
    assert(simulation);
    auto actionOwnerEntity = action->getOwnerEntity().lock();
    assert(actionOwnerEntity);
    assert(actionOwnerEntity.get() == this);

    const QUuid& actionID = action->getID();
    assert(!_objectActions.contains(actionID) || _objectActions[actionID] == action);
    _objectActions[actionID] = action;
    simulation->addAction(action);

    bool success;
    QByteArray newDataCache;
    serializeActions(success, newDataCache);
    if (success) {
        _allActionsDataCache = newDataCache;
        _dirtyFlags |= Simulation::DIRTY_PHYSICS_ACTIVATION;
    } else {
        qCDebug(entities) << "EntityItem::addActionInternal -- serializeActions failed";
    }
    return success;
}

bool EntityItem::updateAction(EntitySimulationPointer simulation, const QUuid& actionID, const QVariantMap& arguments) {
    bool success = false;
    withWriteLock([&] {
        checkWaitingToRemove(simulation);

        if (!_objectActions.contains(actionID)) {
            return;
        }

        EntityActionPointer action = _objectActions[actionID];

        success = action->updateArguments(arguments);
        if (success) {
            action->setIsMine(true);
            serializeActions(success, _allActionsDataCache);
            _dirtyFlags |= Simulation::DIRTY_PHYSICS_ACTIVATION;
        } else {
            qCDebug(entities) << "EntityItem::updateAction failed";
        }
    });
    return success;
}

bool EntityItem::removeAction(EntitySimulationPointer simulation, const QUuid& actionID) {
    bool success = false;
    withWriteLock([&] {
        checkWaitingToRemove(simulation);
        success = removeActionInternal(actionID);
    });
    return success;
}

bool EntityItem::removeActionInternal(const QUuid& actionID, EntitySimulationPointer simulation) {
    _previouslyDeletedActions.insert(actionID, usecTimestampNow());
    if (_objectActions.contains(actionID)) {
        if (!simulation) {
            EntityTreePointer entityTree = _element ? _element->getTree() : nullptr;
            simulation = entityTree ? entityTree->getSimulation() : nullptr;
        }

        EntityActionPointer action = _objectActions[actionID];

        action->setOwnerEntity(nullptr);
        action->setIsMine(false);
        _objectActions.remove(actionID);

        if (simulation) {
            action->removeFromSimulation(simulation);
        }

        bool success = true;
        serializeActions(success, _allActionsDataCache);
        _dirtyFlags |= Simulation::DIRTY_PHYSICS_ACTIVATION;
        setActionDataNeedsTransmit(true);
        return success;
    }
    return false;
}

bool EntityItem::clearActions(EntitySimulationPointer simulation) {
    withWriteLock([&] {
        QHash<QUuid, EntityActionPointer>::iterator i = _objectActions.begin();
        while (i != _objectActions.end()) {
            const QUuid id = i.key();
            EntityActionPointer action = _objectActions[id];
            i = _objectActions.erase(i);
            action->setOwnerEntity(nullptr);
            action->removeFromSimulation(simulation);
        }
        // empty _serializedActions means no actions for the EntityItem
        _actionsToRemove.clear();
        _allActionsDataCache.clear();
        _dirtyFlags |= Simulation::DIRTY_PHYSICS_ACTIVATION;
    });
    return true;
}


void EntityItem::deserializeActions() {
    withWriteLock([&] {
        deserializeActionsInternal();
    });
}


void EntityItem::deserializeActionsInternal() {
    quint64 now = usecTimestampNow();

    if (!_element) {
        qCDebug(entities) << "EntityItem::deserializeActionsInternal -- no _element";
        return;
    }

    EntityTreePointer entityTree = getTree();
    assert(entityTree);
    EntitySimulationPointer simulation = entityTree ? entityTree->getSimulation() : nullptr;
    assert(simulation);

    QVector<QByteArray> serializedActions;
    if (_allActionsDataCache.size() > 0) {
        QDataStream serializedActionsStream(_allActionsDataCache);
        serializedActionsStream >> serializedActions;
    }

    // Keep track of which actions got added or updated by the new actionData
    QSet<QUuid> updated;

    foreach(QByteArray serializedAction, serializedActions) {
        QDataStream serializedActionStream(serializedAction);
        EntityActionType actionType;
        QUuid actionID;
        serializedActionStream >> actionType;
        serializedActionStream >> actionID;
        if (_previouslyDeletedActions.contains(actionID)) {
            continue;
        }

        if (_objectActions.contains(actionID)) {
            EntityActionPointer action = _objectActions[actionID];
            // TODO: make sure types match?  there isn't currently a way to
            // change the type of an existing action.
            if (!action->isMine()) {
                action->deserialize(serializedAction);
            }
            updated << actionID;
        } else {
            auto actionFactory = DependencyManager::get<EntityActionFactoryInterface>();
            EntityItemPointer entity = getThisPointer();
            EntityActionPointer action = actionFactory->factoryBA(entity, serializedAction);
            if (action) {
                entity->addActionInternal(simulation, action);
                updated << actionID;
            } else {
                static QString repeatedMessage =
                    LogHandler::getInstance().addRepeatedMessageRegex(".*action creation failed for.*");
                qCDebug(entities) << "EntityItem::deserializeActionsInternal -- action creation failed for"
                                  << getID() << getName();
                removeActionInternal(actionID, nullptr);
            }
        }
    }

    // remove any actions that weren't included in the new data.
    QHash<QUuid, EntityActionPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        QUuid id = i.key();
        if (!updated.contains(id)) {
            EntityActionPointer action = i.value();

            if (action->isMine()) {
                // we just received an update that didn't include one of our actions.  tell the server about it (again).
                setActionDataNeedsTransmit(true);
            } else {
                // don't let someone else delete my action.
                _actionsToRemove << id;
                _previouslyDeletedActions.insert(id, now);
            }
        }
        i++;
    }

    // trim down _previouslyDeletedActions
    QMutableHashIterator<QUuid, quint64> _previouslyDeletedIter(_previouslyDeletedActions);
    while (_previouslyDeletedIter.hasNext()) {
        _previouslyDeletedIter.next();
        if (now - _previouslyDeletedIter.value() > _rememberDeletedActionTime) {
            _previouslyDeletedActions.remove(_previouslyDeletedIter.key());
        }
    }

    _actionDataDirty = true;

    return;
}

void EntityItem::checkWaitingToRemove(EntitySimulationPointer simulation) {
    foreach(QUuid actionID, _actionsToRemove) {
        removeActionInternal(actionID, simulation);
    }
    _actionsToRemove.clear();
}

void EntityItem::setActionData(QByteArray actionData) {
    withWriteLock([&] {
        setActionDataInternal(actionData);
    });
}

void EntityItem::setActionDataInternal(QByteArray actionData) {
    if (_allActionsDataCache != actionData) {
        _allActionsDataCache = actionData;
        deserializeActionsInternal();
    }
    checkWaitingToRemove();
}

void EntityItem::serializeActions(bool& success, QByteArray& result) const {
    if (_objectActions.size() == 0) {
        success = true;
        result.clear();
        return;
    }

    QVector<QByteArray> serializedActions;
    QHash<QUuid, EntityActionPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        const QUuid id = i.key();
        EntityActionPointer action = _objectActions[id];
        QByteArray bytesForAction = action->serialize();
        serializedActions << bytesForAction;
        i++;
    }

    QDataStream serializedActionsStream(&result, QIODevice::WriteOnly);
    serializedActionsStream << serializedActions;

    if (result.size() >= _maxActionsDataSize) {
        qCDebug(entities) << "EntityItem::serializeActions size is too large -- "
                          << result.size() << ">=" << _maxActionsDataSize;
        success = false;
        return;
    }

    success = true;
    return;
}

const QByteArray EntityItem::getActionDataInternal() const {
    if (_actionDataDirty) {
        bool success;
        serializeActions(success, _allActionsDataCache);
        if (success) {
            _actionDataDirty = false;
        }
    }
    return _allActionsDataCache;
}

const QByteArray EntityItem::getActionData() const {
    QByteArray result;

    if (_actionDataDirty) {
        withWriteLock([&] {
            getActionDataInternal();
            result = _allActionsDataCache;
        });
    } else {
        withReadLock([&] {
            result = _allActionsDataCache;
        });
    }
    return result;
}

QVariantMap EntityItem::getActionArguments(const QUuid& actionID) const {
    QVariantMap result;
    withReadLock([&] {
        if (_objectActions.contains(actionID)) {
            EntityActionPointer action = _objectActions[actionID];
            result = action->getArguments();
            result["type"] = EntityActionInterface::actionTypeToString(action->getType());
        }
    });

    return result;
}

bool EntityItem::shouldSuppressLocationEdits() const {
    QHash<QUuid, EntityActionPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        if (i.value()->shouldSuppressLocationEdits()) {
            return true;
        }
        i++;
    }

    return false;
}

QList<EntityActionPointer> EntityItem::getActionsOfType(EntityActionType typeToGet) {
    QList<EntityActionPointer> result;

    QHash<QUuid, EntityActionPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        EntityActionPointer action = i.value();
        if (action->getType() == typeToGet && action->isActive()) {
            result += action;
        }
        i++;
    }

    return result;
}

void EntityItem::locationChanged(bool tellPhysics) {
    requiresRecalcBoxes();
    if (tellPhysics) {
        _dirtyFlags |= Simulation::DIRTY_TRANSFORM;
        EntityTreePointer tree = getTree();
        if (tree) {
            tree->entityChanged(getThisPointer());
        }
    }
    SpatiallyNestable::locationChanged(tellPhysics); // tell all the children, also
}

void EntityItem::dimensionsChanged() {
    requiresRecalcBoxes();
    SpatiallyNestable::dimensionsChanged(); // Do what you have to do
}

void EntityItem::globalizeProperties(EntityItemProperties& properties, const QString& messageTemplate, const glm::vec3& offset) const {
    // TODO -- combine this with convertLocationToScriptSemantics
    bool success;
    auto globalPosition = getPosition(success);
    if (success) {
        properties.setPosition(globalPosition + offset);
        properties.setRotation(getRotation());
        properties.setDimensions(getDimensions());
        // Should we do velocities and accelerations, too? This could end up being quite involved, which is why the method exists.
    } else {
        properties.setPosition(getQueryAACube().calcCenter() + offset); // best we can do
    }
    if (!messageTemplate.isEmpty()) {
        QString name = properties.getName();
        if (name.isEmpty()) {
            name = EntityTypes::getEntityTypeName(properties.getType());
        }
        qCWarning(entities) << messageTemplate.arg(getEntityItemID().toString()).arg(name).arg(properties.getParentID().toString());
    }
    QUuid empty;
    properties.setParentID(empty);
}

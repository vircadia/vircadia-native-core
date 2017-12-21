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
#include <QJsonDocument>
#include <NetworkingConstants.h>
#include <NetworkAccessManager.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <glm/gtx/transform.hpp>

#include <BufferParser.h>
#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <Octree.h>
#include <PhysicsHelpers.h>
#include <Profile.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()
#include <LogHandler.h>
#include <Extents.h>

#include "EntityScriptingInterface.h"
#include "EntitiesLogging.h"
#include "EntityTree.h"
#include "EntitySimulation.h"
#include "EntityDynamicFactoryInterface.h"

Q_DECLARE_METATYPE(EntityItemPointer);
int entityItemPointernMetaTypeId = qRegisterMetaType<EntityItemPointer>();

int EntityItem::_maxActionsDataSize = 800;
quint64 EntityItem::_rememberDeletedActionTime = 20 * USECS_PER_SECOND;
QString EntityItem::_marketplacePublicKey;

EntityItem::EntityItem(const EntityItemID& entityItemID) :
    SpatiallyNestable(NestableType::Entity, entityItemID)
{
    setLocalVelocity(ENTITY_ITEM_DEFAULT_VELOCITY);
    setLocalAngularVelocity(ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY);
    // explicitly set transform parts to set dirty flags used by batch rendering
    locationChanged();
    dimensionsChanged();
    quint64 now = usecTimestampNow();
    _lastSimulated = now;
    _lastUpdated = now;
}

EntityItem::~EntityItem() {
    // clear out any left-over actions
    EntityTreeElementPointer element = _element; // use local copy of _element for logic below
    EntityTreePointer entityTree = element ? element->getTree() : nullptr;
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

    requestedProperties += PROP_DIMENSIONS;
    requestedProperties += PROP_DENSITY;
    requestedProperties += PROP_GRAVITY;
    requestedProperties += PROP_DAMPING;
    requestedProperties += PROP_RESTITUTION;
    requestedProperties += PROP_FRICTION;
    requestedProperties += PROP_LIFETIME;
    requestedProperties += PROP_SCRIPT;
    requestedProperties += PROP_SCRIPT_TIMESTAMP;
    requestedProperties += PROP_SERVER_SCRIPTS;
    requestedProperties += PROP_COLLISION_SOUND_URL;
    requestedProperties += PROP_REGISTRATION_POINT;
    requestedProperties += PROP_ANGULAR_DAMPING;
    requestedProperties += PROP_VISIBLE;
    requestedProperties += PROP_COLLISIONLESS;
    requestedProperties += PROP_COLLISION_MASK;
    requestedProperties += PROP_DYNAMIC;
    requestedProperties += PROP_LOCKED;
    requestedProperties += PROP_USER_DATA;

    // Certifiable properties
    requestedProperties += PROP_ITEM_NAME;
    requestedProperties += PROP_ITEM_DESCRIPTION;
    requestedProperties += PROP_ITEM_CATEGORIES;
    requestedProperties += PROP_ITEM_ARTIST;
    requestedProperties += PROP_ITEM_LICENSE;
    requestedProperties += PROP_LIMITED_RUN;
    requestedProperties += PROP_MARKETPLACE_ID;
    requestedProperties += PROP_EDITION_NUMBER;
    requestedProperties += PROP_ENTITY_INSTANCE_NUMBER;
    requestedProperties += PROP_CERTIFICATE_ID;
    requestedProperties += PROP_STATIC_CERTIFICATE_VERSION;

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
                                            EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData) const {

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

    // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
    // then our entityTreeElementExtraEncodeData should include data about which properties we need to append.
    if (entityTreeElementExtraEncodeData && entityTreeElementExtraEncodeData->entities.contains(getEntityItemID())) {
        requestedProperties = entityTreeElementExtraEncodeData->entities.value(getEntityItemID());
    }

    EntityPropertyFlags propertiesDidntFit = requestedProperties;

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

        APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, getDimensions());
        APPEND_ENTITY_PROPERTY(PROP_DENSITY, getDensity());
        APPEND_ENTITY_PROPERTY(PROP_GRAVITY, getGravity());
        APPEND_ENTITY_PROPERTY(PROP_DAMPING, getDamping());
        APPEND_ENTITY_PROPERTY(PROP_RESTITUTION, getRestitution());
        APPEND_ENTITY_PROPERTY(PROP_FRICTION, getFriction());
        APPEND_ENTITY_PROPERTY(PROP_LIFETIME, getLifetime());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT, getScript());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, getScriptTimestamp());
        APPEND_ENTITY_PROPERTY(PROP_SERVER_SCRIPTS, getServerScripts());
        APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, getRegistrationPoint());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, getAngularDamping());
        APPEND_ENTITY_PROPERTY(PROP_VISIBLE, getVisible());
        APPEND_ENTITY_PROPERTY(PROP_COLLISIONLESS, getCollisionless());
        APPEND_ENTITY_PROPERTY(PROP_COLLISION_MASK, getCollisionMask());
        APPEND_ENTITY_PROPERTY(PROP_DYNAMIC, getDynamic());
        APPEND_ENTITY_PROPERTY(PROP_LOCKED, getLocked());
        APPEND_ENTITY_PROPERTY(PROP_USER_DATA, getUserData());

        // Certifiable Properties
        APPEND_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, getMarketplaceID());
        APPEND_ENTITY_PROPERTY(PROP_ITEM_NAME, getItemName());
        APPEND_ENTITY_PROPERTY(PROP_ITEM_DESCRIPTION, getItemDescription());
        APPEND_ENTITY_PROPERTY(PROP_ITEM_CATEGORIES, getItemCategories());
        APPEND_ENTITY_PROPERTY(PROP_ITEM_ARTIST, getItemArtist());
        APPEND_ENTITY_PROPERTY(PROP_ITEM_LICENSE, getItemLicense());
        APPEND_ENTITY_PROPERTY(PROP_LIMITED_RUN, getLimitedRun());
        APPEND_ENTITY_PROPERTY(PROP_EDITION_NUMBER, getEditionNumber());
        APPEND_ENTITY_PROPERTY(PROP_ENTITY_INSTANCE_NUMBER, getEntityInstanceNumber());
        APPEND_ENTITY_PROPERTY(PROP_CERTIFICATE_ID, getCertificateID());
        APPEND_ENTITY_PROPERTY(PROP_STATIC_CERTIFICATE_VERSION, getStaticCertificateVersion());

        APPEND_ENTITY_PROPERTY(PROP_NAME, getName());
        APPEND_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, getCollisionSoundURL());
        APPEND_ENTITY_PROPERTY(PROP_HREF, getHref());
        APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, getDescription());
        APPEND_ENTITY_PROPERTY(PROP_ACTION_DATA, getDynamicData());

        // convert AVATAR_SELF_ID to actual sessionUUID.
        QUuid actualParentID = getParentID();
        if (actualParentID == AVATAR_SELF_ID) {
            auto nodeList = DependencyManager::get<NodeList>();
            actualParentID = nodeList->getSessionUUID();
        }
        APPEND_ENTITY_PROPERTY(PROP_PARENT_ID, actualParentID);

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

    int64_t clockSkew = 0;
    uint64_t maxPingRoundTrip = 33333; // two frames periods at 60 fps
    if (args.sourceNode) {
        clockSkew = args.sourceNode->getClockSkewUsec();
        const float MSECS_PER_USEC = 1000;
        maxPingRoundTrip += args.sourceNode->getPingMs() * MSECS_PER_USEC;
    }

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

    bool filterRejection = false;
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
        // This is used in the custom physics setters, below. When an entity-server filter alters
        // or rejects a set of properties, it clears this. In such cases, we don't want those custom
        // setters to ignore what the server says.
        filterRejection = newSimOwner.getID().isNull();
        if (weOwnSimulation) {
            if (newSimOwner.getID().isNull() && !_simulationOwner.pendingRelease(lastEditedFromBufferAdjusted)) {
                // entity-server is trying to clear our ownership (probably at our own request)
                // but we actually want to own it, therefore we ignore this clear event
                // and pretend that we own it (e.g. we assume we'll receive ownership soon)

                // However, for now, when the server uses a newer time than what we sent, listen to what we're told.
                if (overwriteLocalData) {
                    weOwnSimulation = false;
                }
            } else if (_simulationOwner.set(newSimOwner)) {
                markDirtyFlags(Simulation::DIRTY_SIMULATOR_ID);
                somethingChanged = true;
                // recompute weOwnSimulation for later
                weOwnSimulation = _simulationOwner.matchesValidID(myNodeID);
            }
        } else if (_simulationOwner.pendingTake(now - maxPingRoundTrip)) {
            // we sent a bid before this packet could have been sent from the server
            // so we ignore it and pretend we own the object's simulation
            weOwnSimulation = true;
            if (newSimOwner.getID().isNull()) {
                // entity-server is trying to clear someone  else's ownership
                // but we want to own it, therefore we ignore this clear event
                if (!_simulationOwner.isNull()) {
                    // someone else really did own it
                    markDirtyFlags(Simulation::DIRTY_SIMULATOR_ID);
                    somethingChanged = true;
                    _simulationOwner.clearCurrentOwner();
                }
            }
        } else if (newSimOwner.matchesValidID(myNodeID) && !_hasBidOnSimulation) {
            // entity-server tells us that we have simulation ownership while we never requested this for this EntityItem,
            // this could happen when the user reloads the cache and entity tree.
            markDirtyFlags(Simulation::DIRTY_SIMULATOR_ID);
            somethingChanged = true;
            _simulationOwner.clearCurrentOwner();
            weOwnSimulation = false;
        } else if (_simulationOwner.set(newSimOwner)) {
            markDirtyFlags(Simulation::DIRTY_SIMULATOR_ID);
            somethingChanged = true;
            // recompute weOwnSimulation for later
            weOwnSimulation = _simulationOwner.matchesValidID(myNodeID);
        }
    }

    auto lastEdited = lastEditedFromBufferAdjusted;
    bool otherOverwrites = overwriteLocalData && !weOwnSimulation;
    auto shouldUpdate = [lastEdited, otherOverwrites, filterRejection](quint64 updatedTimestamp, bool valueChanged) {
        bool simulationChanged = lastEdited > updatedTimestamp;
        return otherOverwrites && simulationChanged && (valueChanged || filterRejection);
    };

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
        auto customUpdatePositionFromNetwork = [this, shouldUpdate, lastEdited](glm::vec3 value){
            if (shouldUpdate(_lastUpdatedPositionTimestamp, value != _lastUpdatedPositionValue)) {
                setPosition(value);
                _lastUpdatedPositionTimestamp = lastEdited;
                _lastUpdatedPositionValue = value;
            }
        };

        auto customUpdateRotationFromNetwork = [this, shouldUpdate, lastEdited](glm::quat value){
            if (shouldUpdate(_lastUpdatedRotationTimestamp, value != _lastUpdatedRotationValue)) {
                setRotation(value);
                _lastUpdatedRotationTimestamp = lastEdited;
                _lastUpdatedRotationValue = value;
            }
        };

        auto customUpdateVelocityFromNetwork = [this, shouldUpdate, lastEdited](glm::vec3 value){
             if (shouldUpdate(_lastUpdatedVelocityTimestamp, value != _lastUpdatedVelocityValue)) {
                setVelocity(value);
                _lastUpdatedVelocityTimestamp = lastEdited;
                _lastUpdatedVelocityValue = value;
            }
        };

        auto customUpdateAngularVelocityFromNetwork = [this, shouldUpdate, lastEdited](glm::vec3 value){
            if (shouldUpdate(_lastUpdatedAngularVelocityTimestamp, value != _lastUpdatedAngularVelocityValue)) {
                setAngularVelocity(value);
                _lastUpdatedAngularVelocityTimestamp = lastEdited;
                _lastUpdatedAngularVelocityValue = value;
            }
        };

        auto customSetAcceleration = [this, shouldUpdate, lastEdited](glm::vec3 value){
            if (shouldUpdate(_lastUpdatedAccelerationTimestamp, value != _lastUpdatedAccelerationValue)) {
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

    READ_ENTITY_PROPERTY(PROP_DIMENSIONS, glm::vec3, setDimensions);
    READ_ENTITY_PROPERTY(PROP_DENSITY, float, setDensity);
    READ_ENTITY_PROPERTY(PROP_GRAVITY, glm::vec3, setGravity);

    READ_ENTITY_PROPERTY(PROP_DAMPING, float, setDamping);
    READ_ENTITY_PROPERTY(PROP_RESTITUTION, float, setRestitution);
    READ_ENTITY_PROPERTY(PROP_FRICTION, float, setFriction);
    READ_ENTITY_PROPERTY(PROP_LIFETIME, float, setLifetime);
    READ_ENTITY_PROPERTY(PROP_SCRIPT, QString, setScript);
    READ_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, quint64, setScriptTimestamp);

    {
        // We use this scope to work around an issue stopping server script changes
        // from being received by an entity script server running a script that continously updates an entity.

        // Basically, we'll allow recent changes to the server scripts even if there are local changes to other properties
        // that have been made more recently.

        bool overwriteLocalData = !ignoreServerPacket || (lastEditedFromBufferAdjusted > _serverScriptsChangedTimestamp);

        READ_ENTITY_PROPERTY(PROP_SERVER_SCRIPTS, QString, setServerScripts);
    }

    READ_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, glm::vec3, setRegistrationPoint);

    READ_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, float, setAngularDamping);
    READ_ENTITY_PROPERTY(PROP_VISIBLE, bool, setVisible);
    READ_ENTITY_PROPERTY(PROP_COLLISIONLESS, bool, setCollisionless);
    READ_ENTITY_PROPERTY(PROP_COLLISION_MASK, uint8_t, setCollisionMask);
    READ_ENTITY_PROPERTY(PROP_DYNAMIC, bool, setDynamic);
    READ_ENTITY_PROPERTY(PROP_LOCKED, bool, setLocked);
    READ_ENTITY_PROPERTY(PROP_USER_DATA, QString, setUserData);

    READ_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    READ_ENTITY_PROPERTY(PROP_ITEM_NAME, QString, setItemName);
    READ_ENTITY_PROPERTY(PROP_ITEM_DESCRIPTION, QString, setItemDescription);
    READ_ENTITY_PROPERTY(PROP_ITEM_CATEGORIES, QString, setItemCategories);
    READ_ENTITY_PROPERTY(PROP_ITEM_ARTIST, QString, setItemArtist);
    READ_ENTITY_PROPERTY(PROP_ITEM_LICENSE, QString, setItemLicense);
    READ_ENTITY_PROPERTY(PROP_LIMITED_RUN, quint32, setLimitedRun);
    READ_ENTITY_PROPERTY(PROP_EDITION_NUMBER, quint32, setEditionNumber);
    READ_ENTITY_PROPERTY(PROP_ENTITY_INSTANCE_NUMBER, quint32, setEntityInstanceNumber);
    READ_ENTITY_PROPERTY(PROP_CERTIFICATE_ID, QString, setCertificateID);
    READ_ENTITY_PROPERTY(PROP_STATIC_CERTIFICATE_VERSION, quint32, setStaticCertificateVersion);

    READ_ENTITY_PROPERTY(PROP_NAME, QString, setName);
    READ_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, QString, setCollisionSoundURL);
    READ_ENTITY_PROPERTY(PROP_HREF, QString, setHref);
    READ_ENTITY_PROPERTY(PROP_DESCRIPTION, QString, setDescription);
    READ_ENTITY_PROPERTY(PROP_ACTION_DATA, QByteArray, setDynamicData);

    {   // parentID and parentJointIndex are also protected by simulation ownership
        bool oldOverwrite = overwriteLocalData;
        overwriteLocalData = overwriteLocalData && !weOwnSimulation;
        READ_ENTITY_PROPERTY(PROP_PARENT_ID, QUuid, setParentID);
        READ_ENTITY_PROPERTY(PROP_PARENT_JOINT_INDEX, quint16, setParentJointIndex);
        overwriteLocalData = oldOverwrite;
    }


    {
        auto customUpdateQueryAACubeFromNetwork = [this, shouldUpdate, lastEdited](AACube value){
            if (shouldUpdate(_lastUpdatedQueryAACubeTimestamp, value != _lastUpdatedQueryAACubeValue)) {
                setQueryAACube(value);
                _lastUpdatedQueryAACubeTimestamp = lastEdited;
                _lastUpdatedQueryAACubeValue = value;
            }
        };
        READ_ENTITY_PROPERTY(PROP_QUERY_AA_CUBE, AACube, customUpdateQueryAACubeFromNetwork);
    }

    READ_ENTITY_PROPERTY(PROP_LAST_EDITED_BY, QUuid, setLastEditedBy);

    bytesRead += readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
                                                  propertyFlags, overwriteLocalData, somethingChanged);

    ////////////////////////////////////
    // WARNING: Do not add stream content here after the subclass. Always add it before the subclass
    //
    // NOTE: we had a bad version of the stream that we added stream data after the subclass. We can attempt to recover
    // by doing this parsing here... but it's not likely going to fully recover the content.
    //

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
    EntityTreeElementPointer element = _element; // use local copy of _element for logic below
    if (element && element->getTree()) {
        element->getTree()->trackIncomingEntityLastEdited(lastEditedFromBufferAdjusted, bytesRead);
    }


    return bytesRead;
}

void EntityItem::debugDump() const {
    auto position = getWorldPosition();
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
    return getDensity() * _volumeMultiplier * dimensions.x * dimensions.y * dimensions.z;
}

void EntityItem::setDensity(float density) {
    float clampedDensity = glm::max(glm::min(density, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    withWriteLock([&] {
        if (_density != clampedDensity) {
            _density = clampedDensity;
            _dirtyFlags |= Simulation::DIRTY_MASS;
        }
    });
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
    withWriteLock([&] {
        if (_density != newDensity) {
            _density = newDensity;
            _dirtyFlags |= Simulation::DIRTY_MASS;
        }
    });
}

void EntityItem::setHref(QString value) {
    auto href = value.toLower();
    if (! (value.toLower().startsWith("hifi://")) ) {
        return;
    }
    withWriteLock([&] {
        _href = value;
    });
}

void EntityItem::setCollisionSoundURL(const QString& value) {
    bool modified = false;
    withWriteLock([&] {
        if (_collisionSoundURL != value) {
            _collisionSoundURL = value;
            modified = true;
        }
    });
    if (modified) {
        if (auto myTree = getTree()) {
            myTree->notifyNewCollisionSoundURL(value, getEntityItemID());
        }
    }
}

void EntityItem::simulate(const quint64& now) {
    DETAILED_PROFILE_RANGE(simulation_physics, "Simulate");
    if (getLastSimulated() == 0) {
        setLastSimulated(now);
    }

    float timeElapsed = (float)(now - getLastSimulated()) / (float)(USECS_PER_SECOND);

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
        markDirtyFlags(Simulation::DIRTY_MOTION_TYPE);
        setAcceleration(Vectors::ZERO);
    }
    setLastSimulated(now);
}

bool EntityItem::stepKinematicMotion(float timeElapsed) {
    DETAILED_PROFILE_RANGE(simulation_physics, "StepKinematicMotion");
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
        float angularDamping = getAngularDamping();
        // angular damping
        if (angularDamping > 0.0f) {
            angularVelocity *= powf(1.0f - angularDamping, timeElapsed);
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
        float damping = getDamping();
        if (damping > 0.0f) {
            deltaVelocity = (powf(1.0f - damping, timeElapsed) - 1.0f) * linearVelocity;
        }

        const float MIN_KINEMATIC_LINEAR_ACCELERATION_SQUARED = 1.0e-4f; // 0.01 m/sec^2
        vec3 acceleration = getAcceleration();
        if (glm::length2(acceleration) > MIN_KINEMATIC_LINEAR_ACCELERATION_SQUARED) {
            // yes acceleration
            // acceleration is in world-frame but we need it in local-frame
            glm::vec3 linearAcceleration = acceleration;
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
    glm::mat4 translation = glm::translate(getWorldPosition());
    glm::mat4 rotation = glm::mat4_cast(getWorldOrientation());
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
    return getCreated() + (quint64)(getLifetime() * (float)USECS_PER_SECOND);
}

EntityItemProperties EntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EncodeBitstreamParams params; // unknown
    EntityPropertyFlags propertyFlags = desiredProperties.isEmpty() ? getEntityProperties(params) : desiredProperties;
    EntityItemProperties properties(propertyFlags);
    properties._id = getID();
    properties._idSet = true;
    properties._created = getCreated();
    properties._lastEdited = getLastEdited();
    properties.setClientOnly(getClientOnly());
    properties.setOwningAvatarID(getOwningAvatarID());

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
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(serverScripts, getServerScripts);
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

    // Certifiable Properties
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(itemName, getItemName);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(itemDescription, getItemDescription);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(itemCategories, getItemCategories);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(itemArtist, getItemArtist);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(itemLicense, getItemLicense);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(limitedRun, getLimitedRun);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(marketplaceID, getMarketplaceID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(editionNumber, getEditionNumber);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(entityInstanceNumber, getEntityInstanceNumber);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(certificateID, getCertificateID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(staticCertificateVersion, getStaticCertificateVersion);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(name, getName);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(href, getHref);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(description, getDescription);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(actionData, getDynamicData);
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
        properties._acceleration = getAcceleration();
    }

    properties._positionChanged = true;
    properties._velocityChanged = true;
    properties._rotationChanged = true;
    properties._angularVelocityChanged = true;
    properties._accelerationChanged = true;
}

void EntityItem::flagForOwnershipBid(uint8_t priority) {
    markDirtyFlags(Simulation::DIRTY_SIMULATION_OWNERSHIP_PRIORITY);
    auto nodeList = DependencyManager::get<NodeList>();
    if (_simulationOwner.matchesValidID(nodeList->getSessionUUID())) {
        // we already own it
        _simulationOwner.promotePriority(priority);
    } else {
        // we don't own it yet
        _simulationOwner.setPendingPriority(priority, usecTimestampNow());
    }
}

bool EntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    // these affect TerseUpdate properties
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(simulationOwner, setSimulationOwner);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(position, setPosition);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotation, setRotation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(velocity, setVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularVelocity, setAngularVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(acceleration, setAcceleration);

    // these (along with "position" above) affect tree structure
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dimensions, setDimensions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(registrationPoint, setRegistrationPoint);

    // these (along with all properties above) affect the simulation
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(density, setDensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gravity, setGravity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(damping, setDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularDamping, setAngularDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(restitution, setRestitution);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(friction, setFriction);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionless, setCollisionless);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionMask, setCollisionMask);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dynamic, setDynamic);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(created, setCreated);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifetime, setLifetime);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(locked, setLocked);

    // non-simulation properties below
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(script, setScript);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(scriptTimestamp, setScriptTimestamp);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(serverScripts, setServerScripts);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionSoundURL, setCollisionSoundURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localRenderAlpha, setLocalRenderAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(visible, setVisible);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(userData, setUserData);

    // Certifiable Properties
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(itemName, setItemName);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(itemDescription, setItemDescription);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(itemCategories, setItemCategories);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(itemArtist, setItemArtist);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(itemLicense, setItemLicense);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(limitedRun, setLimitedRun);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(marketplaceID, setMarketplaceID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(editionNumber, setEditionNumber);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(entityInstanceNumber, setEntityInstanceNumber);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(certificateID, setCertificateID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(staticCertificateVersion, setStaticCertificateVersion);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(name, setName);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(href, setHref);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(description, setDescription);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(actionData, setDynamicData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(parentID, setParentID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(parentJointIndex, setParentJointIndex);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(queryAACube, setQueryAACube);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(clientOnly, setClientOnly);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(owningAvatarID, setOwningAvatarID);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lastEditedBy, setLastEditedBy);

    if (updateQueryAACube()) {
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
        result.postTranslate((ENTITY_ITEM_HALF_VEC3 - getRegistrationPoint()) * getDimensions()); // Position to center
    }
    return result;
}

/// The maximum bounding cube for the entity, independent of it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
///
AACube EntityItem::getMaximumAACube(bool& success) const {
    if (_recalcMaxAACube) {
        glm::vec3 centerOfRotation = getWorldPosition(success); // also where _registration point is
        if (success) {
            _recalcMaxAACube = false;
            // we want to compute the furthestExtent that an entity can extend out from its "position"
            // to do this we compute the max of these two vec3s: registration and 1-registration
            // and then scale by dimensions
            glm::vec3 maxExtents = getDimensions() * glm::max(_registrationPoint, glm::vec3(1.0f) - _registrationPoint);

            // there exists a sphere that contains maxExtents for all rotations
            float radius = glm::length(maxExtents);

            // put a cube around the sphere
            // TODO? replace _maxAACube with _boundingSphereRadius
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
        glm::vec3 position = getWorldPosition(success);
        if (success) {
            _recalcMinAACube = false;
            glm::vec3 dimensions = getDimensions();
            glm::vec3 unrotatedMinRelativeToEntity = - (dimensions * _registrationPoint);
            glm::vec3 unrotatedMaxRelativeToEntity = dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint);
            Extents extents = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
            extents.rotate(getWorldOrientation());

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
        glm::vec3 position = getWorldPosition(success);
        if (success) {
            _recalcAABox = false;
            glm::vec3 dimensions = getDimensions();
            glm::vec3 unrotatedMinRelativeToEntity = - (dimensions * _registrationPoint);
            glm::vec3 unrotatedMaxRelativeToEntity = dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint);
            Extents extents = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
            extents.rotate(getWorldOrientation());

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

bool EntityItem::shouldPuffQueryAACube() const {
    return hasActions() || isChildOfMyAvatar() || isMovingRelativeToParent();
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

void EntityItem::setRegistrationPoint(const glm::vec3& value) {
    if (value != _registrationPoint) {
        withWriteLock([&] {
            _registrationPoint = glm::clamp(value, 0.0f, 1.0f);
        });
        dimensionsChanged(); // Registration Point affects the bounding box
        markDirtyFlags(Simulation::DIRTY_SHAPE);
    }
}

void EntityItem::setPosition(const glm::vec3& value) {
    if (getLocalPosition() != value) {
        setLocalPosition(value);

        EntityTreePointer tree = getTree();
        markDirtyFlags(Simulation::DIRTY_POSITION);
        if (tree) {
            tree->entityChanged(getThisPointer());
        }
        forEachDescendant([&](SpatiallyNestablePointer object) {
            if (object->getNestableType() == NestableType::Entity) {
                EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                entity->markDirtyFlags(Simulation::DIRTY_POSITION);
                if (tree) {
                    tree->entityChanged(entity);
                }
            }
        });
    }
}

void EntityItem::setParentID(const QUuid& value) {
    QUuid oldParentID = getParentID();
    if (oldParentID != value) {
        EntityTreePointer tree = getTree();
        if (tree && !oldParentID.isNull()) {
            tree->removeFromChildrenOfAvatars(getThisPointer());
        }

        uint32_t oldParentNoBootstrapping = 0;
        uint32_t newParentNoBootstrapping = 0;
        if (!value.isNull() && tree) {
            EntityItemPointer entity = tree->findEntityByEntityItemID(value);
            if (entity) {
                newParentNoBootstrapping = entity->getDirtyFlags() & Simulation::NO_BOOTSTRAPPING;
            }
        }

        if (!oldParentID.isNull() && tree) {
            EntityItemPointer entity = tree->findEntityByEntityItemID(oldParentID);
            if (entity) {
                oldParentNoBootstrapping = entity->getDirtyFlags() & Simulation::NO_BOOTSTRAPPING;
            }
        }

        if (!value.isNull() && (value == Physics::getSessionUUID() || value == AVATAR_SELF_ID)) {
            newParentNoBootstrapping |= Simulation::NO_BOOTSTRAPPING;
        }

        if (!oldParentID.isNull() && (oldParentID == Physics::getSessionUUID() || oldParentID == AVATAR_SELF_ID)) {
            oldParentNoBootstrapping |= Simulation::NO_BOOTSTRAPPING;
        }

        if ((bool)(oldParentNoBootstrapping ^ newParentNoBootstrapping)) {
            if ((bool)(newParentNoBootstrapping & Simulation::NO_BOOTSTRAPPING)) {
                markDirtyFlags(Simulation::NO_BOOTSTRAPPING);
                forEachDescendant([&](SpatiallyNestablePointer object) {
                        if (object->getNestableType() == NestableType::Entity) {
                            EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                            entity->markDirtyFlags(Simulation::DIRTY_COLLISION_GROUP | Simulation::NO_BOOTSTRAPPING);
                        }
                });
            } else {
                clearDirtyFlags(Simulation::NO_BOOTSTRAPPING);
                forEachDescendant([&](SpatiallyNestablePointer object) {
                        if (object->getNestableType() == NestableType::Entity) {
                            EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                            entity->markDirtyFlags(Simulation::DIRTY_COLLISION_GROUP);
                            entity->clearDirtyFlags(Simulation::NO_BOOTSTRAPPING);
                        }
                });
            }
        }

        SpatiallyNestable::setParentID(value);
        // children are forced to be kinematic
        // may need to not collide with own avatar
        markDirtyFlags(Simulation::DIRTY_MOTION_TYPE | Simulation::DIRTY_COLLISION_GROUP);

        if (tree) {
            tree->addToNeedsParentFixupList(getThisPointer());
        }
        updateQueryAACube();
    }
}

void EntityItem::setDimensions(const glm::vec3& value) {
    glm::vec3 newDimensions = glm::max(value, glm::vec3(0.0f)); // can never have negative dimensions
    if (getDimensions() != newDimensions) {
        withWriteLock([&] {
            _dimensions = newDimensions;
        });
        locationChanged();
        dimensionsChanged();
        withWriteLock([&] {
            _dirtyFlags |= (Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS);
            _queryAACubeSet = false;
        });
    }
}

void EntityItem::setRotation(glm::quat rotation) {
    if (getLocalOrientation() != rotation) {
        setLocalOrientation(rotation);
        _dirtyFlags |= Simulation::DIRTY_ROTATION;
        forEachDescendant([&](SpatiallyNestablePointer object) {
            if (object->getNestableType() == NestableType::Entity) {
                EntityItemPointer entity = std::static_pointer_cast<EntityItem>(object);
                entity->markDirtyFlags(Simulation::DIRTY_ROTATION | Simulation::DIRTY_POSITION);
            }
        });
    }
}

void EntityItem::setVelocity(const glm::vec3& value) {
    glm::vec3 velocity = getLocalVelocity();
    if (velocity != value) {
        if (getShapeType() == SHAPE_TYPE_STATIC_MESH) {
            if (velocity != Vectors::ZERO) {
                setLocalVelocity(Vectors::ZERO);
            }
        } else {
            float speed = glm::length(value);
            if (!glm::isnan(speed)) {
                const float MIN_LINEAR_SPEED = 0.001f;
                const float MAX_LINEAR_SPEED = 270.0f; // 3m per step at 90Hz
                if (speed < MIN_LINEAR_SPEED) {
                    velocity = ENTITY_ITEM_ZERO_VEC3;
                } else if (speed > MAX_LINEAR_SPEED) {
                    velocity = (MAX_LINEAR_SPEED / speed) * value;
                } else {
                    velocity = value;
                }
                setLocalVelocity(velocity);
                _dirtyFlags |= Simulation::DIRTY_LINEAR_VELOCITY;
            }
        }
    }
}

void EntityItem::setDamping(float value) {
    auto clampedDamping = glm::clamp(value, 0.0f, 1.0f);
    withWriteLock([&] {
        if (_damping != clampedDamping) {
            _damping = clampedDamping;
            _dirtyFlags |= Simulation::DIRTY_MATERIAL;
        }
    });
}

void EntityItem::setGravity(const glm::vec3& value) {
    withWriteLock([&] {
        if (_gravity != value) {
            if (getShapeType() == SHAPE_TYPE_STATIC_MESH) {
                _gravity = Vectors::ZERO;
            } else {
                float magnitude = glm::length(value);
                if (!glm::isnan(magnitude)) {
                    const float MAX_ACCELERATION_OF_GRAVITY = 10.0f * 9.8f; // 10g
                    if (magnitude > MAX_ACCELERATION_OF_GRAVITY) {
                        _gravity = (MAX_ACCELERATION_OF_GRAVITY / magnitude) * value;
                    } else {
                        _gravity = value;
                    }
                    _dirtyFlags |= Simulation::DIRTY_LINEAR_VELOCITY;
                }
            }
        }
    });
}

void EntityItem::setAngularVelocity(const glm::vec3& value) {
    glm::vec3 angularVelocity = getLocalAngularVelocity();
    if (angularVelocity != value) {
        if (getShapeType() == SHAPE_TYPE_STATIC_MESH) {
            setLocalAngularVelocity(Vectors::ZERO);
        } else {
            float speed = glm::length(value);
            if (!glm::isnan(speed)) {
                const float MIN_ANGULAR_SPEED = 0.0002f;
                const float MAX_ANGULAR_SPEED = 9.0f * TWO_PI; // 1/10 rotation per step at 90Hz
                if (speed < MIN_ANGULAR_SPEED) {
                    angularVelocity = ENTITY_ITEM_ZERO_VEC3;
                } else if (speed > MAX_ANGULAR_SPEED) {
                    angularVelocity = (MAX_ANGULAR_SPEED / speed) * value;
                } else {
                    angularVelocity = value;
                }
                setLocalAngularVelocity(angularVelocity);
                _dirtyFlags |= Simulation::DIRTY_ANGULAR_VELOCITY;
            }
        }
    }
}

void EntityItem::setAngularDamping(float value) {
    auto clampedDamping = glm::clamp(value, 0.0f, 1.0f);
    withWriteLock([&] {
        if (_angularDamping != clampedDamping) {
            _angularDamping = clampedDamping;
            _dirtyFlags |= Simulation::DIRTY_MATERIAL;
        }
    });
}

void EntityItem::setCollisionless(bool value) {
    withWriteLock([&] {
        if (_collisionless != value) {
            _collisionless = value;
            _dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP;
        }
    });
}

void EntityItem::setCollisionMask(uint8_t value) {
    withWriteLock([&] {
        if ((_collisionMask & ENTITY_COLLISION_MASK_DEFAULT) != (value & ENTITY_COLLISION_MASK_DEFAULT)) {
            _collisionMask = (value & ENTITY_COLLISION_MASK_DEFAULT);
            _dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP;
        }
    });
}

void EntityItem::setDynamic(bool value) {
    if (getDynamic() != value) {
        withWriteLock([&] {
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
        });
    }
}

void EntityItem::setRestitution(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_RESTITUTION, value), ENTITY_ITEM_MIN_RESTITUTION);
    withWriteLock([&] {
        if (_restitution != clampedValue) {
            _restitution = clampedValue;
            _dirtyFlags |= Simulation::DIRTY_MATERIAL;
        }
    });

}

void EntityItem::setFriction(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_FRICTION, value), ENTITY_ITEM_MIN_FRICTION);
    withWriteLock([&] {
        if (_friction != clampedValue) {
            _friction = clampedValue;
            _dirtyFlags |= Simulation::DIRTY_MATERIAL;
        }
    });
}

void EntityItem::setLifetime(float value) {
    withWriteLock([&] {
        if (_lifetime != value) {
            _lifetime = value;
            _dirtyFlags |= Simulation::DIRTY_LIFETIME;
        }
    });
}

void EntityItem::setCreated(quint64 value) {
    withWriteLock([&] {
        if (_created != value) {
            _created = value;
            _dirtyFlags |= Simulation::DIRTY_LIFETIME;
        }
    });
}

void EntityItem::computeCollisionGroupAndFinalMask(int16_t& group, int16_t& mask) const {
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
            if (!getSimulatorID().isNull() && getSimulatorID() != Physics::getSessionUUID()) {
                // someone else owns the simulation, so we toggle the avatar bits (swap interpretation)
                userMask ^= USER_COLLISION_MASK_AVATARS | ~userMask;
            }
        }

        if ((bool)(_dirtyFlags & Simulation::NO_BOOTSTRAPPING)) {
            userMask &= ~USER_COLLISION_GROUP_MY_AVATAR;
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
    // NOTE: this method only used by EntityServer.  The Interface uses special code in readEntityDataFromBuffer().
    if (wantTerseEditLogging() && _simulationOwner != owner) {
        qCDebug(entities) << "sim ownership for" << getDebugName() << "is now" << owner;
    }

    if (_simulationOwner.set(owner)) {
        markDirtyFlags(Simulation::DIRTY_SIMULATOR_ID);
    }
}

void EntityItem::clearSimulationOwnership() {
    if (wantTerseEditLogging() && !_simulationOwner.isNull()) {
        qCDebug(entities) << "sim ownership for" << getDebugName() << "is now null";
    }

    _simulationOwner.clear();
    // don't bother setting the DIRTY_SIMULATOR_ID flag because:
    // (a) when entity-server calls clearSimulationOwnership() the dirty-flags are meaningless (only used by interface)
    // (b) the interface only calls clearSimulationOwnership() in a context that already knows best about dirty flags
    //markDirtyFlags(Simulation::DIRTY_SIMULATOR_ID);

}

void EntityItem::setPendingOwnershipPriority(quint8 priority, const quint64& timestamp) {
    _simulationOwner.setPendingPriority(priority, timestamp);
}

void EntityItem::rememberHasSimulationOwnershipBid() const {
    _hasBidOnSimulation = true;
}

QString EntityItem::actionsToDebugString() {
    QString result;
    QVector<QByteArray> serializedActions;
    QHash<QUuid, EntityDynamicPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        const QUuid id = i.key();
        EntityDynamicPointer action = _objectActions[id];
        EntityDynamicType actionType = action->getType();
        result += QString("") + actionType + ":" + action->getID().toString() + " ";
        i++;
    }
    return result;
}

bool EntityItem::addAction(EntitySimulationPointer simulation, EntityDynamicPointer action) {
    bool result;
    withWriteLock([&] {
        checkWaitingToRemove(simulation);

        result = addActionInternal(simulation, action);
        if (result) {
            action->setIsMine(true);
            _dynamicDataDirty = true;
        } else {
            removeActionInternal(action->getID());
        }
    });
    updateQueryAACube();

    return result;
}

bool EntityItem::addActionInternal(EntitySimulationPointer simulation, EntityDynamicPointer action) {
    assert(action);
    assert(simulation);
    auto actionOwnerEntity = action->getOwnerEntity().lock();
    assert(actionOwnerEntity);
    assert(actionOwnerEntity.get() == this);

    const QUuid& actionID = action->getID();
    assert(!_objectActions.contains(actionID) || _objectActions[actionID] == action);
    _objectActions[actionID] = action;
    simulation->addDynamic(action);

    bool success;
    QByteArray newDataCache;
    serializeActions(success, newDataCache);
    if (success) {
        _allActionsDataCache = newDataCache;
        _dirtyFlags |= Simulation::DIRTY_PHYSICS_ACTIVATION;

        auto actionType = action->getType();
        if (actionType == DYNAMIC_TYPE_HOLD || actionType == DYNAMIC_TYPE_FAR_GRAB) {
            if (!(bool)(_dirtyFlags & Simulation::NO_BOOTSTRAPPING)) {
                _dirtyFlags |= Simulation::NO_BOOTSTRAPPING;
                _dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP; // may need to not collide with own avatar
                forEachDescendant([&](SpatiallyNestablePointer child) {
                    if (child->getNestableType() == NestableType::Entity) {
                        EntityItemPointer entity = std::static_pointer_cast<EntityItem>(child);
                        entity->markDirtyFlags(Simulation::NO_BOOTSTRAPPING | Simulation::DIRTY_COLLISION_GROUP);
                    }
                });
            }
        }
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

        EntityDynamicPointer action = _objectActions[actionID];

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
    updateQueryAACube();

    return success;
}

bool EntityItem::stillHasGrabActions() const {
    QList<EntityDynamicPointer> holdActions = getActionsOfType(DYNAMIC_TYPE_HOLD);
    QList<EntityDynamicPointer>::const_iterator i = holdActions.begin();
    while (i != holdActions.end()) {
        EntityDynamicPointer action = *i;
        if (action->isMine()) {
            return true;
        }
        i++;
    }
    QList<EntityDynamicPointer> farGrabActions = getActionsOfType(DYNAMIC_TYPE_FAR_GRAB);
    i = farGrabActions.begin();
    while (i != farGrabActions.end()) {
        EntityDynamicPointer action = *i;
        if (action->isMine()) {
            return true;
        }
        i++;
    }

    return false;
}

bool EntityItem::removeActionInternal(const QUuid& actionID, EntitySimulationPointer simulation) {
    _previouslyDeletedActions.insert(actionID, usecTimestampNow());
    if (_objectActions.contains(actionID)) {
        if (!simulation) {
            EntityTreeElementPointer element = _element; // use local copy of _element for logic below
            EntityTreePointer entityTree = element ? element->getTree() : nullptr;
            simulation = entityTree ? entityTree->getSimulation() : nullptr;
        }

        EntityDynamicPointer action = _objectActions[actionID];

        action->setOwnerEntity(nullptr);
        action->setIsMine(false);

        if (simulation) {
            action->removeFromSimulation(simulation);
        }

        bool success = true;
        serializeActions(success, _allActionsDataCache);
        _dirtyFlags |= Simulation::DIRTY_PHYSICS_ACTIVATION;
        auto removedActionType = action->getType();
        if ((removedActionType == DYNAMIC_TYPE_HOLD || removedActionType == DYNAMIC_TYPE_FAR_GRAB) && !stillHasGrabActions()) {
            _dirtyFlags &= ~Simulation::NO_BOOTSTRAPPING;
            _dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP; // may need to not collide with own avatar
            forEachDescendant([&](SpatiallyNestablePointer child) {
                if (child->getNestableType() == NestableType::Entity) {
                    EntityItemPointer entity = std::static_pointer_cast<EntityItem>(child);
                    entity->markDirtyFlags(Simulation::DIRTY_COLLISION_GROUP);
                    entity->clearDirtyFlags(Simulation::NO_BOOTSTRAPPING);
                }
            });
        } else {
            // NO-OP: we assume NO_BOOTSTRAPPING bits and collision group are correct
            // because they should have been set correctly when the action was added
            // and/or when children were linked
        }
        _objectActions.remove(actionID);
        setDynamicDataNeedsTransmit(true);
        return success;
    }
    return false;
}

bool EntityItem::clearActions(EntitySimulationPointer simulation) {
    withWriteLock([&] {
        QHash<QUuid, EntityDynamicPointer>::iterator i = _objectActions.begin();
        while (i != _objectActions.end()) {
            const QUuid id = i.key();
            EntityDynamicPointer action = _objectActions[id];
            i = _objectActions.erase(i);
            action->setOwnerEntity(nullptr);
            action->removeFromSimulation(simulation);
        }
        // empty _serializedActions means no actions for the EntityItem
        _actionsToRemove.clear();
        _allActionsDataCache.clear();
        _dirtyFlags |= Simulation::DIRTY_PHYSICS_ACTIVATION;
        _dirtyFlags |= Simulation::DIRTY_COLLISION_GROUP; // may need to not collide with own avatar
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

    // Keep track of which actions got added or updated by the new dynamicData
    QSet<QUuid> updated;

    foreach(QByteArray serializedAction, serializedActions) {
        QDataStream serializedActionStream(serializedAction);
        EntityDynamicType actionType;
        QUuid actionID;
        serializedActionStream >> actionType;
        serializedActionStream >> actionID;
        if (_previouslyDeletedActions.contains(actionID)) {
            continue;
        }

        if (_objectActions.contains(actionID)) {
            EntityDynamicPointer action = _objectActions[actionID];
            // TODO: make sure types match?  there isn't currently a way to
            // change the type of an existing action.
            if (!action->isMine()) {
                action->deserialize(serializedAction);
            }
            updated << actionID;
        } else {
            auto actionFactory = DependencyManager::get<EntityDynamicFactoryInterface>();
            EntityItemPointer entity = getThisPointer();
            EntityDynamicPointer action = actionFactory->factoryBA(entity, serializedAction);
            if (action) {
                entity->addActionInternal(simulation, action);
                updated << actionID;
            } else {
                static QString repeatedMessage =
                    LogHandler::getInstance().addRepeatedMessageRegex(".*action creation failed for.*");
                qCDebug(entities) << "EntityItem::deserializeActionsInternal -- action creation failed for"
                        << getID() << _name; // getName();
                removeActionInternal(actionID, nullptr);
            }
        }
    }

    // remove any actions that weren't included in the new data.
    QHash<QUuid, EntityDynamicPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        QUuid id = i.key();
        if (!updated.contains(id)) {
            EntityDynamicPointer action = i.value();

            if (action->isMine()) {
                // we just received an update that didn't include one of our actions.  tell the server about it (again).
                setDynamicDataNeedsTransmit(true);
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

    _dynamicDataDirty = true;

    return;
}

void EntityItem::checkWaitingToRemove(EntitySimulationPointer simulation) {
    foreach(QUuid actionID, _actionsToRemove) {
        removeActionInternal(actionID, simulation);
    }
    _actionsToRemove.clear();
}

void EntityItem::setDynamicData(QByteArray dynamicData) {
    withWriteLock([&] {
        setDynamicDataInternal(dynamicData);
    });
}

void EntityItem::setDynamicDataInternal(QByteArray dynamicData) {
    if (_allActionsDataCache != dynamicData) {
        _allActionsDataCache = dynamicData;
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
    QHash<QUuid, EntityDynamicPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        const QUuid id = i.key();
        EntityDynamicPointer action = _objectActions[id];
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

const QByteArray EntityItem::getDynamicDataInternal() const {
    if (_dynamicDataDirty) {
        bool success;
        serializeActions(success, _allActionsDataCache);
        if (success) {
            _dynamicDataDirty = false;
        }
    }
    return _allActionsDataCache;
}

const QByteArray EntityItem::getDynamicData() const {
    QByteArray result;

    if (_dynamicDataDirty) {
        withWriteLock([&] {
            getDynamicDataInternal();
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
            EntityDynamicPointer action = _objectActions[actionID];
            result = action->getArguments();
            result["type"] = EntityDynamicInterface::dynamicTypeToString(action->getType());
        }
    });

    return result;
}

bool EntityItem::shouldSuppressLocationEdits() const {
    // if any of the actions indicate they'd like suppression, suppress
    QHash<QUuid, EntityDynamicPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        if (i.value()->shouldSuppressLocationEdits()) {
            return true;
        }
        i++;
    }

    // if any of the ancestors are MyAvatar, suppress
    if (isChildOfMyAvatar()) {
        return true;
    }

    return false;
}

QList<EntityDynamicPointer> EntityItem::getActionsOfType(EntityDynamicType typeToGet) const {
    QList<EntityDynamicPointer> result;

    QHash<QUuid, EntityDynamicPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        EntityDynamicPointer action = i.value();
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
    somethingChangedNotification();
}

void EntityItem::dimensionsChanged() {
    requiresRecalcBoxes();
    SpatiallyNestable::dimensionsChanged(); // Do what you have to do
    somethingChangedNotification();
}

void EntityItem::globalizeProperties(EntityItemProperties& properties, const QString& messageTemplate, const glm::vec3& offset) const {
    // TODO -- combine this with convertLocationToScriptSemantics
    bool success;
    auto globalPosition = getWorldPosition(success);
    if (success) {
        properties.setPosition(globalPosition + offset);
        properties.setRotation(getWorldOrientation());
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


bool EntityItem::matchesJSONFilters(const QJsonObject& jsonFilters) const {

    // The intention for the query JSON filter and this method is to be flexible to handle a variety of filters for
    // ALL entity properties. Some work will need to be done to the property system so that it can be more flexible
    // (to grab the value and default value of a property given the string representation of that property, for example)

    // currently the only property filter we handle is '+' for serverScripts
    // which means that we only handle a filtered query asking for entities where the serverScripts property is non-default

    static const QString SERVER_SCRIPTS_PROPERTY = "serverScripts";

    foreach(const auto& property, jsonFilters.keys()) {
        if (property == SERVER_SCRIPTS_PROPERTY  && jsonFilters[property] == EntityQueryFilterSymbol::NonDefault) {
            // check if this entity has a non-default value for serverScripts
            if (_serverScripts != ENTITY_ITEM_DEFAULT_SERVER_SCRIPTS) {
                return true;
            } else {
                return false;
            }
        }
    }

    // the json filter syntax did not match what we expected, return a match
    return true;
}

quint64 EntityItem::getLastSimulated() const {
    quint64 result;
    withReadLock([&] {
        result = _lastSimulated;
    });
    return result;
}

void EntityItem::setLastSimulated(quint64 now) {
    withWriteLock([&] {
        _lastSimulated = now;
    });
}

quint64 EntityItem::getLastEdited() const {
    quint64 result;
    withReadLock([&] {
        result = _lastEdited;
    });
    return result;
}

void EntityItem::setLastEdited(quint64 lastEdited) {
    withWriteLock([&] {
        _lastEdited = _lastUpdated = lastEdited;
        _changedOnServer = glm::max(lastEdited, _changedOnServer);
    });
}

quint64 EntityItem::getLastBroadcast() const {
    quint64 result;
    withReadLock([&] {
        result = _lastBroadcast;
    });
    return result;
}

void EntityItem::setLastBroadcast(quint64 lastBroadcast) {
    withWriteLock([&] {
        _lastBroadcast = lastBroadcast;
    });
}

void EntityItem::markAsChangedOnServer() {
    withWriteLock([&] {
        _changedOnServer = usecTimestampNow();
    });
}

quint64 EntityItem::getLastChangedOnServer() const {
    quint64 result;
    withReadLock([&] {
        result = _changedOnServer;
    });
    return result;
}

void EntityItem::update(const quint64& now) {
    withWriteLock([&] {
        _lastUpdated = now;
    });
}

quint64 EntityItem::getLastUpdated() const {
    quint64 result;
    withReadLock([&] {
        result = _lastUpdated;
    });
    return result;
}

void EntityItem::requiresRecalcBoxes() {
    withWriteLock([&] {
        _recalcAABox = true;
        _recalcMinAACube = true;
        _recalcMaxAACube = true;
    });
}

QString EntityItem::getHref() const {
    QString result;
    withReadLock([&] {
        result = _href;
    });
    return result;
}

QString EntityItem::getDescription() const {
    QString result;
    withReadLock([&] {
        result = _description;
    });
    return result;
}

void EntityItem::setDescription(const QString& value) {
    withWriteLock([&] {
        _description = value;
    });
}

float EntityItem::getLocalRenderAlpha() const {
    float result;
    withReadLock([&] {
        result = _localRenderAlpha;
    });
    return result;
}

void EntityItem::setLocalRenderAlpha(float localRenderAlpha) {
    withWriteLock([&] {
        _localRenderAlpha = localRenderAlpha;
    });
}

glm::vec3 EntityItem::getGravity() const {
    glm::vec3 result;
    withReadLock([&] {
        result = _gravity;
    });
    return result;
}

glm::vec3 EntityItem::getAcceleration() const {
    glm::vec3 result;
    withReadLock([&] {
        result = _acceleration;
    });
    return result;
}

void EntityItem::setAcceleration(const glm::vec3& value) {
    withWriteLock([&] {
        _acceleration = value;
    });
}

float EntityItem::getDamping() const {
    float result;
    withReadLock([&] {
        result = _damping;
    });
    return result;
}

float EntityItem::getRestitution() const {
    float result;
    withReadLock([&] {
        result = _restitution;
    });
    return result;
}

float EntityItem::getFriction() const {
    float result;
    withReadLock([&] {
        result = _friction;
    });
    return result;
}

// lifetime related properties.
float EntityItem::getLifetime() const {
    float result;
    withReadLock([&] {
        result = _lifetime;
    });
    return result;
}

quint64 EntityItem::getCreated() const {
    quint64 result;
    withReadLock([&] {
        result = _created;
    });
    return result;
}

QString EntityItem::getScript() const {
    QString result;
    withReadLock([&] {
        result = _script;
    });
    return result;
}

void EntityItem::setScript(const QString& value) {
    withWriteLock([&] {
        _script = value;
    });
}

quint64 EntityItem::getScriptTimestamp() const {
    quint64 result;
    withReadLock([&] {
        result = _scriptTimestamp;
    });
    return result;
}

void EntityItem::setScriptTimestamp(const quint64 value) {
    withWriteLock([&] {
        _scriptTimestamp = value;
    });
}

QString EntityItem::getServerScripts() const {
    QString result;
    withReadLock([&] {
        result = _serverScripts;
    });
    return result;
}

void EntityItem::setServerScripts(const QString& serverScripts) {
    withWriteLock([&] {
        _serverScripts = serverScripts;
        _serverScriptsChangedTimestamp = usecTimestampNow();
    });
}

QString EntityItem::getCollisionSoundURL() const {
    QString result;
    withReadLock([&] {
        result = _collisionSoundURL;
    });
    return result;
}

glm::vec3 EntityItem::getRegistrationPoint() const {
    glm::vec3 result;
    withReadLock([&] {
        result = _registrationPoint;
    });
    return result;
}

float EntityItem::getAngularDamping() const {
    float result;
    withReadLock([&] {
        result = _angularDamping;
    });
    return result;
}

QString EntityItem::getName() const {
    QString result;
    withReadLock([&] {
        result = _name;
    });
    return result;
}

void EntityItem::setName(const QString& value) {
    withWriteLock([&] {
        _name = value;
    });
}

QString EntityItem::getDebugName() {
    QString result = getName();
    if (result.isEmpty()) {
        result = getID().toString();
    }
    return result;
}

bool EntityItem::getVisible() const {
    bool result;
    withReadLock([&] {
        result = _visible;
    });
    return result;
}

void EntityItem::setVisible(bool value) {
    withWriteLock([&] {
        _visible = value;
    });
}

bool EntityItem::isChildOfMyAvatar() const {
    QUuid ancestorID = findAncestorOfType(NestableType::Avatar);
    return !ancestorID.isNull() && (ancestorID == Physics::getSessionUUID() || ancestorID == AVATAR_SELF_ID);
}

bool EntityItem::getCollisionless() const {
    bool result;
    withReadLock([&] {
        result = _collisionless;
    });
    return result;
}

uint8_t EntityItem::getCollisionMask() const {
    uint8_t result;
    withReadLock([&] {
        result = _collisionMask;
    });
    return result;
}

bool EntityItem::getDynamic() const {
    if (SHAPE_TYPE_STATIC_MESH == getShapeType()) {
        return false;
    }
    bool result;
    withReadLock([&] {
        result = _dynamic;
    });
    return result;
}

bool EntityItem::getLocked() const {
    bool result;
    withReadLock([&] {
        result = _locked;
    });
    return result;
}

void EntityItem::setLocked(bool value) {
    bool changed { false };
    withWriteLock([&] {
        if (_locked != value) {
            _locked = value;
            changed = true;
        }
    });
    if (changed) {
        markDirtyFlags(Simulation::DIRTY_MOTION_TYPE);
        EntityTreePointer tree = getTree();
        if (tree) {
            tree->entityChanged(getThisPointer());
        }
    }
}

QString EntityItem::getUserData() const {
    QString result;
    withReadLock([&] {
        result = _userData;
    });
    return result;
}

void EntityItem::setUserData(const QString& value) {
    withWriteLock([&] {
        _userData = value;
    });
}

// Certifiable Properties
#define DEFINE_PROPERTY_GETTER(type, accessor, var) \
type EntityItem::get##accessor() const {            \
    type result;         \
    withReadLock([&] {   \
        result = _##var; \
    });                  \
    return result;       \
}

#define DEFINE_PROPERTY_SETTER(type, accessor, var)   \
void EntityItem::set##accessor(const type & value) { \
    withWriteLock([&] {                               \
       _##var = value;                                \
    });                                               \
}
#define DEFINE_PROPERTY_ACCESSOR(type, accessor, var) DEFINE_PROPERTY_GETTER(type, accessor, var) DEFINE_PROPERTY_SETTER(type, accessor, var)
DEFINE_PROPERTY_ACCESSOR(QString, ItemName, itemName)
DEFINE_PROPERTY_ACCESSOR(QString, ItemDescription, itemDescription)
DEFINE_PROPERTY_ACCESSOR(QString, ItemCategories, itemCategories)
DEFINE_PROPERTY_ACCESSOR(QString, ItemArtist, itemArtist)
DEFINE_PROPERTY_ACCESSOR(QString, ItemLicense, itemLicense)
DEFINE_PROPERTY_ACCESSOR(quint32, LimitedRun, limitedRun)
DEFINE_PROPERTY_ACCESSOR(QString, MarketplaceID, marketplaceID)
DEFINE_PROPERTY_ACCESSOR(quint32, EditionNumber, editionNumber)
DEFINE_PROPERTY_ACCESSOR(quint32, EntityInstanceNumber, entityInstanceNumber)
DEFINE_PROPERTY_ACCESSOR(QString, CertificateID, certificateID)
DEFINE_PROPERTY_ACCESSOR(quint32, StaticCertificateVersion, staticCertificateVersion)

uint32_t EntityItem::getDirtyFlags() const {
    uint32_t result;
    withReadLock([&] {
        result = _dirtyFlags;
    });
    return result;
}


void EntityItem::markDirtyFlags(uint32_t mask) {
    withWriteLock([&] {
        _dirtyFlags |= mask;
    });
}

void EntityItem::clearDirtyFlags(uint32_t mask) {
    withWriteLock([&] {
        _dirtyFlags &= ~mask;
    });
}

float EntityItem::getDensity() const {
    float result;
    withReadLock([&] {
        result = _density;
    });
    return result;
}

EntityItem::ChangeHandlerId EntityItem::registerChangeHandler(const ChangeHandlerCallback& handler) {
    ChangeHandlerId result = QUuid::createUuid();
    withWriteLock([&] {
        _changeHandlers[result] = handler;
    });
    return result;
}

void EntityItem::deregisterChangeHandler(const ChangeHandlerId& changeHandlerId) {
    withWriteLock([&] {
        _changeHandlers.remove(changeHandlerId);
    });
}

void EntityItem::somethingChangedNotification() {
    auto id = getEntityItemID();
    withReadLock([&] {
        for (const auto& handler : _changeHandlers.values()) {
            handler(id);
        }
    });
}

void EntityItem::retrieveMarketplacePublicKey() {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest;
    networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QUrl requestURL = NetworkingConstants::METAVERSE_SERVER_URL();
    requestURL.setPath("/api/v1/commerce/marketplace_key");
    QJsonObject request;
    networkRequest.setUrl(requestURL);

    QNetworkReply* networkReply = NULL;
    networkReply = networkAccessManager.get(networkRequest);

    connect(networkReply, &QNetworkReply::finished, [=]() {
        QJsonObject jsonObject = QJsonDocument::fromJson(networkReply->readAll()).object();
        jsonObject = jsonObject["data"].toObject();

        if (networkReply->error() == QNetworkReply::NoError) {
            if (!jsonObject["public_key"].toString().isEmpty()) {
                EntityItem::_marketplacePublicKey = jsonObject["public_key"].toString();
                qCWarning(entities) << "Marketplace public key has been set to" << _marketplacePublicKey;
            } else {
                qCWarning(entities) << "Marketplace public key is empty!";
            }
        } else {
            qCWarning(entities) << "Call to" << networkRequest.url() << "failed! Error:" << networkReply->error();
        }

        networkReply->deleteLater();
    });
}

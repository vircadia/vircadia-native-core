//
//  AvatarHashMap.cpp
//  libraries/avatars/src
//
//  Created by Andrew Meadows on 1/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarHashMap.h"

#include <QtCore/QDataStream>

#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "AvatarLogging.h"
#include "AvatarTraits.h"

const int SURROGATE_COUNT = 2;

AvatarHashMap::AvatarHashMap() {
    auto nodeList = DependencyManager::get<NodeList>();

    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::BulkAvatarData, this, "processAvatarDataPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, this, "processKillAvatar");
    packetReceiver.registerListener(PacketType::AvatarIdentity, this, "processAvatarIdentityPacket");
    packetReceiver.registerListener(PacketType::BulkAvatarTraits, this, "processBulkAvatarTraits");

    connect(nodeList.data(), &NodeList::uuidChanged, this, &AvatarHashMap::sessionUUIDChanged);
}

QVector<QUuid> AvatarHashMap::getAvatarIdentifiers() {
    QReadLocker locker(&_hashLock);
    return _avatarHash.keys().toVector();
}

QVector<QUuid> AvatarHashMap::getAvatarsInRange(const glm::vec3& position, float rangeMeters) const {
    auto hashCopy = getHashCopy();
    QVector<QUuid> avatarsInRange;
    auto rangeMetersSquared = rangeMeters * rangeMeters;
    for (const AvatarSharedPointer& sharedAvatar : hashCopy) {
        glm::vec3 avatarPosition = sharedAvatar->getWorldPosition();
        auto distanceSquared = glm::distance2(avatarPosition, position);
        if (distanceSquared < rangeMetersSquared) {
            avatarsInRange.push_back(sharedAvatar->getSessionUUID());
        }
    }
    return avatarsInRange;
}

bool AvatarHashMap::isAvatarInRange(const glm::vec3& position, const float range) {
    auto hashCopy = getHashCopy();
    foreach(const AvatarSharedPointer& sharedAvatar, hashCopy) {
        glm::vec3 avatarPosition = sharedAvatar->getWorldPosition();
        float distance = glm::distance(avatarPosition, position);
        if (distance < range) {
            return true;
        }
    }
    return false;
}

int AvatarHashMap::numberOfAvatarsInRange(const glm::vec3& position, float rangeMeters) {
    auto hashCopy = getHashCopy();
    auto rangeMeters2 = rangeMeters * rangeMeters;
    int count = 0;
    for (const AvatarSharedPointer& sharedAvatar : hashCopy) {
        glm::vec3 avatarPosition = sharedAvatar->getWorldPosition();
        auto distance2 = glm::distance2(avatarPosition, position);
        if (distance2 < rangeMeters2) {
            ++count;
        }
    }
    return count;
}

AvatarSharedPointer AvatarHashMap::newSharedAvatar() {
    return std::make_shared<AvatarData>();
}

AvatarSharedPointer AvatarHashMap::addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    qCDebug(avatars) << "Adding avatar with sessionUUID " << sessionUUID << "to AvatarHashMap.";

    auto avatar = newSharedAvatar();
    avatar->setSessionUUID(sessionUUID);
    avatar->setOwningAvatarMixer(mixerWeakPointer);

    // addAvatar is only called from newOrExistingAvatar, which already locks _hashLock
    _avatarHash.insert(sessionUUID, avatar);
    emit avatarAddedEvent(sessionUUID);

    return avatar;
}

AvatarSharedPointer AvatarHashMap::newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer,
    bool& isNew) {
    QWriteLocker locker(&_hashLock);
    auto avatar = _avatarHash.value(sessionUUID);
    if (!avatar) {
        avatar = addAvatar(sessionUUID, mixerWeakPointer);
        isNew = true;
    } else {
        isNew = false;
    }
    return avatar;
}

AvatarSharedPointer AvatarHashMap::findAvatar(const QUuid& sessionUUID) const {
    QReadLocker locker(&_hashLock);
    if (_avatarHash.contains(sessionUUID)) {
        return _avatarHash.value(sessionUUID);
    }
    return nullptr;
}

void AvatarHashMap::processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    PerformanceTimer perfTimer("receiveAvatar");
    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (message->getBytesLeftToRead()) {
        parseAvatarData(message, sendingNode);
    }
}

AvatarSharedPointer AvatarHashMap::parseAvatarData(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    int positionBeforeRead = message->getPosition();

    QByteArray byteArray = message->readWithoutCopy(message->getBytesLeftToRead());

    // make sure this isn't our own avatar data or for a previously ignored node
    auto nodeList = DependencyManager::get<NodeList>();

    bool isNewAvatar;
    if (sessionUUID != _lastOwnerSessionUUID && (!nodeList->isIgnoringNode(sessionUUID) || nodeList->getRequestsDomainListData())) {
        auto avatar = newOrExistingAvatar(sessionUUID, sendingNode, isNewAvatar);
        
        
        if (isNewAvatar) {
            QWriteLocker locker(&_hashLock);
            _pendingAvatars.insert(sessionUUID, { std::chrono::steady_clock::now(), 0, avatar });
            std::vector<QUuid> surrogateIDs;
            for (int i = 0; i < SURROGATE_COUNT; i++) {
                QUuid surrogateID = QUuid::createUuid();
                surrogateIDs.push_back(surrogateID);
                auto surrogateAvatar = addAvatar(surrogateID, sendingNode);
                surrogateAvatar->setSurrogateIndex(i + 1);
                surrogateAvatar->parseDataFromBuffer(byteArray);
                _pendingAvatars.insert(surrogateID, { std::chrono::steady_clock::now(), 0, surrogateAvatar });
            }
            _surrogates.insert(std::pair<QUuid, std::vector<QUuid>>(sessionUUID, surrogateIDs));
        } else {
            auto surrogateIDs = _surrogates[sessionUUID];
            for (auto id : surrogateIDs) {
                auto surrogateAvatar = newOrExistingAvatar(id, sendingNode, isNewAvatar);
                if (!isNewAvatar) {
                    surrogateAvatar->parseDataFromBuffer(byteArray);
                }
            }

        }

        // have the matching (or new) avatar parse the data from the packet
        int bytesRead = avatar->parseDataFromBuffer(byteArray);
        message->seek(positionBeforeRead + bytesRead);

        avatar->parseDataFromBuffer(byteArray);
        return avatar;
    } else {
        // create a dummy AvatarData class to throw this data on the ground
        AvatarData dummyData;
        int bytesRead = dummyData.parseDataFromBuffer(byteArray);
        message->seek(positionBeforeRead + bytesRead);
        return std::make_shared<AvatarData>();
    }
}

void AvatarHashMap::processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {

    // peek the avatar UUID from the incoming packet
    QUuid identityUUID = QUuid::fromRfc4122(message->peek(NUM_BYTES_RFC4122_UUID));

    if (identityUUID.isNull()) {
        qCDebug(avatars) << "Refusing to process identity packet for null avatar ID";
        return;
    }

    // make sure this isn't for an ignored avatar
    auto nodeList = DependencyManager::get<NodeList>();
    static auto EMPTY = QUuid();

    {
        QReadLocker locker(&_hashLock);
        _pendingAvatars.remove(identityUUID);
        auto me = _avatarHash.find(EMPTY);
        if ((me != _avatarHash.end()) && (identityUUID == me.value()->getSessionUUID())) {
            // We add MyAvatar to _avatarHash with an empty UUID. Code relies on this. In order to correctly handle an
            // identity packet for ourself (such as when we are assigned a sessionDisplayName by the mixer upon joining),
            // we make things match here.
            identityUUID = EMPTY;
        }
    }
    
    if (!nodeList->isIgnoringNode(identityUUID) || nodeList->getRequestsDomainListData()) {
        // mesh URL for a UUID, find avatar in our list
        bool isNewAvatar;
        auto avatar = newOrExistingAvatar(identityUUID, sendingNode, isNewAvatar);
        bool identityChanged = false;
        bool displayNameChanged = false;
        // In this case, the "sendingNode" is the Avatar Mixer.
        avatar->processAvatarIdentity(message->getMessage(), identityChanged, displayNameChanged);
        auto surrogateIDs = _surrogates[identityUUID];
        for (auto id : surrogateIDs) {
            auto surrogateAvatar = newOrExistingAvatar(id, sendingNode, isNewAvatar);
            if (!isNewAvatar) {
                surrogateAvatar->processAvatarIdentity(message->getMessage(), identityChanged, displayNameChanged);
            }
        }
    }
}

void AvatarHashMap::processAvatarTraits(QUuid sessionUUID, QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    message->seek(0);
    while (message->getBytesLeftToRead()) {
        // read the avatar ID to figure out which avatar this is for
        auto avatarID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));
        avatarID = sessionUUID;

        // grab the avatar so we can ask it to process trait data
        bool isNewAvatar;
        auto avatar = newOrExistingAvatar(avatarID, sendingNode, isNewAvatar);

        // read the first trait type for this avatar
        AvatarTraits::TraitType traitType;
        message->readPrimitive(&traitType);

        // grab the last trait versions for this avatar
        auto& lastProcessedVersions = _processedTraitVersions[avatarID];

        while (traitType != AvatarTraits::NullTrait) {
            AvatarTraits::TraitVersion packetTraitVersion;
            message->readPrimitive(&packetTraitVersion);

            AvatarTraits::TraitWireSize traitBinarySize;
            bool skipBinaryTrait = false;


            if (AvatarTraits::isSimpleTrait(traitType)) {
                message->readPrimitive(&traitBinarySize);

                // check if this trait version is newer than what we already have for this avatar
                if (packetTraitVersion > lastProcessedVersions[traitType]) {
                    avatar->processTrait(traitType, message->read(traitBinarySize));
                    lastProcessedVersions[traitType] = packetTraitVersion;
                }
                else {
                    skipBinaryTrait = true;
                }
            }
            else {
                AvatarTraits::TraitInstanceID traitInstanceID =
                    QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

                message->readPrimitive(&traitBinarySize);

                auto& processedInstanceVersion = lastProcessedVersions.getInstanceValueRef(traitType, traitInstanceID);
                if (packetTraitVersion > processedInstanceVersion) {
                    if (traitBinarySize == AvatarTraits::DELETED_TRAIT_SIZE) {
                        avatar->processDeletedTraitInstance(traitType, traitInstanceID);
                    }
                    else {
                        avatar->processTraitInstance(traitType, traitInstanceID, message->read(traitBinarySize));
                    }
                    processedInstanceVersion = packetTraitVersion;
                }
                else {
                    skipBinaryTrait = true;
                }
            }

            if (skipBinaryTrait) {
                // we didn't read this trait because it was older or because we didn't have an avatar to process it for
                message->seek(message->getPosition() + traitBinarySize);
            }

            // read the next trait type, which is null if there are no more traits for this avatar
            message->readPrimitive(&traitType);
        }
    }
}

void AvatarHashMap::processBulkAvatarTraits(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    while (message->getBytesLeftToRead()) {
        // read the avatar ID to figure out which avatar this is for
        auto avatarID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));
        processAvatarTraits(avatarID, message, sendingNode);
        auto surrogateIDs = _surrogates[avatarID];
        for (auto id : surrogateIDs) {
            processAvatarTraits(id, message, sendingNode);
        }
    }
}

void AvatarHashMap::processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // read the node id
    QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    KillAvatarReason reason;
    message->readPrimitive(&reason);
    removeAvatar(sessionUUID, reason);
    auto surrogateIDs = _surrogates[sessionUUID];
    for (auto id : surrogateIDs) {
        removeAvatar(id, reason);
    }
}

void AvatarHashMap::removeAvatar(const QUuid& sessionUUID, KillAvatarReason removalReason) {
    QWriteLocker locker(&_hashLock);

    _pendingAvatars.remove(sessionUUID);
    auto removedAvatar = _avatarHash.take(sessionUUID);

    if (removedAvatar) {
        handleRemovedAvatar(removedAvatar, removalReason);
    }
    auto surrogateIDs = _surrogates[sessionUUID];
    for (auto id : surrogateIDs) {
        _pendingAvatars.remove(id);
        auto removedSurrogate = _avatarHash.take(id);
        if (removedSurrogate) {
            handleRemovedAvatar(removedSurrogate, removalReason);
        }
    }
}

void AvatarHashMap::handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason) {
    // remove any information about processed traits for this avatar
    _processedTraitVersions.erase(removedAvatar->getID());

    qCDebug(avatars) << "Removed avatar with UUID" << uuidStringWithoutCurlyBraces(removedAvatar->getSessionUUID())
        << "from AvatarHashMap" << removalReason;
    emit avatarRemovedEvent(removedAvatar->getSessionUUID());
}

void AvatarHashMap::sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID) {
    _lastOwnerSessionUUID = oldUUID;
    emit avatarSessionChangedEvent(sessionUUID, oldUUID);
}


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

#include "Profile.h"

static const QString VERIFY_FAIL_MODEL { "/meshes/verifyFailed.fst" };

void AvatarReplicas::addReplica(const QUuid& parentID, AvatarSharedPointer replica) {
    if (parentID == QUuid()) {
        return;
    }
    if (_replicasMap.find(parentID) == _replicasMap.end()) {
        std::vector<AvatarSharedPointer> emptyReplicas = std::vector<AvatarSharedPointer>();
        _replicasMap.insert(std::pair<QUuid, std::vector<AvatarSharedPointer>>(parentID, emptyReplicas));
    }
    auto &replicas = _replicasMap[parentID];
    replica->setReplicaIndex((int)replicas.size() + 1);
    replicas.push_back(replica);
}

std::vector<QUuid> AvatarReplicas::getReplicaIDs(const QUuid& parentID) {
    std::vector<QUuid> ids;
    if (_replicasMap.find(parentID) != _replicasMap.end()) {
        auto &replicas = _replicasMap[parentID];
        for (int i = 0; i < (int)replicas.size(); i++) {
            ids.push_back(replicas[i]->getID());
        }
    } else if (_replicaCount > 0) {
        for (int i = 0; i < _replicaCount; i++) {
            ids.push_back(QUuid::createUuid());
        }
    }
    return ids;
}

void AvatarReplicas::parseDataFromBuffer(const QUuid& parentID, const QByteArray& buffer) {
    if (_replicasMap.find(parentID) != _replicasMap.end()) {
        auto &replicas = _replicasMap[parentID];
        for (auto avatar : replicas) {
            avatar->parseDataFromBuffer(buffer);
        }
    }
}

void AvatarReplicas::removeReplicas(const QUuid& parentID) {
    if (_replicasMap.find(parentID) != _replicasMap.end()) {
        _replicasMap.erase(parentID);
    }
}

std::vector<AvatarSharedPointer> AvatarReplicas::takeReplicas(const QUuid& parentID) {
    std::vector<AvatarSharedPointer> replicas;

    auto it = _replicasMap.find(parentID);

    if (it != _replicasMap.end()) {
        // take a copy of the replica shared pointers for this parent
        replicas.swap(it->second);

        // erase the replicas for this parent from our map
        _replicasMap.erase(it);
    }

    return replicas;
}

void AvatarReplicas::processAvatarIdentity(const QUuid& parentID, const QByteArray& identityData, bool& identityChanged, bool& displayNameChanged) {
    if (_replicasMap.find(parentID) != _replicasMap.end()) {
        auto &replicas = _replicasMap[parentID];
        QDataStream identityDataStream(identityData);
        for (auto avatar : replicas) {
            avatar->processAvatarIdentity(identityDataStream, identityChanged, displayNameChanged);
        }
    }
}
void AvatarReplicas::processTrait(const QUuid& parentID, AvatarTraits::TraitType traitType, QByteArray traitBinaryData) {
    if (_replicasMap.find(parentID) != _replicasMap.end()) {
        auto &replicas = _replicasMap[parentID];
        for (auto avatar : replicas) {
            avatar->processTrait(traitType, traitBinaryData);
        }
    }
}
void AvatarReplicas::processDeletedTraitInstance(const QUuid& parentID, AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID instanceID) {
    if (_replicasMap.find(parentID) != _replicasMap.end()) {
        auto &replicas = _replicasMap[parentID];
        for (auto avatar : replicas) {
            avatar->processDeletedTraitInstance(traitType, instanceID);
        }
    }
}
void AvatarReplicas::processTraitInstance(const QUuid& parentID, AvatarTraits::TraitType traitType,
    AvatarTraits::TraitInstanceID instanceID, QByteArray traitBinaryData) {
    if (_replicasMap.find(parentID) != _replicasMap.end()) {
        auto &replicas = _replicasMap[parentID];
        for (auto avatar : replicas) {
            avatar->processTraitInstance(traitType, instanceID, traitBinaryData);
        }
    }
}

AvatarHashMap::AvatarHashMap() {
    auto nodeList = DependencyManager::get<NodeList>();

    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::BulkAvatarData,
        PacketReceiver::makeSourcedListenerReference<AvatarHashMap>(this, &AvatarHashMap::processAvatarDataPacket));
    packetReceiver.registerListener(PacketType::KillAvatar,
        PacketReceiver::makeSourcedListenerReference<AvatarHashMap>(this, &AvatarHashMap::processKillAvatar));
    packetReceiver.registerListener(PacketType::AvatarIdentity,
        PacketReceiver::makeSourcedListenerReference<AvatarHashMap>(this, &AvatarHashMap::processAvatarIdentityPacket));
    packetReceiver.registerListener(PacketType::BulkAvatarTraits,
        PacketReceiver::makeSourcedListenerReference<AvatarHashMap>(this, &AvatarHashMap::processBulkAvatarTraits));

    connect(nodeList.data(), &NodeList::uuidChanged, this, &AvatarHashMap::sessionUUIDChanged);

    connect(nodeList.data(), &NodeList::nodeKilled, this, [this](SharedNodePointer killedNode){
        if (killedNode->getType() == NodeType::AvatarMixer) {
            clearOtherAvatars();
        }
    });
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

void AvatarHashMap::setReplicaCount(int count) {
    _replicas.setReplicaCount(count);
    auto avatars = getAvatarIdentifiers();
    for (int i = 0; i < avatars.size(); i++) {
        KillAvatarReason reason = KillAvatarReason::NoReason;
        if (avatars[i] != QUuid()) {
            removeAvatar(avatars[i], reason);
            auto replicaIDs = _replicas.getReplicaIDs(avatars[i]);
            for (auto id : replicaIDs) {
                removeAvatar(id, reason);
            }
        }
    }
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

AvatarSharedPointer AvatarHashMap::newSharedAvatar(const QUuid& sessionUUID) {
    auto avatarData = std::make_shared<AvatarData>();
    avatarData->setSessionUUID(sessionUUID);
    return avatarData;
}

AvatarSharedPointer AvatarHashMap::addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    qCDebug(avatars) << "Adding avatar with sessionUUID " << sessionUUID << "to AvatarHashMap.";

    auto avatar = newSharedAvatar(sessionUUID);
    avatar->setSessionUUID(sessionUUID);
    avatar->setOwningAvatarMixer(mixerWeakPointer);

    {
        QWriteLocker locker(&_hashLock);
        _avatarHash.insert(sessionUUID, avatar);
    }
    emit avatarAddedEvent(sessionUUID);
    return avatar;
}

AvatarSharedPointer AvatarHashMap::newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer, bool& isNew) {
    auto avatar = findAvatar(sessionUUID);
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
    auto avatarIter = _avatarHash.find(sessionUUID);
    if (avatarIter != _avatarHash.end()) {
        return avatarIter.value();
    }
    return nullptr;
}

void AvatarHashMap::processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    DETAILED_PROFILE_RANGE(network, __FUNCTION__);
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
            avatar->setIsNewAvatar(true);
            auto replicaIDs = _replicas.getReplicaIDs(sessionUUID);
            for (auto replicaID : replicaIDs) {
                auto replicaAvatar = addAvatar(replicaID, sendingNode);
                replicaAvatar->setIsNewAvatar(true);
                _replicas.addReplica(sessionUUID, replicaAvatar);
            }
        } 
        
        // have the matching (or new) avatar parse the data from the packet
        int bytesRead = avatar->parseDataFromBuffer(byteArray);
        message->seek(positionBeforeRead + bytesRead);
        _replicas.parseDataFromBuffer(sessionUUID, byteArray);
        

        return avatar;
    } else {
        // Shouldn't happen if mixer functioning correctly - debugging for BUGZ-781:
        qCDebug(avatars) << "Discarding received avatar data" << sessionUUID << (sessionUUID == _lastOwnerSessionUUID ? "(is self)" : "")
            << "isIgnoringNode = " << nodeList->isIgnoringNode(sessionUUID);

        // create a dummy AvatarData class to throw this data on the ground
        AvatarData dummyData;
        int bytesRead = dummyData.parseDataFromBuffer(byteArray);
        message->seek(positionBeforeRead + bytesRead);
        return std::make_shared<AvatarData>();
    }
}

void AvatarHashMap::processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    QDataStream avatarIdentityStream(message->getMessage());

    while (!avatarIdentityStream.atEnd()) {
        // peek the avatar UUID from the incoming packet
        avatarIdentityStream.startTransaction();
        QUuid identityUUID;
        avatarIdentityStream >> identityUUID;
        avatarIdentityStream.rollbackTransaction();

        if (identityUUID.isNull()) {
            qCDebug(avatars) << "Refusing to process identity packet for null avatar ID";
            return;
        }

        // make sure this isn't for an ignored avatar
        auto nodeList = DependencyManager::get<NodeList>();
        static auto EMPTY = QUuid();

        {
            QReadLocker locker(&_hashLock);
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
            avatar->processAvatarIdentity(avatarIdentityStream, identityChanged, displayNameChanged);
            _replicas.processAvatarIdentity(identityUUID, message->getMessage(), identityChanged, displayNameChanged);
        }
    }
}

void AvatarHashMap::processBulkAvatarTraits(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    AvatarTraits::TraitMessageSequence seq;

    // Trying to read more bytes than available, bail
    if (message->getBytesLeftToRead() < (qint64)sizeof(AvatarTraits::TraitMessageSequence)) {
        qWarning() << "Malformed bulk trait packet, bailling";
        return;
    }

    message->readPrimitive(&seq);

    auto traitsAckPacket = NLPacket::create(PacketType::BulkAvatarTraitsAck, sizeof(AvatarTraits::TraitMessageSequence), true);
    traitsAckPacket->writePrimitive(seq);
    auto nodeList = DependencyManager::get<LimitedNodeList>();
    SharedNodePointer avatarMixer = nodeList->soloNodeOfType(NodeType::AvatarMixer);
    if (!avatarMixer.isNull()) {
        // we have a mixer to send to, acknowledge that we received these
        // traits.
        nodeList->sendPacket(std::move(traitsAckPacket), *avatarMixer);
    }

    while (message->getBytesLeftToRead() > 0) {
        // Trying to read more bytes than available, bail
        if (message->getBytesLeftToRead() < qint64(NUM_BYTES_RFC4122_UUID +
                                                   sizeof(AvatarTraits::TraitType))) {
            qWarning() << "Malformed bulk trait packet, bailling";
            return;
        }

        // read the avatar ID to figure out which avatar this is for
        auto avatarID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        // grab the avatar so we can ask it to process trait data
        bool isNewAvatar;
        auto avatar = newOrExistingAvatar(avatarID, sendingNode, isNewAvatar);

        // read the first trait type for this avatar
        AvatarTraits::TraitType traitType;
        message->readPrimitive(&traitType);

        // grab the last trait versions for this avatar
        auto& lastProcessedVersions = _processedTraitVersions[avatarID];

        while (traitType != AvatarTraits::NullTrait && message->getBytesLeftToRead() > 0) {
            // Trying to read more bytes than available, bail
            if (message->getBytesLeftToRead() < qint64(sizeof(AvatarTraits::TraitVersion))) {
                qWarning() << "Malformed bulk trait packet, bailling";
                return;
            }

            AvatarTraits::TraitVersion packetTraitVersion;
            message->readPrimitive(&packetTraitVersion);

            AvatarTraits::TraitWireSize traitBinarySize;
            bool skipBinaryTrait = false;

            if (AvatarTraits::isSimpleTrait(traitType)) {
                // Trying to read more bytes than available, bail
                if (message->getBytesLeftToRead() < qint64(sizeof(AvatarTraits::TraitWireSize))) {
                    qWarning() << "Malformed bulk trait packet, bailling";
                    return;
                }

                message->readPrimitive(&traitBinarySize);

                // Trying to read more bytes than available, bail
                if (message->getBytesLeftToRead() < traitBinarySize) {
                    qWarning() << "Malformed bulk trait packet, bailling";
                    return;
                }

                // check if this trait version is newer than what we already have for this avatar
                if (packetTraitVersion > lastProcessedVersions[traitType]) {
                    auto traitData = message->read(traitBinarySize);
                    avatar->processTrait(traitType, traitData);
                    _replicas.processTrait(avatarID, traitType, traitData);
                    lastProcessedVersions[traitType] = packetTraitVersion;
                } else {
                    skipBinaryTrait = true;
                }
            } else {
                // Trying to read more bytes than available, bail
                if (message->getBytesLeftToRead() < qint64(NUM_BYTES_RFC4122_UUID +
                                                           sizeof(AvatarTraits::TraitWireSize))) {
                    qWarning() << "Malformed bulk trait packet, bailling";
                    return;
                }

                AvatarTraits::TraitInstanceID traitInstanceID =
                    QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

                message->readPrimitive(&traitBinarySize);

                // Trying to read more bytes than available, bail
                if (traitBinarySize < -1 || message->getBytesLeftToRead() < traitBinarySize) {
                    qWarning() << "Malformed bulk trait packet, bailling";
                    return;
                }

                auto& processedInstanceVersion = lastProcessedVersions.getInstanceValueRef(traitType, traitInstanceID);
                if (packetTraitVersion > processedInstanceVersion) {
                    if (traitBinarySize == AvatarTraits::DELETED_TRAIT_SIZE) {
                        avatar->processDeletedTraitInstance(traitType, traitInstanceID);
                        _replicas.processDeletedTraitInstance(avatarID, traitType, traitInstanceID);
                    } else {
                        auto traitData = message->read(traitBinarySize);
                        avatar->processTraitInstance(traitType, traitInstanceID, traitData);
                        _replicas.processTraitInstance(avatarID, traitType, traitInstanceID, traitData);
                    }
                    processedInstanceVersion = packetTraitVersion;
                } else {
                    skipBinaryTrait = true;
                }
            }

            if (skipBinaryTrait && traitBinarySize > 0) {
                // we didn't read this trait because it was older or because we didn't have an avatar to process it for
                message->seek(message->getPosition() + traitBinarySize);
            }

            // read the next trait type, which is null if there are no more traits for this avatar
            message->readPrimitive(&traitType);
        }
    }
}

void AvatarHashMap::processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // read the node id
    QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    KillAvatarReason reason;
    message->readPrimitive(&reason);
    removeAvatar(sessionUUID, reason);
    auto replicaIDs = _replicas.getReplicaIDs(sessionUUID);
    for (auto id : replicaIDs) {
        removeAvatar(id, reason);
    }
}

void AvatarHashMap::removeAvatar(const QUuid& sessionUUID, KillAvatarReason removalReason) {
    std::vector<AvatarSharedPointer> removedAvatars;

    {
        QWriteLocker locker(&_hashLock);

        auto replicas = _replicas.takeReplicas(sessionUUID);

        for (auto& replica : replicas) {
            auto removedReplica = _avatarHash.take(replica->getID());
            if (removedReplica) {
                removedAvatars.push_back(removedReplica);
            }
        }

        auto removedAvatar = _avatarHash.take(sessionUUID);
        if (removedAvatar) {
            removedAvatars.push_back(removedAvatar);
        }
    }

    for (auto& removedAvatar: removedAvatars) {
        handleRemovedAvatar(removedAvatar, removalReason);
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

void AvatarHashMap::clearOtherAvatars() {
    QList<AvatarSharedPointer> removedAvatars;

    {
        QWriteLocker locker(&_hashLock);

        // grab a copy of the current avatars so we can call handleRemoveAvatar for them
        removedAvatars = _avatarHash.values();

        _avatarHash.clear();
    }

    for (auto& av : removedAvatars) {
        handleRemovedAvatar(av);
    }
}

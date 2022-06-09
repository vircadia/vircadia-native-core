//
//  AvatarPacketHandler.hpp
//  libraries/avatars-core/src
//
//  Created by Nshan G. on 11 May 2022.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_AVATARS_CORE_SRC_AVATARPACKETHANDLER_HPP
#define LIBRARIES_AVATARS_CORE_SRC_AVATARPACKETHANDLER_HPP

#include "AvatarPacketHandler.h"

#include <chrono>

#include <QtCore/QDataStream>

#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <PerfStat.h>
#include <SharedUtil.h>

#include "AvatarLogging.h"
#include "AvatarTraits.h"

#include "Profile.h"

template <typename Derived, typename AvatarPtr>
Derived& AvatarPacketHandler<Derived, AvatarPtr>::derived() {
    return *static_cast<Derived*>(this);
}

template <typename Derived, typename AvatarPtr>
const Derived& AvatarPacketHandler<Derived, AvatarPtr>::derived() const {
    return *static_cast<const Derived*>(this);
}

template <typename Derived, typename AvatarPtr>
AvatarPtr AvatarPacketHandler<Derived, AvatarPtr>::getAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixer, bool& isNew) {
    AvatarPtr avatar = derived().newOrExistingAvatar(sessionUUID, mixer, isNew);
    if (isNew) {
        // FIXME: CRTP, derived sets this multiple times, but with this it's no longer necessary
        avatar->setSessionUUID(sessionUUID);
    }
    return avatar;
}

template <typename Derived, typename AvatarPtr>
AvatarPacketHandler<Derived, AvatarPtr>::AvatarPacketHandler() {
    auto nodeList = DependencyManager::get<NodeList>();

    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::BulkAvatarData,
        PacketReceiver::makeSourcedListenerReference<Derived>(&derived(), &AvatarPacketHandler::processAvatarDataPacket));
    packetReceiver.registerListener(PacketType::KillAvatar,
        PacketReceiver::makeSourcedListenerReference<Derived>(&derived(), &AvatarPacketHandler::processKillAvatar));
    packetReceiver.registerListener(PacketType::AvatarIdentity,
        PacketReceiver::makeSourcedListenerReference<Derived>(&derived(), &AvatarPacketHandler::processAvatarIdentityPacket));
    packetReceiver.registerListener(PacketType::BulkAvatarTraits,
        PacketReceiver::makeSourcedListenerReference<Derived>(&derived(), &AvatarPacketHandler::processBulkAvatarTraits));

    derived().connect(nodeList.data(), &NodeList::uuidChanged, &derived(), [this](const QUuid& sessionUUID, const QUuid& oldUUID){
        sessionUUIDChanged(sessionUUID, oldUUID);
    });

    derived().connect(nodeList.data(), &NodeList::nodeKilled, &derived(), [this](SharedNodePointer killedNode){
        if (killedNode->getType() == NodeType::AvatarMixer) {
            derived().clearOtherAvatars();
        }
    });

    derived().connect(nodeList.data(), &NodeList::nodeActivated, &derived(), [this](SharedNodePointer node){
        if (node->getType() == NodeType::AvatarMixer) {
            queryExpiry = std::chrono::steady_clock::now();
            derived().onAvatarMixerActivated();
        }
    });

    derived().connect(nodeList.data(), &NodeList::ignoredNode, &derived(), [this](const QUuid& nodeID, bool enabled) {
        if (enabled) {
            processKillAvatar(nodeID, KillAvatarReason::AvatarIgnored);
        }
        derived().onNodeIngored(nodeID, enabled);
        //FIXME: CRTP interface/src/avatar/AvatarManager has similar ignored node handler with this additional logic
        // if (!enabled) {
        //     auto avatar = std::static_pointer_cast<Avatar>(getAvatarBySessionID(nodeID));
        //     if (avatar) {
        //         avatar->createOrb();
        //     }
        // }
    });

}

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    DETAILED_PROFILE_RANGE(network, __FUNCTION__);
    PerformanceTimer perfTimer("receiveAvatar");
    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (message->getBytesLeftToRead()) {
        parseAvatarData(message, sendingNode);
    }
}

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::parseAvatarData(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    int positionBeforeRead = message->getPosition();

    QByteArray byteArray = message->readWithoutCopy(message->getBytesLeftToRead());

    // make sure this isn't our own avatar data or for a previously ignored node
    auto nodeList = DependencyManager::get<NodeList>();
    bool isNewAvatar;
    if (sessionUUID != _lastOwnerSessionUUID && (!nodeList->isIgnoringNode(sessionUUID) || nodeList->getRequestsDomainListData())) {

        AvatarPtr avatar = getAvatar(sessionUUID, sendingNode, isNewAvatar);

        // FIXME: CRTP move to Derived::newOrExistingAvatar
        // if (isNewAvatar) {
        //     QWriteLocker locker(&_hashLock);
        //     avatar->setIsNewAvatar(true);
        //     auto replicaIDs = _replicas.getReplicaIDs(sessionUUID);
        //     for (auto replicaID : replicaIDs) {
        //         auto replicaAvatar = addAvatar(replicaID, sendingNode);
        //         replicaAvatar->setIsNewAvatar(true);
        //         _replicas.addReplica(sessionUUID, replicaAvatar);
        //     }
        // }

        // have the matching (or new) avatar parse the data from the packet
        int bytesRead = avatar->parseDataFromBuffer(byteArray);
        derived().onAvatarDataReceived(sessionUUID, byteArray);

        // FIXME: CRTP move to Derived::onAvatarDataReceived
        // _replicas.parseDataFromBuffer(sessionUUID, byteArray);

        message->seek(positionBeforeRead + bytesRead);
    } else {
        // Shouldn't happen if mixer functioning correctly - debugging for BUGZ-781:
        qCDebug(avatars) << "Discarding received avatar data" << sessionUUID << (sessionUUID == _lastOwnerSessionUUID ? "(is self)" : "")
            << "isIgnoringNode = " << nodeList->isIgnoringNode(sessionUUID);

        // create a dummy AvatarData class to throw this data on the ground
        typename std::pointer_traits<AvatarPtr>::element_type dummyData;
        int bytesRead = dummyData.parseDataFromBuffer(byteArray);
        message->seek(positionBeforeRead + bytesRead);
    }
}

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
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

        identityUUID = derived().mapIdentityUUID(identityUUID);

        // FIXME: CRTP Derived::mapIdentityUUID
        // {
        //     static auto EMPTY = QUuid();
        //     QReadLocker locker(&_hashLock);
        //     auto me = _avatarHash.find(EMPTY);
        //     if ((me != _avatarHash.end()) && (identityUUID == me.value()->getSessionUUID())) {
        //         // We add MyAvatar to _avatarHash with an empty UUID. Code relies on this. In order to correctly handle an
        //         // identity packet for ourself (such as when we are assigned a sessionDisplayName by the mixer upon joining),
        //         // we make things match here.
        //         identityUUID = EMPTY;
        //     }
        // }

        if (!nodeList->isIgnoringNode(identityUUID) || nodeList->getRequestsDomainListData()) {
            // mesh URL for a UUID, find avatar in our list
            bool isNewAvatar;
            AvatarPtr avatar = getAvatar(identityUUID, sendingNode, isNewAvatar);
            bool identityChanged = false;
            bool displayNameChanged = false;
            // In this case, the "sendingNode" is the Avatar Mixer.
            avatar->processAvatarIdentity(avatarIdentityStream, identityChanged, displayNameChanged);

            derived().onAvatarIdentityReceived(identityUUID, message->getMessage());

            // FIXME: CRTP move to Derived::onAvatarIdentityReceived
            // _replicas.processAvatarIdentity(identityUUID, message->getMessage(), identityChanged, displayNameChanged);
        }
    }
}

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::processBulkAvatarTraits(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
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
        AvatarPtr avatar = getAvatar(avatarID, sendingNode, isNewAvatar);

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

                    // FIXME::CRTP
                    // _replicas.processTrait(avatarID, traitType, traitData);

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
                        // FIXME::CRTP
                        // _replicas.processDeletedTraitInstance(avatarID, traitType, traitInstanceID);
                    } else {
                        auto traitData = message->read(traitBinarySize);
                        avatar->processTraitInstance(traitType, traitInstanceID, traitData);
                        // FIXME::CRTP
                        // _replicas.processTraitInstance(avatarID, traitType, traitInstanceID, traitData);
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

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // read the node id
    QUuid sessionUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    KillAvatarReason reason;
    message->readPrimitive(&reason);

    processKillAvatar(sessionUUID, reason);
}

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::processKillAvatar(const QUuid& sessionUUID, KillAvatarReason reason) {
    // remove any information about processed traits for this avatar
    // TODO: CRTP removeAvatar on derived returns nothing, this expects it to return the removed avatar (not replicas)
    auto removed = derived().removeAvatar(sessionUUID, reason);
    if (removed) {
        _processedTraitVersions.erase(removed->getSessionUUID());
    }
}

// FIXME: move to Derived, some code was removed in comparison to what's there now
// void AvatarHashMap::handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason) {
//     qCDebug(avatars) << "Removed avatar with UUID" << uuidStringWithoutCurlyBraces(removedAvatar->getSessionUUID())
//         << "from AvatarHashMap" << removalReason;
//     emit avatarRemovedEvent(removedAvatar->getSessionUUID());
// }

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID) {
    _lastOwnerSessionUUID = oldUUID;

    derived().onSessionUUIDChanged(sessionUUID, oldUUID);
    // FIXME: CRTP
    // emit avatarSessionChangedEvent(sessionUUID, oldUUID);
}

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::clearOtherAvatars() {
    _processedTraitVersions.clear();
    // FIXME: CRTP, virtual overridden in AvatarManager without calling base, but this should be called
    // QList<AvatarSharedPointer> removedAvatars;
    //
    // {
    //     QWriteLocker locker(&_hashLock);
    //
    //     // grab a copy of the current avatars so we can call handleRemoveAvatar for them
    //     removedAvatars = _avatarHash.values();
    //
    //     _avatarHash.clear();
    // }
    //
    // for (auto& av : removedAvatars) {
    //     handleRemovedAvatar(av);
    // }
}

template <typename Derived, typename AvatarPtr>
void AvatarPacketHandler<Derived, AvatarPtr>::query() {
    bool viewIsDifferentEnough = false;
    auto& views = derived().getViews();
    auto& lastQueriedViews = derived().getLastQueriedViews();
    if (views.size() == lastQueriedViews.size()) {
        for (size_t i = 0; i < views.size(); ++i) {
            if (!views[i].isVerySimilar(lastQueriedViews[i])) {
                viewIsDifferentEnough = true;
                break;
            }
        }
    } else {
        viewIsDifferentEnough = true;
    }

    // if it's been a while since our last query or the view has significantly changed then send a query, otherwise suppress it
    static const std::chrono::seconds MIN_PERIOD_BETWEEN_QUERIES { 3 };
    auto now = std::chrono::steady_clock::now();
    if (now > queryExpiry || viewIsDifferentEnough) {
        auto avatarPacket = NLPacket::create(PacketType::AvatarQuery);
        auto destinationBuffer = reinterpret_cast<unsigned char*>(avatarPacket->getPayload());
        unsigned char* bufferStart = destinationBuffer;

        uint8_t numFrustums = (uint8_t)views.size();
        memcpy(destinationBuffer, &numFrustums, sizeof(numFrustums));
        destinationBuffer += sizeof(numFrustums);

        for (const auto& view : views) {
            destinationBuffer += view.serialize(destinationBuffer);
        }

        avatarPacket->setPayloadSize(destinationBuffer - bufferStart);

        DependencyManager::get<NodeList>()->broadcastToNodes(std::move(avatarPacket), NodeSet() << NodeType::AvatarMixer);

        lastQueriedViews = views;
        queryExpiry = now + MIN_PERIOD_BETWEEN_QUERIES;
    }
}

#endif /* end of include guard */

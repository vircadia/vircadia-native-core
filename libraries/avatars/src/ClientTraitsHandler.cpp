//
//  ClientTraitsHandler.cpp
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 7/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ClientTraitsHandler.h"

#include <QtCore/QObject>

#include <NodeList.h>
#include <NLPacketList.h>

#include "AvatarData.h"

ClientTraitsHandler::ClientTraitsHandler(AvatarData* owningAvatar) :
    _owningAvatar(owningAvatar)
{
    auto nodeList = DependencyManager::get<NodeList>();
    QObject::connect(nodeList.data(), &NodeList::nodeAdded, this, [this](SharedNodePointer addedNode) {
        if (addedNode->getType() == NodeType::AvatarMixer) {
            resetForNewMixer();
        }
    });

    nodeList->getPacketReceiver().registerListener(PacketType::SetAvatarTraits,
        PacketReceiver::makeSourcedListenerReference<ClientTraitsHandler>(this, &ClientTraitsHandler::processTraitOverride));
}

void ClientTraitsHandler::markTraitUpdated(AvatarTraits::TraitType updatedTrait) {
    Lock lock(_traitLock);
    _traitStatuses[updatedTrait] = Updated;
    _hasChangedTraits = true;
}

void ClientTraitsHandler::markInstancedTraitUpdated(AvatarTraits::TraitType traitType, QUuid updatedInstanceID) {
    Lock lock(_traitLock);
    _traitStatuses.instanceInsert(traitType, updatedInstanceID, Updated);
    _hasChangedTraits = true;
}

void ClientTraitsHandler::markInstancedTraitDeleted(AvatarTraits::TraitType traitType, QUuid deleteInstanceID) {
    Lock lock(_traitLock);
    _traitStatuses.instanceInsert(traitType, deleteInstanceID, Deleted);
    _hasChangedTraits = true;
}

void ClientTraitsHandler::resetForNewMixer() {
    Lock lock(_traitLock);

    // re-set the current version to 0
    _currentTraitVersion = AvatarTraits::DEFAULT_TRAIT_VERSION;

    // mark that all traits should be sent next time
    _shouldPerformInitialSend = true;

    // reset the trait statuses
    _traitStatuses.reset();

    // pre-fill the instanced statuses that we will need to send next frame
    _owningAvatar->prepareResetTraitInstances();
}

int ClientTraitsHandler::sendChangedTraitsToMixer() {
    std::unique_lock<Mutex> lock(_traitLock);
    int bytesWritten = 0;

    if (hasChangedTraits() || _shouldPerformInitialSend) {
        // we have at least one changed trait to send

        auto nodeList = DependencyManager::get<NodeList>();
        auto avatarMixer = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        if (!avatarMixer || !avatarMixer->getActiveSocket()) {
            // we don't have an avatar mixer with an active socket, we can't send changed traits at this time
            return 0;
        }

        // we have a mixer to send to, setup our set traits packet
        auto traitsPacketList = NLPacketList::create(PacketType::SetAvatarTraits, QByteArray(), true, true);

        // bump and write the current trait version to an extended header
        // the trait version is the same for all traits in this packet list
        traitsPacketList->writePrimitive(++_currentTraitVersion);

        // take a copy of the set of changed traits and clear the stored set
        auto traitStatusesCopy { _traitStatuses };
        _traitStatuses.reset();
        _hasChangedTraits = false;

        // if this was an initial send of all traits, consider it completed
        bool initialSend = _shouldPerformInitialSend;
        _shouldPerformInitialSend = false;

        // we can release the lock here since we've taken a copy of statuses
        // and will setup the packet using the information in the copy
        lock.unlock();

        auto simpleIt = traitStatusesCopy.simpleCBegin();
        while (simpleIt != traitStatusesCopy.simpleCEnd()) {
            // because the vector contains all trait types (for access using trait type as index)
            // we double check that it is a simple iterator here
            auto traitType = static_cast<AvatarTraits::TraitType>(std::distance(traitStatusesCopy.simpleCBegin(), simpleIt));

            if (initialSend || *simpleIt == Updated) {
                bytesWritten += AvatarTraits::packTrait(traitType, *traitsPacketList, *_owningAvatar);
                
                if (traitType == AvatarTraits::SkeletonModelURL) {
                    // keep track of our skeleton version in case we get an override back
                    _currentSkeletonVersion = _currentTraitVersion;
                }
            }

            ++simpleIt;
        }

        auto instancedIt = traitStatusesCopy.instancedCBegin();
        while (instancedIt != traitStatusesCopy.instancedCEnd()) {
            for (auto& instanceIDValuePair : instancedIt->instances) {
                if ((initialSend && instanceIDValuePair.value != Deleted)
                    || instanceIDValuePair.value == Updated) {
                    // this is a changed trait we need to send or we haven't send out trait information yet
                    // ask the owning avatar to pack it
                    bytesWritten += AvatarTraits::packTraitInstance(instancedIt->traitType, instanceIDValuePair.id,
                                                                    *traitsPacketList, *_owningAvatar);

                } else if (!initialSend && instanceIDValuePair.value == Deleted) {
                    // pack delete for this trait instance
                    bytesWritten += AvatarTraits::packInstancedTraitDelete(instancedIt->traitType, instanceIDValuePair.id,
                                                           *traitsPacketList);
                }
            }

            ++instancedIt;
        }

        nodeList->sendPacketList(std::move(traitsPacketList), *avatarMixer);
    }

    return bytesWritten;
}

void ClientTraitsHandler::processTraitOverride(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    if (sendingNode->getType() == NodeType::AvatarMixer) {
        Lock lock(_traitLock);

        while (message->getBytesLeftToRead() > 0) {
            // Trying to read more bytes than available, bail
            if (message->getBytesLeftToRead() < qint64(sizeof(AvatarTraits::TraitType) +
                                                       sizeof(AvatarTraits::TraitVersion) +
                                                       sizeof(AvatarTraits::TraitWireSize))) {
                qWarning() << "Malformed trait override packet, bailling";
                return;
            }

            AvatarTraits::TraitType traitType;
            message->readPrimitive(&traitType);

            AvatarTraits::TraitVersion traitVersion;
            message->readPrimitive(&traitVersion);

            AvatarTraits::TraitWireSize traitBinarySize;
            message->readPrimitive(&traitBinarySize);

            // Trying to read more bytes than available, bail
            if (traitBinarySize < -1 || message->getBytesLeftToRead() < traitBinarySize) {
                qWarning() << "Malformed trait override packet, bailling";
                return;
            }

            // only accept an override if this is for a trait type we override
            // and the version matches what we last sent for skeleton
            if (traitType == AvatarTraits::SkeletonModelURL
                && traitVersion == _currentSkeletonVersion
                && _traitStatuses[AvatarTraits::SkeletonModelURL] != Updated) {

                // override the skeleton URL but do not mark the trait as having changed
                // so that we don't unecessarily send a new trait packet to the mixer with the overriden URL

                auto hasChangesBefore = _hasChangedTraits;

                auto traitBinaryData = message->readWithoutCopy(traitBinarySize);
                _owningAvatar->processTrait(traitType, traitBinaryData);

                // setSkeletonModelURL will flag us for changes to the SkeletonModelURL so we reset some state here to
                // avoid unnecessarily sending the overriden skeleton model URL back to the mixer
                _traitStatuses.erase(AvatarTraits::SkeletonModelURL);
                _hasChangedTraits = hasChangesBefore;
            } else {
                message->seek(message->getPosition() + traitBinarySize);
            }
        }
    }
}

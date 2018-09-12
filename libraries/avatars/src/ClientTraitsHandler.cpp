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
    QObject::connect(nodeList.data(), &NodeList::nodeAdded, [this](SharedNodePointer addedNode){
        if (addedNode->getType() == NodeType::AvatarMixer) {
            resetForNewMixer();
        }
    });

    nodeList->getPacketReceiver().registerListener(PacketType::SetAvatarTraits, this, "processTraitOverride");
}

void ClientTraitsHandler::resetForNewMixer() {
    // re-set the current version to 0
    _currentTraitVersion = AvatarTraits::DEFAULT_TRAIT_VERSION;

    // mark that all traits should be sent next time
    _shouldPerformInitialSend = true;

    // reset the trait statuses
    _traitStatuses.reset();

    // pre-fill the instanced statuses that we will need to send next frame
    _owningAvatar->prepareResetTraitInstances();
}

void ClientTraitsHandler::sendChangedTraitsToMixer() {
    if (hasChangedTraits() || _shouldPerformInitialSend) {
        // we have at least one changed trait to send

        auto nodeList = DependencyManager::get<NodeList>();
        auto avatarMixer = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        if (!avatarMixer || !avatarMixer->getActiveSocket()) {
            // we don't have an avatar mixer with an active socket, we can't send changed traits at this time
            return;
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

        auto simpleIt = traitStatusesCopy.simpleCBegin();
        while (simpleIt != traitStatusesCopy.simpleCEnd()) {
            // because the vector contains all trait types (for access using trait type as index)
            // we double check that it is a simple iterator here
            auto traitType = static_cast<AvatarTraits::TraitType>(std::distance(traitStatusesCopy.simpleCBegin(), simpleIt));

            if (_shouldPerformInitialSend || *simpleIt == Updated) {
                if (traitType == AvatarTraits::SkeletonModelURL) {
                    _owningAvatar->packTrait(traitType, *traitsPacketList);

                    // keep track of our skeleton version in case we get an override back
                    _currentSkeletonVersion = _currentTraitVersion;
                }
            }

            ++simpleIt;
        }

        auto instancedIt = traitStatusesCopy.instancedCBegin();
        while (instancedIt != traitStatusesCopy.instancedCEnd()) {
            for (auto& instanceIDValuePair : instancedIt->instances) {
                if ((_shouldPerformInitialSend && instanceIDValuePair.value != Deleted)
                    || instanceIDValuePair.value == Updated) {
                    // this is a changed trait we need to send or we haven't send out trait information yet
                    // ask the owning avatar to pack it
                    _owningAvatar->packTraitInstance(instancedIt->traitType, instanceIDValuePair.id, *traitsPacketList);
                } else if (!_shouldPerformInitialSend && instanceIDValuePair.value == Deleted) {
                    // pack delete for this trait instance
                    AvatarTraits::packInstancedTraitDelete(instancedIt->traitType, instanceIDValuePair.id,
                                                           *traitsPacketList);
                }
            }

            ++instancedIt;
        }

        nodeList->sendPacketList(std::move(traitsPacketList), *avatarMixer);

        // if this was an initial send of all traits, consider it completed
        _shouldPerformInitialSend = false;
    }
}

void ClientTraitsHandler::processTraitOverride(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    if (sendingNode->getType() == NodeType::AvatarMixer) {
        while (message->getBytesLeftToRead()) {
            AvatarTraits::TraitType traitType;
            message->readPrimitive(&traitType);

            AvatarTraits::TraitVersion traitVersion;
            message->readPrimitive(&traitVersion);

            AvatarTraits::TraitWireSize traitBinarySize;
            message->readPrimitive(&traitBinarySize);

            // only accept an override if this is for a trait type we override
            // and the version matches what we last sent for skeleton
            if (traitType == AvatarTraits::SkeletonModelURL
                && traitVersion == _currentSkeletonVersion
                && _traitStatuses[AvatarTraits::SkeletonModelURL] != Updated) {

                // override the skeleton URL but do not mark the trait as having changed
                // so that we don't unecessarily send a new trait packet to the mixer with the overriden URL
                auto encodedSkeletonURL = QUrl::fromEncoded(message->readWithoutCopy(traitBinarySize));

                auto hasChangesBefore = _hasChangedTraits;

                _owningAvatar->setSkeletonModelURL(encodedSkeletonURL);

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

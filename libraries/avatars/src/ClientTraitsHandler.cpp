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
    _performInitialSend = true;
}

void ClientTraitsHandler::sendChangedTraitsToMixer() {
    if (hasChangedTraits() || _performInitialSend) {
        // we have at least one changed trait to send

        auto nodeList = DependencyManager::get<NodeList>();
        auto avatarMixer = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        if (!avatarMixer || !avatarMixer->getActiveSocket()) {
            // we don't have an avatar mixer with an active socket, we can't send changed traits at this time
            return;
        }

        // we have a mixer to send to, setup our set traits packet

        // bump and write the current trait version to an extended header
        // the trait version is the same for all traits in this packet list
        ++_currentTraitVersion;
        QByteArray extendedHeader(reinterpret_cast<char*>(&_currentTraitVersion), sizeof(_currentTraitVersion));

        auto traitsPacketList = NLPacketList::create(PacketType::SetAvatarTraits, extendedHeader, true);

        // take a copy of the set of changed traits and clear the stored set
        auto changedTraitsCopy { _changedTraits };
        _changedTraits.clear();

        if (_performInitialSend || changedTraitsCopy.contains(AvatarTraits::SkeletonModelURL)) {
            traitsPacketList->startSegment();
            _owningAvatar->packTrait(AvatarTraits::SkeletonModelURL, *traitsPacketList);
            traitsPacketList->endSegment();

            // keep track of our skeleton version in case we get an override back
            _currentSkeletonVersion = _currentTraitVersion;
        }

        nodeList->sendPacketList(std::move(traitsPacketList), *avatarMixer);

        // if this was an initial send of all traits, consider it completed
        _performInitialSend = false;
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
                && !hasTraitChanged(AvatarTraits::SkeletonModelURL)) {
                // override the skeleton URL but do not mark the trait as having changed
                // so that we don't unecessarily sent a new trait packet to the mixer with the overriden URL
                auto encodedSkeletonURL = QUrl::fromEncoded(message->readWithoutCopy(traitBinarySize));
                _owningAvatar->setSkeletonModelURL(encodedSkeletonURL);

                _changedTraits.erase(AvatarTraits::SkeletonModelURL);
            } else {
                message->seek(message->getPosition() + traitBinarySize);
            }
        }
    }
}

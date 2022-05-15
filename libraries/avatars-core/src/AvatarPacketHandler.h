//
//  AvatarPacketHandler.h
//  libraries/avatars-core/src
//
//  Created by Nshan G. Meadows on 11 May 2022.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_AVATARS_CORE_SRC_AVATARPACKETHANDLER_H
#define LIBRARIES_AVATARS_CORE_SRC_AVATARPACKETHANDLER_H

#include <QtCore/QHash>
#include <QtCore/QSharedPointer>
#include <QtCore/QUuid>

#include <functional>
#include <memory>
#include <chrono>

#include <glm/glm.hpp>

#include <DependencyManager.h>
#include <NLPacket.h>
#include <Node.h>

#include "AssociatedTraitValues.h"
#include "AvatarDataStream.h"

const int CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 50;
const quint64 MIN_TIME_BETWEEN_MY_AVATAR_DATA_SENDS = USECS_PER_SECOND / CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND;

template <typename Derived, typename AvatarPtr>
class AvatarPacketHandler {

    void sessionUUIDChanged(const QUuid& sessionUUID, const QUuid& oldUUID);

    void processAvatarDataPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

    void processAvatarIdentityPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

    void processBulkAvatarTraits(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

    void processKillAvatar(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

protected:
    AvatarPacketHandler();

    void parseAvatarData(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    void clearOtherAvatars();


private:
    Derived& derived();
    const Derived& derived() const;

    AvatarPtr getAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixer, bool& isNew);

    QUuid _lastOwnerSessionUUID;
    std::unordered_map<QUuid, AvatarTraits::TraitVersions> _processedTraitVersions;
};

#endif /* end of include guard */

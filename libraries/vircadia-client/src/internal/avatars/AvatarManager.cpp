//
//  AvatarManager.cpp
//  libraries/vircadia-client/src/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarManager.h"

#include <cassert>

#include <AvatarPacketHandler.hpp>

namespace vircadia::client
{

    AvatarPtr::AvatarPtr() = default;

    AvatarPtr::AvatarPtr(std::shared_ptr<Avatar> ptr, std::unique_lock<std::mutex> lock) :
        std::shared_ptr<Avatar>(ptr), lock(std::move(lock)) {}

    void AvatarManager::update() {
        std::scoped_lock lock(myAvatarMutex);
        AvatarDataPacket::SendStatus status;
        myAvatar.sendAllPackets(Avatar::SendAllData, status);
    }

    void AvatarManager::updateData() {

        {
            std::scoped_lock lock(avatarsMutex);
            // write avatar data and epitaphs out
        }

        {
            std::scoped_lock lock(myAvatarMutex);
            myAvatar.data = myAvatarDataIn;
            // TODO: clear changes
        }

    }

    const std::vector<AvatarData>& AvatarManager::getAvatarDataOut() const {
        return avatarDataOut;
    }

    template <typename T>
    auto firstEquals(T value) {
        return [value] (const auto& pair) { return pair.first == value; };
    };

    AvatarPtr AvatarManager::newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer, bool& isNew) {
        std::unique_lock lock(avatarsMutex);
        auto avatar = std::find_if(avatars.begin(), avatars.end(), firstEquals(sessionUUID));
        isNew = avatar == avatars.end();
        if (isNew) {
            avatars.emplace_back(sessionUUID, AvatarPtr(std::make_shared<Avatar>()));
            avatar = std::prev(avatars.end());
            auto epitaph = std::find_if(epitaphs.begin(), epitaphs.end(), firstEquals(sessionUUID));
            if (epitaph != epitaphs.end()) {
                epitaphs.erase(epitaph);
            }
        }

        return AvatarPtr(avatar->second, std::move(lock));
    }

    AvatarPtr AvatarManager::removeAvatar(const QUuid& sessionUUID, KillAvatarReason reason) {
        std::scoped_lock lock(avatarsMutex);
        auto avatar = std::find_if(avatars.begin(), avatars.end(), firstEquals(sessionUUID));

        assert(avatar != avatars.end());

        if (avatar != avatars.end()) {
            return AvatarPtr{};
        }

        AvatarPtr removed = std::move(avatar->second);
        avatars.erase(avatar);

        constexpr size_t EPITAPHS_MAX = 100;
        if (epitaphs.size() == EPITAPHS_MAX) {
            epitaphs.erase(epitaphs.begin());
        }
        epitaphs.emplace_back(sessionUUID, reason);

        return removed;
    }

    void AvatarManager::clearOtherAvatars() {
        std::scoped_lock lock(avatarsMutex);
        AvatarPacketHandler<AvatarManager, AvatarPtr>::clearOtherAvatars();
        avatars.clear();
        epitaphs.clear();
    }

    QUuid AvatarManager::mapIdentityUUID(const QUuid& uuid) { return uuid; }
    void AvatarManager::onAvatarIdentityReceived(const QUuid& identityUUID, const QByteArray& data) {}
    void AvatarManager::onAvatarDataReceived(const QUuid& sessionUUID, const QByteArray& data) {}

} // namespace vircadia::client

using namespace vircadia::client;

template class AvatarPacketHandler<AvatarManager, AvatarPtr>;

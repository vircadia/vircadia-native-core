//
//  AvatarManager.cpp
//  libraries/vircadia-client/src/internal/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarManager.h"

#include <cassert>

#include <AvatarPacketHandler.hpp>
#include <ThreadHelpers.h>

#include "GLMHelpers.h"

namespace vircadia::client
{

    AvatarPtr::AvatarPtr() = default;

    AvatarPtr::AvatarPtr(std::shared_ptr<Avatar> ptr, std::unique_lock<std::mutex> lock) :
        std::shared_ptr<Avatar>(ptr), lock(std::move(lock)) {}

    AvatarManager::AvatarManager() :
        AvatarPacketHandler<AvatarManager, AvatarPtr>(),
        myAvatar()
    {
        setCustomDeleter([](Dependency* dependency){
            static_cast<AvatarManager*>(dependency)->deleteLater();
        });
        myAvatar.clientTraitsHandler.reset(new ClientTraitsHandler(&myAvatar));

        moveToNewNamedThread(this, "Avatar Manager Thread");

        // TODO: figure out if this is necessary for trait override packets
        // moveToNewNamedThread(myAvatar.clientTraitsHandler.get(), "Avatar Client Traits Handler Thread");
    }

    template <typename T>
    auto firstEquals(T value) {
        return [value] (const auto& pair) { return pair.first == value; };
    };

    void AvatarManager::update() {

        udt::SequenceNumber identitySequenceNumber {};
        QUuid myUUID {};
        {
            std::scoped_lock lock(myAvatarMutex);
            identitySequenceNumber = myAvatar.data.identitySequenceNumber;
            myUUID = myAvatar.nodeUUID;
        }

        {
            std::scoped_lock lock(avatarsMutex);
            auto myAvatarReceived = std::find_if(avatars.begin(), avatars.end(), firstEquals(myUUID));

            if (myAvatarReceived != avatars.end()) {
                identitySequenceNumber = std::max(identitySequenceNumber, myAvatarReceived->second->data.identitySequenceNumber);
            }
        }

        {
            std::scoped_lock lock(myAvatarMutex);

            myAvatar.data.identitySequenceNumber = identitySequenceNumber;

            for(auto&& grabData : myAvatar.data.grabs.added) {
                myAvatar.grab(
                    qUuidfromBytes(grabData.target_id),
                    grabData.joint_index,
                    glmVec3From(grabData.offset.position),
                    glmQuatFrom(grabData.offset.rotation)
                );
            }

            for(auto&& uuid : myAvatar.data.grabs.removed) {
                myAvatar.releaseGrab(qUuidfromBytes(uuid.data()));
            }

            myAvatar.data.grabs.added.clear();
            myAvatar.data.grabs.removed.clear();

            AvatarDataPacket::SendStatus status;
            status.itemFlags = myAvatar.data.changes.to_ullong();
            status.itemFlags &= AvatarDataPacket::ALL_HAS_FLAGS;
            do {
                myAvatar.sendAllPackets(Avatar::CullSmallData, status);
            } while (!status);
            myAvatar.data.changes &= ~AvatarDataPacket::ALL_HAS_FLAGS;

            identitySequenceNumber = myAvatar.data.identitySequenceNumber;
        }

        {
            std::scoped_lock lock(viewsMutex);
            query();
        }
    }

    void AvatarManager::updateData() {

        {
            std::scoped_lock lock(avatarsMutex);

            avatarDataOut.resize(avatars.size());
            std::transform(avatars.begin(), avatars.end(), avatarDataOut.begin(), [](auto& avatar){
                avatar.second->setGrabDataIn();
                auto data = avatar.second->data;
                avatar.second->data.changes.reset();
                return data;
            });

            epitaphsOut.resize(epitaphs.size());
            std::transform(epitaphs.begin(), epitaphs.end(), epitaphsOut.begin(), [](auto& epitaph) {
                return std::pair{toUUIDArray(epitaph.first), epitaph.second};
            });
            epitaphs.clear();
        }

        {
            std::scoped_lock lock(myAvatarMutex);
            auto previousChanges = myAvatar.data.changes;
            auto identitySequenceNumber = myAvatar.data.identitySequenceNumber;

            myAvatar.data = myAvatarDataIn;

            myAvatar.data.changes |= previousChanges;
            myAvatar.data.identitySequenceNumber = identitySequenceNumber;

            myAvatarDataIn.changes.reset();
            myAvatarDataIn.grabs.added.clear();
            myAvatarDataIn.grabs.removed.clear();
        }

        {
            std::scoped_lock lock(viewsMutex);
            views.resize(viewsIn.size());
            std::transform(viewsIn.begin(), viewsIn.end(), views.begin(), [](const auto& view) {
                return ConicalViewFrustumData {
                    glmVec3From(view.position),
                    glmVec3From(view.direction),
                    view.angle,
                    view.radius,
                    view.far_clip
                };
            });
        }

    }

    const std::vector<AvatarData>& AvatarManager::getAvatarDataOut() const {
        return avatarDataOut;
    }

    AvatarPtr AvatarManager::newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer, bool& isNew) {
        std::unique_lock lock(avatarsMutex);
        auto avatar = std::find_if(avatars.begin(), avatars.end(), firstEquals(sessionUUID));
        isNew = avatar == avatars.end();
        if (isNew) {
            avatars.emplace_back(sessionUUID, AvatarPtr(std::make_shared<Avatar>()));
            avatar = std::prev(avatars.end());
        }

        return AvatarPtr(avatar->second, std::move(lock));
    }

    AvatarPtr AvatarManager::removeAvatar(const QUuid& sessionUUID, KillAvatarReason reason) {
        std::scoped_lock lock(avatarsMutex);

        for(auto&& a : avatars) {
            qDebug() << a.first;
        }

        auto avatar = std::find_if(avatars.begin(), avatars.end(), firstEquals(sessionUUID));

        if (avatar == avatars.end()) {
            return AvatarPtr{};
        }

        AvatarPtr removed = std::move(avatar->second);
        avatars.erase(avatar);

        epitaphs.emplace_back(sessionUUID, reason);

        return removed;
    }

    void AvatarManager::clearOtherAvatars() {
        std::scoped_lock lock(avatarsMutex);
        AvatarPacketHandler<AvatarManager, AvatarPtr>::clearOtherAvatars();
        avatars.clear();
        epitaphs.clear();
    }

    const std::vector<ConicalViewFrustumData>& AvatarManager::getViews() const {
        return views;
    }

    std::vector<ConicalViewFrustumData>& AvatarManager::getLastQueriedViews() {
        return lastQueriedViews;
    }

    QUuid AvatarManager::mapIdentityUUID(const QUuid& uuid) { return uuid; }
    void AvatarManager::onAvatarIdentityReceived(const QUuid& identityUUID, const QByteArray& data) {}
    void AvatarManager::onAvatarDataReceived(const QUuid& sessionUUID, const QByteArray& data) {}

    void AvatarManager::onAvatarMixerActivated() {
        std::scoped_lock lock(myAvatarMutex);
        myAvatar.data.changes.set(AvatarData::IdentityIndex);
    }

    void AvatarManager::onSessionUUIDChanged(const QUuid& uuid, const QUuid& old) {
        std::scoped_lock lock(myAvatarMutex);
        myAvatar.nodeUUID = uuid;

        // NOTE: UUID is sent with identity packet but not entirely
        // sure if this is proper. There is also a sendUUID option
        // in AvatarDataPacket::SendStatus, to send it with a data
        // packet.
        myAvatar.data.changes.set(AvatarData::IdentityIndex);
    }

    void AvatarManager::onNodeIngored(const QUuid&, bool) {}

} // namespace vircadia::client

using namespace vircadia::client;

template class AvatarPacketHandler<AvatarManager, AvatarPtr>;

//
//  AvatarManager.h
//  libraries/vircadia-client/src/internal/avatars
//
//  Created by Nshan G. on 11 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARMANAGER_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARMANAGER_H

#include <utility>
#include <vector>
#include <mutex>

#include <QObject>

#include <AvatarPacketHandler.h>

#include "AvatarData.h"

namespace vircadia::client
{

    /// @private
    class AvatarPtr : public std::shared_ptr<Avatar> {
    public:
        AvatarPtr();
        explicit AvatarPtr(std::shared_ptr<Avatar>, std::unique_lock<std::mutex> = {});
    private:
        std::unique_lock<std::mutex> lock;
    };

    /// @private
    class AvatarManager : public QObject, public AvatarPacketHandler<AvatarManager, AvatarPtr> {
        Q_OBJECT
    public:

        void update();
        void updateData();
        const std::vector<AvatarData>& getAvatarDataOut() const;

        AvatarData myAvatarDataIn;
        std::vector<AvatarData> avatarDataOut;
        std::vector<std::pair<QUuid, KillAvatarReason>> epitaphsOut;

    private:
        friend class AvatarPacketHandler<AvatarManager, AvatarPtr>;
        QUuid mapIdentityUUID(const QUuid&);
        AvatarPtr newOrExistingAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer, bool& isNew);
        AvatarPtr removeAvatar(const QUuid& sessionUUID, KillAvatarReason);
        void clearOtherAvatars();

        void onAvatarIdentityReceived(const QUuid& identityUUID, const QByteArray& data);
        void onAvatarDataReceived(const QUuid& sessionUUID, const QByteArray& data);

        Avatar myAvatar;
        std::vector<std::pair<QUuid, AvatarPtr>> avatars;
        std::vector<std::pair<QUuid, KillAvatarReason>> epitaphs;


        mutable std::mutex myAvatarMutex;
        mutable std::mutex avatarsMutex;
    };

} // namespace vircadia::client

#endif /* end of include guard */

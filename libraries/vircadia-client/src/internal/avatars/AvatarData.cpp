//
//  AvatarData.cpp
//  libraries/vircadia-client/src/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarData.h"

#include <AvatarDataStream.hpp>

namespace vircadia::client
{

    Avatar::Avatar() {
        AvatarDataStream<Avatar>::initClientTraitsHandler();
    }

    const QUuid& Avatar::getSessionUUID() const {
        return data.sessionUUID;
    }

    void Avatar::setSessionUUID(const QUuid& uuid) {
        data.sessionUUID = uuid;
    }

    AvatarDataPacket::Identity Avatar::getIdentityDataOut() const {
        return std::get<AvatarData::IdentityIndex>(data.properties);
    }

    bool Avatar::getIdentityDataChanged() {
        return data.changes.test(AvatarData::IdentityIndex);
    }

    void Avatar::onIdentityDataSent() {
        data.changes.reset(AvatarData::IdentityIndex);
    }

    AvatarDataPacket::AvatarGlobalPosition Avatar::getGlobalPositionOut() const {
        auto position = data.getProperty<AvatarData::GlobalPositionIndex>();
        return {{position.x, position.y, position.z}};
    }

    void Avatar::setGlobalPositionIn(const AvatarDataPacket::AvatarGlobalPosition& position) {
        data.setProperty<AvatarData::GlobalPositionIndex>({
            position.globalPosition[0],
            position.globalPosition[1],
            position.globalPosition[2]
        });
    }

} // namespace vircadia::client

template class AvatarDataStream<vircadia::client::Avatar>;

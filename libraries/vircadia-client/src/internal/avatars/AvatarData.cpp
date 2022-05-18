//
//  AvatarData.cpp
//  libraries/vircadia-client/src/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
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

    AvatarDataPacket::AvatarBoundingBox Avatar::getBoundingBoxOut() const {
        auto bounds = data.getProperty<AvatarData::BoundingBoxIndex>();
        data.getProperty<3>();
        return {
            {
                bounds.dimensions.x,
                bounds.dimensions.y,
                bounds.dimensions.z
            },
            {
                bounds.offset.x,
                bounds.offset.y,
                bounds.offset.z
            }
        };
    }

    void Avatar::setBoundingBoxIn(const AvatarDataPacket::AvatarBoundingBox& bounds) {
        data.setProperty<AvatarData::BoundingBoxIndex>({
            {
                bounds.avatarDimensions[0],
                bounds.avatarDimensions[1],
                bounds.avatarDimensions[2]
            },
            {
                bounds.boundOriginOffset[0],
                bounds.boundOriginOffset[1],
                bounds.boundOriginOffset[2]
            }
        });
    }

    glm::quat Avatar::getOrientationOut() const {
        auto orientation = data.getProperty<AvatarData::OrientationIndex>();
        return {
            orientation.w,
            orientation.x,
            orientation.y,
            orientation.z
        };
    }

    void Avatar::setOrientationIn(glm::quat orientation) {
        data.setProperty<AvatarData::OrientationIndex>({
            orientation.x,
            orientation.y,
            orientation.z,
            orientation.w
        });
    }

    float Avatar::getScaleOut() const {
        return data.getProperty<AvatarData::ScaleIndex>();
    }

    void Avatar::setScaleIn(float scale) {
        data.setProperty<AvatarData::ScaleIndex>(scale);
    }

    void Avatar::onParseError(std::string) {}

} // namespace vircadia::client

template class AvatarDataStream<vircadia::client::Avatar>;

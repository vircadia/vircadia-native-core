//
//  AvatarData.cpp
//  libraries/vircadia-client/src/internal/avatars
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

    Avatar::Avatar() :
        clientTraitsHandler(new ClientTraitsHandler(this))
    {}

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
        data.setProperty<AvatarData::BoundingBoxIndex>({ {
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

    AvatarDataPacket::LookAtPosition Avatar::getLookAtPositionOut() const {
        auto& position = data.getProperty<AvatarData::LookAtPositionIndex>();
        return { position.x, position.y, position.z };
    }

    void Avatar::setLookAtPositionIn(const AvatarDataPacket::LookAtPosition& position) {
        data.setProperty<AvatarData::LookAtPositionIndex>({
            position.lookAtPosition[0],
            position.lookAtPosition[1],
            position.lookAtPosition[2]
        });
    }

    float Avatar::getAudioLoudnessOut() const {
        return data.getProperty<AvatarData::AudioLoudnessIndex>();
    }

    void Avatar::setAudioLoudnessIn(float loudness) {
        data.setProperty<AvatarData::AudioLoudnessIndex>(loudness);
    }

    Avatar::Transform Avatar::getSensorToWorldMatrixOut() const {
        auto transform = data.getProperty<AvatarData::SensorToWorldMatrixIndex>();
        auto position = transform.vantage.position;
        auto rotation = transform.vantage.rotation;
        auto scale = transform.scale;
        return {
            {position.x, position.y, position.z},
            {rotation.w, rotation.x, rotation.y, rotation.z},
            {scale, scale, scale},
        };
    }

    void Avatar::setSensorToWorldMatrixIn(Transform transform) {
        auto position = transform.translation;
        auto rotation = transform.rotation;
        auto scale = transform.scale;
        data.setProperty<AvatarData::SensorToWorldMatrixIndex>({
            {
                {position.x, position.y, position.z},
                {rotation.x, rotation.y, rotation.z, rotation.w}
            },
            scale.x
        });
    }

    Avatar::AdditionalFlags Avatar::getAdditionalFlagsOut() const {
        auto flags = data.getProperty<AvatarData::AdditionalFlagsIndex>();
        return {
            KeyState(flags.key_state),
            flags.hand_state,
            bool(flags.has_head_scripted_blendshapes),
            bool(flags.has_procedural_eye_movement),
            bool(flags.has_audio_face_movement),
            bool(flags.has_procedural_eye_face_movement),
            bool(flags.has_procedural_blink_face_movement),
            bool(flags.collides_with_avatars),
            bool(flags.has_priority)
        };
    };

    void Avatar::setAdditionalFlagsIn(vircadia_avatar_additional_flags flags) {
        data.setProperty<AvatarData::AdditionalFlagsIndex>(flags);
    }

    Avatar::ParentInfo Avatar::getParentInfoOut() const {
        auto parent = data.getProperty<AvatarData::ParentInfoIndex>();
        UUID id;
        std::copy_n(parent.uuid, id.size(), id.data());
        return { id, parent.joint_index};
    }

    void Avatar::setParentInfoIn(const uint8_t* uuid, uint16_t jointIndex) {
        vircadia_avatar_parent_info info{};
        std::copy_n(uuid, UUID{}.size(), info.uuid);
        info.joint_index = jointIndex;
        data.setProperty<AvatarData::ParentInfoIndex>(info);
    }

    AvatarDataPacket::AvatarLocalPosition Avatar::getLocalPositionOut() const {
        auto position = data.getProperty<AvatarData::LocalPositionIndex>();
        return {{position.x, position.y, position.z}};
    }

    void Avatar::setLocalPositionIn(const AvatarDataPacket::AvatarLocalPosition& position) {
        data.setProperty<AvatarData::LocalPositionIndex>({
            position.localPosition[0],
            position.localPosition[1],
            position.localPosition[2]
        });
    }

    Avatar::HandControllers Avatar::getHandControllersOut() const {
        auto controllers = data.getProperty<AvatarData::HandControllersIndex>();
        return {
            {
                {
                    controllers.left.position.x,
                    controllers.left.position.y,
                    controllers.left.position.z
                },
                {
                    controllers.left.rotation.w,
                    controllers.left.rotation.x,
                    controllers.left.rotation.y,
                    controllers.left.rotation.z
                }
            },
            {
                {
                    controllers.right.position.x,
                    controllers.right.position.y,
                    controllers.right.position.z
                },
                {
                    controllers.right.rotation.w,
                    controllers.right.rotation.x,
                    controllers.right.rotation.y,
                    controllers.right.rotation.z
                }
            },
        };
    }

    void Avatar::setHandControllersIn(const HandControllers& controllers) {
        data.setProperty<AvatarData::HandControllersIndex>({
            {
                {
                    controllers.left.position.x,
                    controllers.left.position.y,
                    controllers.left.position.z
                },
                {
                    controllers.left.orientation.x,
                    controllers.left.orientation.y,
                    controllers.left.orientation.z,
                    controllers.left.orientation.w
                },
            },
            {
                {
                    controllers.left.position.x,
                    controllers.left.position.y,
                    controllers.left.position.z
                },
                {
                    controllers.left.orientation.x,
                    controllers.left.orientation.y,
                    controllers.left.orientation.z,
                    controllers.left.orientation.w
                },
            }
        });
    }

    Avatar::FaceTrackerInfo Avatar::getFaceTrackerInfoOut() const {
        auto& info = data.getProperty<AvatarData::FaceTrackerInfoIndex>();
        return {
            info.left_eye_blink,
            info.right_eye_blink,
            info.average_loundness,
            info.brow_audio_lift,
            info.blendshape_coefficients_size,
            reinterpret_cast<const uint8_t*>(info.blendshape_coefficients)
        };
    }

    void Avatar::setFaceTrackerInfoIn(const FaceTrackerInfo& info)
    {
        auto [
            left_eye_blink,
            right_eye_blink,
            average_loundness,
            brow_audio_lift,
            blendshape_coefficients_size,
            blendshape_coefficients
        ] = info;
        vircadia_avatar_face_tracker_info setInfo{};
        setInfo.left_eye_blink = left_eye_blink;
        setInfo.right_eye_blink = right_eye_blink;
        setInfo.average_loundness = average_loundness;
        setInfo.brow_audio_lift = brow_audio_lift;
        setInfo.blendshape_coefficients_size = blendshape_coefficients_size;
        std::memcpy(setInfo.blendshape_coefficients, blendshape_coefficients, blendshape_coefficients_size * sizeof(float));
        data.setProperty<AvatarData::FaceTrackerInfoIndex>(setInfo);
    }

    int Avatar::getJointDataSizeOut() const {
        return data.getProperty<AvatarData::JointDataIndex>().size();
    }

    JointData Avatar::getJointDataOut(int index) const {
        const auto& joint = data.getProperty<AvatarData::JointDataIndex>()[index];
        const auto& flags = data.getProperty<AvatarData::JointDefaultPoseFlagsIndex>()[index];
        return {
            {
                joint.rotation.w,
                joint.rotation.x,
                joint.rotation.y,
                joint.rotation.z
            },
            {
                joint.position.x,
                joint.position.y,
                joint.position.z
            },
            bool(flags.rotation_is_default),
            bool(flags.translation_is_default)
        };
    }

    void Avatar::setJointDataSizeIn(int size) {
        data.resizeProperty<AvatarData::JointDataIndex>(size);
        data.resizeProperty<AvatarData::JointDefaultPoseFlagsIndex>(size);
    }

    void Avatar::setJointDataPositionIn(int index, glm::vec3 position) {
        auto joint = data.getProperty<AvatarData::JointDataIndex>()[index];
        joint.position = vircadia_vector {
            position.x, position.y, position.z
        };
        data.setProperty<AvatarData::JointDataIndex>(joint, index);
    }

    void Avatar::setJointDataRotationIn(int index, glm::quat rotation) {
        auto joint = data.getProperty<AvatarData::JointDataIndex>()[index];
        joint.rotation = vircadia_quaternion {
            rotation.x, rotation.y, rotation.z, rotation.w
        };
        data.setProperty<AvatarData::JointDataIndex>(joint, index);
    }

    void Avatar::setJointDataPositionDefaultIn(int index, bool isDefault) {
        auto flags = data.getProperty<AvatarData::JointDefaultPoseFlagsIndex>()[index];
        flags.translation_is_default = isDefault;
        data.setProperty<AvatarData::JointDefaultPoseFlagsIndex>(flags, index);
    }

    void Avatar::setJointDataRotationDefaultIn(int index, bool isDefault) {
        auto flags = data.getProperty<AvatarData::JointDefaultPoseFlagsIndex>()[index];
        flags.rotation_is_default = isDefault;
        data.setProperty<AvatarData::JointDefaultPoseFlagsIndex>(flags, index);
    }

    void Avatar::setSkeletonModelURLIn(const QByteArray& encodedURL) {
        data.setProperty<AvatarData::SkeletonModelURLIndex>(encodedURL.toStdString());
    }

    void Avatar::onParseError(std::string) {
        //TODO keep a log of last N errors
    }

    void Avatar::onPacketTooSmallError(std::string, int, int) {
        //TODO keep a log of last N errors
    }

    ClientTraitsHandler* Avatar::getClientTraitsHandler() {
        return clientTraitsHandler.get();
    }

    const ClientTraitsHandler* Avatar::getClientTraitsHandler() const {
        return clientTraitsHandler.get();
    }

} // namespace vircadia::client

template class AvatarDataStream<vircadia::client::Avatar>;

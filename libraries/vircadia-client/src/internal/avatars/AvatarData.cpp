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

#include "GLMHelpers.h"

using vircadia::client::operator==;
using vircadia::client::operator!=;

namespace vircadia::client
{

    QUuid Avatar::getSessionUUID() const {
        return qUuidfromBytes(data.sessionUUID.data());
    }

    void Avatar::setSessionUUID(const QUuid& uuid) {
        data.sessionUUID = toUUIDArray(uuid);
    }

    QUuid Avatar::getSessionUUIDOut() const {
        return nodeUUID;
    }

    AvatarDataPacket::Identity Avatar::getIdentityDataOut() const {
        auto identity = std::get<AvatarData::IdentityIndex>(data.properties);
        QVector<AttachmentData> attachments;
        std::transform(identity.attachments.begin(), identity.attachments.end(),
            std::back_inserter(attachments), [](const auto& x){
                return AttachmentData{
                    QString(x.modelURL.c_str()),
                    QString(x.jointName.c_str()),
                    glmVec3From(x.transform.vantage.position),
                    glmQuatFrom(x.transform.vantage.rotation),
                    x.transform.scale,
                    x.isSoft
                };
        });
        return {
            std::move(attachments),
            QString(identity.displayName.c_str()),
            QString(identity.sessionDisplayName.c_str()),
            identity.identityFlags
        };
    }

    void Avatar::setIdentityDataIn(AvatarDataPacket::Identity identity) {
        std::vector<AvatarData::Attachment> attachments;
        std::transform(identity.attachmentData.begin(), identity.attachmentData.end(),
            std::back_inserter(attachments), [](const auto& x) {
                return AvatarData::Attachment{
                    x.modelURL.toString().toStdString(),
                    x.jointName.toStdString(),
                    vectorFrom(x.translation),
                    quaternionFrom(x.rotation),
                    x.scale,
                    x.isSoft
                };
            }
        );
        data.setProperty<AvatarData::IdentityIndex>({
            std::move(attachments),
            identity.displayName.toStdString(),
            identity.sessionDisplayName.toStdString(),
            identity.identityFlags
        });
    }

    bool Avatar::getIdentityDataChanged() const {
        return data.changes.test(AvatarData::IdentityIndex);
    }

    udt::SequenceNumber Avatar::getIdentitySequenceNumberOut() const {
        return data.identitySequenceNumber;
    }

    void Avatar::setIdentitySequenceNumberIn(udt::SequenceNumber number) {
        data.identitySequenceNumber = number;
    }

    void Avatar::pushIdentitySequenceNumber() {
        ++data.identitySequenceNumber;
    }

    void Avatar::onIdentityDataSent() {
        data.changes.reset(AvatarData::IdentityIndex);
    }

    bool Avatar::getSkeletonModelURLChanged() const {
        return data.changes.test(AvatarData::SkeletonModelURLIndex);
    }

    bool Avatar::getSkeletonDataChanged() const {
        return data.changes.test(AvatarData::SkeletonDataIndex);
    }

    bool Avatar::getGrabDataChanged() const {
        return data.changes.test(AvatarData::GrabDataIndex);
    }

    bool Avatar::getEntityDataChanged() const {
        return data.changes.test(AvatarData::EntityDataIndex);
    }

    void Avatar::onClientTraitsSent() {
        data.changes.reset(AvatarData::SkeletonModelURLIndex);
        data.changes.reset(AvatarData::SkeletonDataIndex);
        data.changes.reset(AvatarData::GrabDataIndex);
        data.changes.reset(AvatarData::EntityDataIndex);
    }

    AvatarDataPacket::AvatarGlobalPosition Avatar::getGlobalPositionOut() const {
        auto position = data.getProperty<AvatarData::GlobalPositionIndex>();
        AvatarDataPacket::AvatarGlobalPosition result;
        serialize(position, result.globalPosition);
        return result;
    }

    void Avatar::setGlobalPositionIn(const AvatarDataPacket::AvatarGlobalPosition& position) {
        data.setProperty<AvatarData::GlobalPositionIndex>(
            vectorFrom(position.globalPosition));
    }

    AvatarDataPacket::AvatarBoundingBox Avatar::getBoundingBoxOut() const {
        auto bounds = data.getProperty<AvatarData::BoundingBoxIndex>();
        AvatarDataPacket::AvatarBoundingBox result;
        serialize(bounds.dimensions, result.avatarDimensions);
        serialize(bounds.offset, result.boundOriginOffset);
        return result;
    }

    void Avatar::setBoundingBoxIn(const AvatarDataPacket::AvatarBoundingBox& bounds) {
        data.setProperty<AvatarData::BoundingBoxIndex>({
            vectorFrom(bounds.avatarDimensions),
            vectorFrom(bounds.boundOriginOffset),
        });
    }

    glm::quat Avatar::getOrientationOut() const {
        return glmQuatFrom(data.getProperty<AvatarData::OrientationIndex>());
    }

    void Avatar::setOrientationIn(glm::quat orientation) {
        data.setProperty<AvatarData::OrientationIndex>(
            quaternionFrom(orientation));
    }

    float Avatar::getScaleOut() const {
        return data.getProperty<AvatarData::ScaleIndex>();
    }

    void Avatar::setScaleIn(float scale) {
        data.setProperty<AvatarData::ScaleIndex>(scale);
    }

    AvatarDataPacket::LookAtPosition Avatar::getLookAtPositionOut() const {
        auto& position = data.getProperty<AvatarData::LookAtPositionIndex>();
        AvatarDataPacket::LookAtPosition result;
        serialize(position, result.lookAtPosition);
        return result;
    }

    void Avatar::setLookAtPositionIn(const AvatarDataPacket::LookAtPosition& position) {
        data.setProperty<AvatarData::LookAtPositionIndex>({
            vectorFrom(position.lookAtPosition)
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
        return {
            glmVec3From(transform.vantage.position),
            glmQuatFrom(transform.vantage.rotation),
            glm::vec3(transform.scale)
        };
    }

    void Avatar::setSensorToWorldMatrixIn(Transform transform) {
        data.setProperty<AvatarData::SensorToWorldMatrixIndex>({
            {
                vectorFrom(transform.translation),
                quaternionFrom(transform.rotation)
            },
            transform.scale.x
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
        AvatarDataPacket::AvatarLocalPosition result;
        serialize(position, result.localPosition);
        return result;
    }

    void Avatar::setLocalPositionIn(const AvatarDataPacket::AvatarLocalPosition& position) {
        data.setProperty<AvatarData::LocalPositionIndex>(
            vectorFrom(position.localPosition));
    }

    Avatar::HandControllers Avatar::getHandControllersOut() const {
        auto controllers = data.getProperty<AvatarData::HandControllersIndex>();
        return {
            {
                glmVec3From(controllers.left.position),
                glmQuatFrom(controllers.left.rotation),
            },
            {
                glmVec3From(controllers.right.position),
                glmQuatFrom(controllers.right.rotation),
            }
        };
    }

    void Avatar::setHandControllersIn(const HandControllers& controllers) {
        data.setProperty<AvatarData::HandControllersIndex>({
            {
                vectorFrom(controllers.left.position),
                quaternionFrom(controllers.left.orientation)
            },
            {
                vectorFrom(controllers.right.position),
                quaternionFrom(controllers.right.orientation)
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
        if (index < 0) {
            return JointData{};
        }

        const auto& joints = data.getProperty<AvatarData::JointDataIndex>();
        const auto& allFlags = data.getProperty<AvatarData::JointDefaultPoseFlagsIndex>();
        const auto& joint = size_t(index) < joints.size() ? joints[index] : vircadia_vantage{};
        const auto& flags = size_t(index) < allFlags.size() ? allFlags[index] : vircadia_joint_flags{};
        return {
            glmQuatFrom(joint.rotation),
            glmVec3From(joint.position),
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
        joint.position = vectorFrom(position);
        data.setProperty<AvatarData::JointDataIndex>(index, joint);
    }

    void Avatar::setJointDataRotationIn(int index, glm::quat rotation) {
        auto joint = data.getProperty<AvatarData::JointDataIndex>()[index];
        joint.rotation = quaternionFrom(rotation);
        data.setProperty<AvatarData::JointDataIndex>(index, joint);
    }

    void Avatar::setJointDataPositionDefaultIn(int index, bool isDefault) {
        auto flags = data.getProperty<AvatarData::JointDefaultPoseFlagsIndex>()[index];
        flags.translation_is_default = isDefault;
        data.setProperty<AvatarData::JointDefaultPoseFlagsIndex>(index, flags);
    }

    void Avatar::setJointDataRotationDefaultIn(int index, bool isDefault) {
        auto flags = data.getProperty<AvatarData::JointDefaultPoseFlagsIndex>()[index];
        flags.rotation_is_default = isDefault;
        data.setProperty<AvatarData::JointDefaultPoseFlagsIndex>(index, flags);
    }

    AvatarDataPacket::FarGrabJoints Avatar::getFarGrabJointsOut() const {
        auto joints = data.getProperty<AvatarData::GrabJointsIndex>();
        AvatarDataPacket::FarGrabJoints result;
        serialize(joints.left.position, result.leftFarGrabPosition);
        serialize(joints.left.rotation, result.leftFarGrabRotation);
        serialize(joints.right.position, result.rightFarGrabPosition);
        serialize(joints.right.rotation, result.rightFarGrabRotation);
        serialize(joints.mouse.position, result.mouseFarGrabPosition);
        serialize(joints.mouse.rotation, result.mouseFarGrabRotation);
        return result;
    }

    void Avatar::setFarGrabJointsIn(const AvatarDataPacket::FarGrabJoints& joints)  {
        data.setProperty<AvatarData::GrabJointsIndex>({
            {
                vectorFrom(joints.leftFarGrabPosition),
                quaternionFrom(joints.leftFarGrabRotation)
            },
            {
                vectorFrom(joints.rightFarGrabPosition),
                quaternionFrom(joints.rightFarGrabRotation)
            },
            {
                vectorFrom(joints.mouseFarGrabPosition),
                quaternionFrom(joints.mouseFarGrabRotation)
            }
        });
    }

    const char* Avatar::getSkeletonModelURLOut() const {
        return data.getProperty<AvatarData::SkeletonModelURLIndex>().c_str();
    }

    void Avatar::setSkeletonModelURLIn(const QByteArray& encodedURL) {
        data.setProperty<AvatarData::SkeletonModelURLIndex>(encodedURL.toStdString());
    }

    std::vector<AvatarSkeletonTrait::UnpackedJointData> Avatar::getSkeletonDataOut() const {
        auto bones = data.getProperty<AvatarData::SkeletonDataIndex>();
        std::vector<AvatarSkeletonTrait::UnpackedJointData> result;
        int stringStart = 0;
        std::transform(bones.begin(), bones.end(), std::back_inserter(result), [&stringStart](auto& bone) {
            AvatarSkeletonTrait::UnpackedJointData result{};
            result.boneType = bone.type;
            result.defaultTranslation = glmVec3From(bone.defaultTransform.vantage.position);
            result.defaultRotation = glmQuatFrom(bone.defaultTransform.vantage.rotation);
            result.defaultScale = bone.defaultTransform.scale;
            result.jointIndex = bone.index;
            result.parentIndex = bone.parentIndex;
            result.jointName = QString(bone.name.c_str());

            // TODO: this logic belongs in AvatarDataStream
            result.stringStart = stringStart;
            result.stringLength = result.jointName.size();
            stringStart += result.stringLength;

            return result;

        });
        return result;
    }

    void Avatar::setSkeletonDataIn(std::vector<AvatarSkeletonTrait::UnpackedJointData> skeletonData) {
        constexpr auto index = AvatarData::SkeletonDataIndex;
        data.resizeProperty<index>(skeletonData.size());
        for (std::size_t i = 0; i != skeletonData.size(); ++i) {
            data.setProperty<index>(i, AvatarData::Bone{
                AvatarSkeletonTrait::BoneType(skeletonData[i].boneType),
                {
                    {
                        vectorFrom(skeletonData[i].defaultTranslation),
                        quaternionFrom(skeletonData[i].defaultRotation)
                    },
                    skeletonData[i].defaultScale
                },
                skeletonData[i].jointIndex,
                skeletonData[i].parentIndex,
                skeletonData[i].jointName.toStdString()
            });
        }
    }

    void Avatar::setGrabDataIn() {
        data.resizeProperty<AvatarData::GrabDataIndex>(_avatarGrabData.size());
        int i = 0;
        for (auto begin = _avatarGrabData.begin(); begin != _avatarGrabData.end(); ++begin) {

            Grab grab;
            grab.fromByteArray(*begin);

            vircadia_avatar_grab converted{};
            auto targetID = toUUIDArray(grab.getTargetID());
            std::copy_n(targetID.begin(), targetID.size(), converted.target_id);
            converted.joint_index = grab.getParentJointIndex();
            converted.offset.position = vectorFrom(grab.getPositionalOffset());
            converted.offset.rotation = quaternionFrom(grab.getRotationalOffset());

            data.setProperty<AvatarData::GrabDataIndex>(i++,
                std::pair{ toUUIDArray(begin.key()), converted });
        }
    }

    void Avatar::onParseError(std::string) {
        //TODO keep a log of last N errors
    }

    void Avatar::onPacketTooSmallError(std::string, int, int) {
        //TODO keep a log of last N errors
    }

    void Avatar::onGrabRemoved(QUuid) {};

    ClientTraitsHandler* Avatar::getClientTraitsHandler() {
        return clientTraitsHandler.get();
    }

    const ClientTraitsHandler* Avatar::getClientTraitsHandler() const {
        return clientTraitsHandler.get();
    }

} // namespace vircadia::client

template class AvatarDataStream<vircadia::client::Avatar>;

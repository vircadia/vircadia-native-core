//
//  avatars.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 16 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "avatars.h"


#include "internal/Error.h"
#include "internal/Context.h"
#include "internal/Avatars.h"
#include "internal/avatars/AvatarData.h"
#include "internal/avatars/GLMHelpers.h"

using namespace vircadia::client;

using vircadia::client::operator==;

VIRCADIA_CLIENT_DYN_API
int vircadia_enable_avatars(int context_id) {
    return chain(checkContextReady(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->avatars().enable();
        return 0;
    });
}

int checkAvatarsEnabled(int id) {
    return chain(checkContextReady(id), [&](auto) {
        return std::next(std::begin(contexts), id)->avatars().isEnabled()
            ? 0
            : toInt(ErrorCode::AVATARS_DISABLED);
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_update_avatars(int context_id) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->avatars().update();
        return 0;
    });
}

template <AvatarData::PropertyIndex Index, typename T>
int setProperty(int context_id, T value) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->avatars().myAvatar()
            .setProperty<Index>(value);
        return 0;
    });
}

template <AvatarData::PropertyIndex Index>
int resizeProperty(int context_id, int size) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->avatars().myAvatar()
            .resizeProperty<Index>(size);
        return 0;
    });
}

template <typename F>
auto validateAvatarIndex(int context_id, int index, F&& f) {
    return chain(checkContextReady(context_id), [&](auto) {
        const auto& avatars = std::next(std::begin(contexts), context_id)->avatars().all();
        return chain(checkIndexValid(avatars, index, ErrorCode::AVATAR_INVALID), [&](auto) {
            return std::invoke(std::forward<F>(f), *std::next(std::begin(avatars), index));
        });
    });
}

template <AvatarData::PropertyIndex Index, ErrorCode error>
struct indexChecker {
    int operator()(const AvatarData& avatar, int index) const {
        return checkIndexValid(avatar.getProperty<Index>(), index, error);
    }
};

template <>
struct indexChecker<AvatarData::IdentityIndex, ErrorCode::AVATAR_ATTACHMENT_INVALID> {
    int operator()(const AvatarData& avatar, int index) const {
        return checkIndexValid(avatar.getProperty<AvatarData::IdentityIndex>().attachments, index, ErrorCode::AVATAR_ATTACHMENT_INVALID);
    }
};

template <AvatarData::PropertyIndex Index>
auto getIndexChecker() {
    if constexpr (Index == AvatarData::JointDataIndex ||
        Index == AvatarData::JointDefaultPoseFlagsIndex) {
        return indexChecker<Index, ErrorCode::AVATAR_JOINT_INVALID>{};
    } else if constexpr (Index == AvatarData::SkeletonDataIndex) {
        return indexChecker<Index, ErrorCode::AVATAR_BONE_INVALID>{};
    } else if constexpr (Index == AvatarData::GrabDataIndex) {
        return indexChecker<Index, ErrorCode::AVATAR_GRAB_INVALID>{};
    }
}

template <typename Check, typename F>
auto validatePropertyIndex(int context_id, int avatarIndex, int propertyIndex, Check check, F callback) {
    return validateAvatarIndex(context_id, avatarIndex, [&](auto& avatar) {
        return chain(check(avatar, propertyIndex), [&](auto) {
            return callback(avatar);
        });
    });
}

template <typename Check, typename F>
auto validatePropertyIndex(int context_id, int propertyIndex, Check check, F callback) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        auto& avatar = std::next(std::begin(contexts), context_id)->avatars().myAvatar();
        return chain(check(avatar, propertyIndex), [&](auto) {
            return callback(avatar);
        });
    });
}

template <AvatarData::PropertyIndex Index, typename Value>
int setProperty(int context_id, int index, Value value) {
    return validatePropertyIndex(context_id, index, getIndexChecker<Index>(), [&](AvatarData& avatar) {
        avatar.setProperty<Index>(index, std::move(value));
        return 0;
    });
}

template <typename F>
int setIdentity(int context_id, F&& f) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        auto& avatar = std::next(std::begin(contexts), context_id)->avatars().myAvatar();
        constexpr auto index = AvatarData::IdentityIndex;
        auto identity = avatar.getProperty<index>();
        f(identity);
        avatar.setProperty<index>(identity);
        return 0;
    });
}

int setIdentityFlag(int context_id, AvatarDataPacket::IdentityFlag flag) {
    return setIdentity(context_id, [flag](auto& identity){
        identity.identityFlags |= flag;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_display_name(int context_id, const char* display_name) {
    return setIdentity(context_id, [display_name](auto& identity){
        identity.displayName = display_name;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_is_replicated(int context_id, uint8_t is_replicated) {
    return setIdentityFlag(context_id,
        AvatarDataPacket::IdentityFlag::isReplicated);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_look_at_snapping(int context_id, uint8_t look_at_snapping_enabled) {
    return setIdentityFlag(context_id,
        AvatarDataPacket::IdentityFlag::lookAtSnapping);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_verification(int context_id, uint8_t verification_failed) {
    return setIdentityFlag(context_id,
        AvatarDataPacket::IdentityFlag::verificationFailed);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachment_count(int context_id, int attachment_count) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        auto& avatar = std::next(std::begin(contexts), context_id)->avatars().myAvatar();
        avatar.setProperty<AvatarData::IdentityIndex>( overloaded {
            [](auto& identity) -> int {
                return identity.attachments.size();
            },
            [](auto& identity, auto&& count) {
                identity.attachments.resize(count);
            }
        }, attachment_count);
        return 0;
    });
}

AvatarData::Attachment toAttachment(vircadia_avatar_attachment x) {
    return {
        x.model_url,
        x.joint_name,
        x.transform,
        bool(x.is_soft)
    };
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachment(int context_id, int attachment_index, vircadia_avatar_attachment attachment) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        auto& avatar = std::next(std::begin(contexts), context_id)->avatars().myAvatar();
        return chain(indexChecker<AvatarData::IdentityIndex, ErrorCode::AVATAR_ATTACHMENT_INVALID>{}(avatar, attachment_index), [&](auto) {
            avatar.setProperty<AvatarData::IdentityIndex>( overloaded {
                [attachment_index](auto& identity) -> decltype(auto) {
                    return (identity.attachments[attachment_index]);
                },
                [attachment_index](auto& identity, auto&& attachment) {
                    identity.attachments[attachment_index] = std::forward<decltype(attachment)>(attachment);
                }
            }, toAttachment(attachment));
            return 0;
        });
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachments(int context_id, vircadia_avatar_attachment* attachments, int size) {
    return setIdentity(context_id, [&](auto& identity) {
        identity.attachments.clear();
        for (int i = 0; i != size; ++i) {
            identity.attachments.push_back(toAttachment(attachments[i]));
        }
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_session_display_name(int context_id, const char* session_display_name) {
    return setIdentity(context_id, [session_display_name](auto& identity){
        identity.sessionDisplayName = session_display_name;
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_model_url(int context_id, const char* skeleton_model_url) {
    return setProperty<AvatarData::SkeletonModelURLIndex>(context_id, skeleton_model_url);
}

AvatarData::Bone toBone(vircadia_avatar_bone bone) {
    return {
        AvatarSkeletonTrait::BoneType(bone.type),
        bone.default_transform,
        bone.index,
        bone.parent_index,
        bone.name
    };
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bone_count(int context_id, int bone_count) {
    return resizeProperty<AvatarData::SkeletonDataIndex>(context_id, bone_count);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bone(int context_id, int bone_index, vircadia_avatar_bone bone) {
    return setProperty<AvatarData::SkeletonDataIndex>(
        context_id, bone_index, toBone(bone));
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_data(int context_id, vircadia_avatar_bone* data, int size) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        auto& avatar = std::next(std::begin(contexts), context_id)->avatars().myAvatar();
        constexpr auto index = AvatarData::SkeletonDataIndex;
        avatar.resizeProperty<index>(size);
        for (int i = 0; i != size; ++i) {
            avatar.setProperty<index>(i, toBone(data[i]));
        }
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_global_position(int context_id, vircadia_vector position) {
    return setProperty<AvatarData::GlobalPositionIndex>(context_id, position);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bounding_box(int context_id, vircadia_bounds bounding_box) {
    return setProperty<AvatarData::BoundingBoxIndex>(context_id, bounding_box);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_orientation(int context_id, vircadia_quaternion orientation) {
    return setProperty<AvatarData::OrientationIndex>(context_id, orientation);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_scale(int context_id, float scale) {
    return setProperty<AvatarData::ScaleIndex>(context_id, scale);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_look_at(int context_id, vircadia_vector position) {
    return setProperty<AvatarData::LookAtPositionIndex>(context_id, position);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_audio_loudness(int context_id, float loudness) {
    return setProperty<AvatarData::AudioLoudnessIndex>(context_id, loudness);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_sensor_to_world(int context_id, vircadia_transform transform) {
    return setProperty<AvatarData::SensorToWorldMatrixIndex>(context_id, transform);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_additional_flags(int context_id, vircadia_avatar_additional_flags flags) {
    return setProperty<AvatarData::AdditionalFlagsIndex>(context_id, flags);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_parent_info(int context_id, vircadia_avatar_parent_info info) {
    return setProperty<AvatarData::ParentInfoIndex>(context_id, info);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_local_position(int context_id, vircadia_vector position) {
    return setProperty<AvatarData::LocalPositionIndex>(context_id, position);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_hand_controllers(int context_id, vircadia_avatar_hand_controllers controllers) {
    return setProperty<AvatarData::HandControllersIndex>(context_id, controllers);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_face_tracker_info(int context_id, vircadia_avatar_face_tracker_info info) {
    return setProperty<AvatarData::FaceTrackerInfoIndex>(context_id, info);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_count(int context_id, int joint_count) {
    return resizeProperty<AvatarData::JointDataIndex>(context_id, joint_count);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint(int context_id, int joint_index, vircadia_vantage joint) {
    return setProperty<AvatarData::JointDataIndex>(
        context_id, joint_index, joint);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_data(int context_id, const vircadia_vantage* joints, int size) {
    return setProperty<AvatarData::JointDataIndex>(context_id, std::vector<vircadia_vantage>(joints, joints + size));
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags_count(int context_id, int joint_count) {
    return resizeProperty<AvatarData::JointDefaultPoseFlagsIndex>(context_id, joint_count);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags(int context_id, int joint_index, vircadia_joint_flags flags) {
    return setProperty<AvatarData::JointDefaultPoseFlagsIndex>(
        context_id, joint_index, flags);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_all_joint_flags(int context_id, const vircadia_joint_flags* joints, int size) {
    return setProperty<AvatarData::JointDefaultPoseFlagsIndex>(context_id, std::vector<vircadia_joint_flags>(joints, joints + size));
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_grab_joints(int context_id, vircadia_far_grab_joints joints) {
    return setProperty<AvatarData::GrabJointsIndex>(context_id, joints);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_my_avatar_grab(int context_id, vircadia_avatar_grab grab) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->avatars()
            .myAvatar().grabs.added.push_back(grab);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_my_avatar_release_grab(int context_id, const uint8_t* uuid) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        UUID removed;
        std::copy_n(uuid, removed.size(), removed.begin());
        std::next(std::begin(contexts), context_id)->avatars()
            .myAvatar().grabs.removed.push_back(removed);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_avatar_view_count(int context_id, int view_count) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->
            avatars().views().resize(view_count);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_avatar_view(int context_id, int view_index, vircadia_conical_view_frustum view_frustum) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        auto& views = std::next(std::begin(contexts), context_id)->avatars().views();
        return chain(checkIndexValid(views, view_index, ErrorCode::AVATAR_VIEW_INVALID), [&](auto) {
            views[view_index] = view_frustum;
            return 0;
        });
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_count(int context_id) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) -> int {
        return std::next(std::begin(contexts), context_id)
            ->avatars().all().size();
    });
}

VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_get_avatar_uuid(int context_id, int avatar_index) {
    return validateAvatarIndex(context_id, avatar_index, [](auto& avatar) {
        return avatar.sessionUUID.data();
    });
}

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_display_name(int context_id, int avatar_index) {
    return validateAvatarIndex(context_id, avatar_index, [](const AvatarData& avatar) {
        return avatar.getProperty<AvatarData::IdentityIndex>().displayName.c_str();
    });
}

int getIdentityFlag(int context_id, int avatar_index, AvatarDataPacket::IdentityFlag flag) {
    return validateAvatarIndex(context_id, avatar_index, [flag](const AvatarData& avatar) {
        return (avatar.getProperty<AvatarData::IdentityIndex>().identityFlags & flag) ? 1 : 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_is_replicated(int context_id, int avatar_index) {
    return getIdentityFlag(context_id, avatar_index, AvatarDataPacket::IdentityFlag::isReplicated);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_look_at_snapping(int context_id, int avatar_index) {
    return getIdentityFlag(context_id, avatar_index, AvatarDataPacket::IdentityFlag::lookAtSnapping);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_verification_failed(int context_id, int avatar_index) {
    return getIdentityFlag(context_id, avatar_index, AvatarDataPacket::IdentityFlag::verificationFailed);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_attachment_count(int context_id, int avatar_index) {
    return validateAvatarIndex(context_id, avatar_index, [](const AvatarData& avatar) -> int {
        return avatar.getProperty<AvatarData::IdentityIndex>().attachments.size();
    });
}

template <>
auto vircadia::client::makeError<vircadia_avatar_attachment_result>(int errorCode) {
    return vircadia_avatar_attachment_result { {}, errorCode };
}

VIRCADIA_CLIENT_DYN_API
vircadia_avatar_attachment_result vircadia_get_avatar_attachment(int context_id, int avatar_index, int attachment_index) {
    constexpr auto propertyIndex = AvatarData::IdentityIndex;
    return validatePropertyIndex(context_id, avatar_index,
        attachment_index,
        indexChecker<propertyIndex, ErrorCode::AVATAR_ATTACHMENT_INVALID>{},
        [&](const AvatarData& avatar) {
            const auto& attachment = avatar.getProperty<propertyIndex>().attachments[attachment_index];
            return vircadia_avatar_attachment_result {
                {
                    attachment.modelURL.c_str(),
                    attachment.jointName.c_str(),
                    attachment.transform,
                    uint8_t(attachment.isSoft ? 1 : 0)
                },
                0
            };
        }
    );
}

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_session_display_name(int context_id, int avatar_index) {
    return validateAvatarIndex(context_id, avatar_index, [](const AvatarData& avatar) {
        return avatar.getProperty<AvatarData::IdentityIndex>().sessionDisplayName.c_str();
    });
}

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_skeleton_model_url(int context_id, int avatar_index) {
    return validateAvatarIndex(context_id, avatar_index, [](auto& avatar) {
        return avatar.template getProperty<AvatarData::SkeletonModelURLIndex>().c_str();
    });
}

template <AvatarData::PropertyIndex propertyIndex>
int getPropertySize(int context_id, int avatar_index) {
    return validateAvatarIndex(context_id, avatar_index, [](auto& avatar) -> int {
        return avatar.template getProperty<propertyIndex>().size();
    });
}

template <AvatarData::PropertyIndex Index>
auto getPropertyPtr(int context_id, int avatar_index) {
    return validateAvatarIndex(context_id, avatar_index, [](auto& avatar) {
        return &avatar.template getProperty<Index>();
    });
}

template <AvatarData::PropertyIndex propertyIndex, typename Converter>
auto getProperty(int context_id, int avatar_index, int property_index, Converter&& convert) {
    return validatePropertyIndex(context_id, avatar_index,
        property_index,
        getIndexChecker<propertyIndex>(),
        [&](const AvatarData& avatar) {
            return convert(avatar.getProperty<propertyIndex>()[property_index]);
        }
    );
}

template <AvatarData::PropertyIndex propertyIndex>
auto getPropertyPtr(int context_id, int avatar_index, int property_index) {
    return getProperty<propertyIndex>(context_id, avatar_index,
        property_index,
        [](auto& property) {
            return &property;
        }
    );
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_bone_count(int context_id, int avatar_index) {
    return getPropertySize<AvatarData::SkeletonDataIndex>(context_id, avatar_index);
}

template <>
auto vircadia::client::makeError<vircadia_avatar_bone_result>(int errorCode) {
    return vircadia_avatar_bone_result { {}, errorCode };
}

VIRCADIA_CLIENT_DYN_API
vircadia_avatar_bone_result vircadia_get_avatar_bone(int context_id, int avatar_index, int bone_index) {
    return getProperty<AvatarData::SkeletonDataIndex>(context_id, avatar_index,
        bone_index, [](auto& bone) {
            return vircadia_avatar_bone_result {
                {
                    bone.type,
                    bone.defaultTransform,
                    bone.index,
                    bone.parentIndex,
                    bone.name.c_str()
                },
                0
            };
        }
    );
}

VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_global_position(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::GlobalPositionIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_bounds* vircadia_get_avatar_bounding_box(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::BoundingBoxIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_quaternion* vircadia_get_avatar_orientation(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::OrientationIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const float* vircadia_get_avatar_scale(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::ScaleIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_look_at_position(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::LookAtPositionIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const float* vircadia_get_avatar_audio_loudness(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::AudioLoudnessIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_transform* vircadia_get_avatar_sensor_to_world(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::SensorToWorldMatrixIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_additional_flags*  vircadia_get_avatar_additional_flags(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::AdditionalFlagsIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_parent_info* vircadia_get_avatar_parent_info(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::ParentInfoIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_local_position(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::LocalPositionIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_hand_controllers* vircadia_get_avatar_hand_controllers(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::HandControllersIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_face_tracker_info* vircadia_get_avatar_face_tracker_info(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::FaceTrackerInfoIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_joint_count(int context_id, int avatar_index) {
    return getPropertySize<AvatarData::JointDataIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_vantage* vircadia_get_avatar_joint(int context_id, int avatar_index, int joint_index) {
    return getPropertyPtr<AvatarData::JointDataIndex>(context_id, avatar_index, joint_index);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_joint_flags_count(int context_id, int avatar_index) {
    return getPropertySize<AvatarData::JointDefaultPoseFlagsIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_joint_flags* vircadia_get_avatar_joint_flags(int context_id, int avatar_index, int joint_index) {
    return getPropertyPtr<AvatarData::JointDefaultPoseFlagsIndex>(context_id, avatar_index, joint_index);
}

VIRCADIA_CLIENT_DYN_API
const vircadia_far_grab_joints* vircadia_get_avatar_grab_joints(int context_id, int avatar_index) {
    return getPropertyPtr<AvatarData::GrabJointsIndex>(context_id, avatar_index);
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_grabs_count(int context_id, int avatar_index) {
    return getPropertySize<AvatarData::GrabDataIndex>(context_id, avatar_index);
}

template <>
auto vircadia::client::makeError<vircadia_avatar_grab_result>(int errorCode) {
    return vircadia_avatar_grab_result { {}, {}, errorCode };
}

VIRCADIA_CLIENT_DYN_API
vircadia_avatar_grab_result vircadia_get_avatar_grab(int context_id, int avatar_index, int grab_index) {
    return getProperty<AvatarData::GrabDataIndex>(context_id, avatar_index,
        grab_index, [](auto& grab) {
            auto& [id, data] = grab;
            return vircadia_avatar_grab_result {
                id.data(),
                data,
                0
            };
        }
    );
}


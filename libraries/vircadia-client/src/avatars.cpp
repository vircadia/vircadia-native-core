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

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_display_name(int context_id, const char* display_name) {
    return chain(checkAvatarsEnabled(context_id), [&](auto) {
        auto& avatar = std::next(std::begin(contexts), context_id)->avatars().myAvatar();
        constexpr auto index = AvatarData::IdentityIndex;
        auto identity = avatar.getProperty<index>();
        identity.displayName = display_name;
        avatar.setProperty<index>(identity);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_model_url(int context_id, const char* skeleton_model_url) {
    return setProperty<AvatarData::SkeletonModelURLIndex>(context_id, skeleton_model_url);
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
int vircadia_set_my_avatar_joint_data(int context_id, vircadia_vantage* joints, int size) {
    return setProperty<AvatarData::JointDataIndex>(context_id, std::vector<vircadia_vantage>(joints, joints + size));
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags(int context_id, vircadia_joint_flags* joints, int size) {
    return setProperty<AvatarData::JointDefaultPoseFlagsIndex>(context_id, std::vector<vircadia_joint_flags>(joints, joints + size));
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_grab_joints(int context_id, vircadia_far_grab_joints joints) {
    return setProperty<AvatarData::GrabJointsIndex>(context_id, joints);
}


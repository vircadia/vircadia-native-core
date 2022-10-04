//
//  avatar_constants.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 9 June 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "avatar_constants.h"

#include <AvatarDataStream.h>

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_left_hand_pointing() {
    return LEFT_HAND_POINTING_FLAG;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_right_hand_pointing() {
    return RIGHT_HAND_POINTING_FLAG;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_index_finger_pointing() {
    return IS_FINGER_POINTING_FLAG;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_skeleton_root_bone() {
    return AvatarSkeletonTrait::BoneType::SkeletonRoot;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_skeleton_child_bone() {
    return AvatarSkeletonTrait::BoneType::SkeletonChild;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_root_bone() {
    return AvatarSkeletonTrait::BoneType::NonSkeletonRoot;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_child_bone() {
    return AvatarSkeletonTrait::BoneType::NonSkeletonChild;
}

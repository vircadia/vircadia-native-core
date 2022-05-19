//
//  avatars.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 16 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_AVATARS_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_AVATARS_H

#include <stdint.h>

#include "common.h"

struct vircadia_vector {
    float x;
    float y;
    float z;
};

struct vircadia_quaternion {
    float x;
    float y;
    float z;
    float w;
};

struct vircadia_bounds {
    vircadia_vector dimensions;
    vircadia_vector offset;
};

struct vircadia_vantage {
    vircadia_vector position;
    vircadia_quaternion rotation;
};

struct vircadia_transform {
    vircadia_vantage vantage;
    float scale;
};

struct vircadia_avatar_additional_flags{
    uint8_t hand_state;
    uint8_t key_state;
    uint8_t has_head_scripted_blendshapes;
    uint8_t has_procedural_eye_movement;
    uint8_t has_audio_face_movement;
    uint8_t has_procedural_eye_face_movement;
    uint8_t has_procedural_blink_face_movement;
    uint8_t collides_with_avatars;
    uint8_t has_priority;
};

struct vircadia_avatar_parent_info {
    uint8_t uuid[16];
    uint16_t joint_index;
};

struct vircadia_avatar_hand_controllers {
    vircadia_vantage left;
    vircadia_vantage right;
};

struct vircadia_avatar_face_tracker_info {
    float left_eye_blink;
    float right_eye_blink;
    float average_loundness;
    float brow_audio_lift;
    uint8_t blendshape_coefficients_size;
    float blendshape_coefficients[256];
};

struct vircadia_far_grab_joints {
    vircadia_vantage left;
    vircadia_vantage right;
    vircadia_vantage mouse;
};

struct vircadia_joint_flags {
    uint8_t translation_is_default;
    uint8_t rotation_is_default;
};

struct vircadia_avatar_attachment {
    const char* model_url;
    const char* jount_name;
    vircadia_vantage vantage;
    float scale;
    uint8_t isSoft;
};

VIRCADIA_CLIENT_DYN_API
int vircadia_enable_avatars(int context_id);

VIRCADIA_CLIENT_DYN_API
int vircadia_update_avatars(int context_id);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_display_name(int context_id, const char* display_name);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_is_replicated(int context_id, uint8_t is_replicated);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_look_at_snapping(int context_id, uint8_t look_at_snapping_enabled);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_verification(int context_id, uint8_t verification_failed);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachments(int context_id, vircadia_avatar_attachment* attachments, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_session_display_name(int context_id, const char* session_display_name);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_model_url(int context_id, const char* skeleton_model_url);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_global_position(int context_id, vircadia_vector position);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bounding_box(int context_id, vircadia_bounds bounding_box);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_orientation(int context_id, vircadia_quaternion orientation);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_scale(int context_id, float scale);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_look_at(int context_id, vircadia_vector position);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_audio_loudness(int context_id, float loudness);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_sensor_to_world(int context_id, vircadia_transform transform);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_additional_flags(int context_id, vircadia_avatar_additional_flags flags);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_parent_info(int context_id, vircadia_avatar_parent_info info);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_local_position(int context_id, vircadia_vector position);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_hand_controllers(int context_id, vircadia_avatar_hand_controllers controllers);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_face_tracker_info(int context_id, vircadia_avatar_face_tracker_info info);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_data(int context_id, vircadia_vantage* joints, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags(int context_id, vircadia_joint_flags* joints, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_grab_joints(int context_id, vircadia_far_grab_joints joints);

#endif /* end of include guard */

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
    const char* joint_name;
    vircadia_transform transform;
    uint8_t is_soft;
};

struct vircadia_avatar_attachment_result {
    vircadia_avatar_attachment result;
    int error;
};

struct vircadia_avatar_bone {
    uint8_t type;
    vircadia_transform default_transform;
    int index;
    int parent_index;
    const char* name;
};

struct vircadia_avatar_bone_result {
    vircadia_avatar_bone result;
    int error;
};

struct vircadia_avatar_grab {
    uint8_t target_id[16];
    int joint_index;
    vircadia_vantage offset;
};

struct vircadia_avatar_grab_result {
    const uint8_t* id;
    vircadia_avatar_grab result;
    int error;
};

struct vircadia_conical_view_frustum {
    vircadia_vector position;
    vircadia_vector direction;
    float angle;
    float radius;
    float far_clip;
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
int vircadia_set_my_avatar_attachment(int context_id, int attachment_index, vircadia_avatar_attachment attachment);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachment_count(int context_id, int attachment_count);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachments(int context_id, vircadia_avatar_attachment* attachments, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_session_display_name(int context_id, const char* session_display_name);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_model_url(int context_id, const char* skeleton_model_url);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bone_count(int context_id, int bone_count);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bone(int context_id, int bone_index, vircadia_avatar_bone bone);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_data(int context_id, vircadia_avatar_bone* data, int size);

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
int vircadia_set_my_avatar_joint_count(int context_id, int joint_count);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint(int context_id, int joint_index, vircadia_vantage joint);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_data(int context_id, const vircadia_vantage* joints, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags_count(int context_id, int joint_count);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags(int context_id, int joint_index, vircadia_joint_flags flags);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_all_joint_flags(int context_id, const vircadia_joint_flags* flags, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_grab_joints(int context_id, vircadia_far_grab_joints joints);

VIRCADIA_CLIENT_DYN_API
int vircadia_my_avatar_grab(int context_id, vircadia_avatar_grab grab);

VIRCADIA_CLIENT_DYN_API
int vircadia_my_avatar_release_grab(int context_id, const uint8_t* uuid);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_avatar_view_count(int context_id, int view_count);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_avatar_view(int context_id, int view_index, vircadia_conical_view_frustum view_frustum);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_count(int context_id);

VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_get_avatar_uuid(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_display_name(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_is_replicated(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_look_at_snapping(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_verification_failed(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_attachment_count(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
vircadia_avatar_attachment_result vircadia_get_avatar_attachment(int context_id, int avatar_index, int attachment_index);

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_session_display_name(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_skeleton_model_url(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_bone_count(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
vircadia_avatar_bone_result vircadia_get_avatar_bone(int context_id, int avatar_index, int bone_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_global_position(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_bounds* vircadia_get_avatar_bounding_box(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_quaternion* vircadia_get_avatar_orientation(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const float* vircadia_get_avatar_scale(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_look_at_position(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const float* vircadia_get_avatar_audio_loudness(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_transform* vircadia_get_avatar_sensor_to_world(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_additional_flags*  vircadia_get_avatar_additional_flags(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_parent_info* vircadia_get_avatar_parent_info(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_local_position(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_hand_controllers* vircadia_get_avatar_hand_controllers(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_face_tracker_info* vircadia_get_avatar_face_tracker_info(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_joint_count(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_vantage* vircadia_get_avatar_joint(int context_id, int avatar_index, int joint_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_joint_flags_count(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_joint_flags* vircadia_get_avatar_joint_flags(int context_id, int avatar_index, int joint_index);

VIRCADIA_CLIENT_DYN_API
const vircadia_far_grab_joints* vircadia_get_avatar_grab_joints(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_grabs_count(int context_id, int avatar_index);

VIRCADIA_CLIENT_DYN_API
vircadia_avatar_grab_result vircadia_get_avatar_grab(int context_id, int avatar_index, int grab_index);

#endif /* end of include guard */

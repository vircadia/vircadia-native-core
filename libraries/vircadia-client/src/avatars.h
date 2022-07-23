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
#include "geometry.h"

/// @brief Defines the layout of additional avatar flags.
///
/// Used with vircadia_set_my_avatar_additional_flags() and
/// vircadia_get_avatar_additional_flags()
struct vircadia_avatar_additional_flags{
    /// @brief A bit set for the pointing state of the avatar's hands.
    ///
    /// This controls where the laser emanates from. If the index
    /// finger is pointing the laser emanates from the tip of that
    /// finger, otherwise it emanates from the palm.
    ///
    /// Valid values are any combination(including 0) of: \n
    /// vircadia_avatar_left_hand_pointing() \n
    /// vircadia_avatar_right_hand_pointing() \n
    /// vircadia_avatar_index_finger_pointing() \n
    uint8_t hand_state;

    // Indicates the state of some keyboard keys (???)
    //
    // Seems to be unused throughout the codebase. \n
    // Valid values are: \n
    // 0 - no key is down, \n
    // 1 - insert key is down, \n
    // 2 - delete key is down \n
    /// @private
    uint8_t key_state;

    /// @brief Indicates that the avatar face movement is controlled by
    /// code/script.
    uint8_t has_head_scripted_blendshapes;

    // TODO: document or mark private
    uint8_t has_procedural_eye_movement;

    /// @brief Indicates that the avatar's mouth blend shapes animate
    /// automatically based on detected microphone input.
    ///
    /// Set this to 0 to fully control the mouth facial blend shapes.
    uint8_t has_audio_face_movement;

    /// @brief Indicates that the facial blend shapes for avatar's eyes adjust
    /// automatically as the eyes move.
    ///
    /// This can be set to 1 prevent the iris from being obscured by the upper
    /// or lower lids, or to 0 to have full control of the eye blend shapes.
    uint8_t has_procedural_eye_face_movement;

    /// @brief Indicates whether the avatar blinks automatically by animating
    /// facial blend shapes.
    ///
    /// Set to 0 to to fully control the blink facial blend shapes.
    uint8_t has_procedural_blink_face_movement;

    /// @brief Indicates that the avatar can collide with other avatars.
    uint8_t collides_with_avatars;

    /// @brief Read only flag, indicates that the avatar is in "hero" zone.
    uint8_t has_priority;
};

/// @brief Defines the layout of avatar parent information.
///
/// Used with vircadia_set_my_avatar_parent_info() and
/// vircadia_get_avatar_parent_info().
/// Must be set along with local position
/// vircadia_set_my_avatar_local_position() /
/// vircadia_get_avatar_local_position().
struct vircadia_avatar_parent_info {
    /// @brief RFC4122 identifier of the parent.
    uint8_t uuid[16];
    /// @brief Joint index of the parent that avatar is relative to.
    uint16_t joint_index;
};

/// @brief Defines the layout of avatar hand controller information.
///
/// Used with vircadia_set_my_avatar_hand_controllers() and
/// vircadia_get_avatar_hand_controllers()
struct vircadia_avatar_hand_controllers {
    /// @brief The rotation and translation of the left hand controller
    /// relative to the avatar.
    vircadia_vantage left;
    /// @brief The rotation and translation of the left hand controller
    /// relative to the avatar.
    vircadia_vantage right;
};

/// @brief Defines the layout of avatar hand controller information.
///
/// Used with vircadia_set_my_avatar_face_tracker_info() and
/// vircadia_get_avatar_face_tracker_info()
struct vircadia_avatar_face_tracker_info {
    /// @brief Unused.
    float left_eye_blink;
    /// @brief Unused.
    float right_eye_blink;
    /// @brief Unused.
    float average_loundness;
    /// @brief Unused.
    float brow_audio_lift;

    /// @brief The size of facial animation blend shape coefficients array.
    uint8_t blendshape_coefficients_size;
    /// @brief The facial animation blend shape coefficients array.
    float blendshape_coefficients[256];
};

/// @brief Defines the layout of avatar controller far grab joints data.
///
/// Used with vircadia_set_my_avatar_grab_joints() and
/// vircadia_get_avatar_grab_joints()
struct vircadia_far_grab_joints {
    /// @brief Left controller far grab joint position and rotation.
    vircadia_vantage left;
    /// @brief Right controller far grab joints position and rotation.
    vircadia_vantage right;
    /// @brief Mouse far grab joint position and rotation.
    vircadia_vantage mouse;
};

/// @brief Defines the layout of avatar joint flags.
///
/// Used with vircadia_set_my_avatar_joint_flags() and
/// vircadia_get_avatar_joint_flags().
struct vircadia_joint_flags {

    /// @brief The specified joint translation is in default pose.
    ///
    /// If this is set the corresponding joint translation value
    /// (vircadia_set_my_avatar_joint() / vircadia_get_avatar_joint) has no
    /// meaning.
    uint8_t translation_is_default;

    /// @brief The specified joint rotation is in default pose.
    ///
    /// If this is set the corresponding joint rotation value
    /// (vircadia_set_my_avatar_joint() / vircadia_get_avatar_joint) has no
    /// meaning.
    uint8_t rotation_is_default;

};

/// @brief Defines the layout of avatar attachment data.
///
/// Used with vircadia_set_my_avatar_attachment() and
/// vircadia_get_avatar_attachment().
struct vircadia_avatar_attachment {

    /// @brief URL of the attachment model.
    const char* model_url;

    /// @brief The name of the avatar model is attached to.
    const char* joint_name;

    /// @brief The linear transformation of the model relative to the joint it's
    /// attached to.
    vircadia_transform transform;

    /// @brief Indicates that the model has a skeleton,
    /// which is rotated to fit avatar's current pose.
    ///
    /// This is used, for example, to have clothing that
    /// moves with avatar.  If this is set the
    /// vircadia_avatar_attachment::transform is
    /// ignored.
    uint8_t is_soft;

};

/// @brief Represents avatar attachment data or an error.
///
/// Used with vircadia_get_avatar_attachment().
struct vircadia_avatar_attachment_result {

    /// @brief The attachment data.
    vircadia_avatar_attachment result;

    /// @brief The status of the result.
    ///
    /// 0 - success. \n
    /// negative value - error occurred, and the
    /// vircadia_avatar_attachment_result::result has no meaning. \n
    /// Possible error codes: \n
    /// vircadia_error_context_invalid() \n
    /// vircadia_error_context_loss() \n
    /// vircadia_error_avatar_invalid() \n
    /// vircadia_error_avatar_attachment_invalid()
    int error;

};

/// @brief Defines the layout of avatar skeleton bone/joint data.
///
/// Used with vircadia_set_my_avatar_bone() and vircadia_get_avatar_bone().
struct vircadia_avatar_bone {

    /// @brief The type of the bone/joint.
    ///
    /// Valid values:
    /// vircadia_avatar_skeleton_root_bone() \n
    /// vircadia_avatar_skeleton_child_bone() \n
    /// vircadia_avatar_root_bone() \n
    /// vircadia_avatar_child_bone()
    uint8_t type;

    /// @brief The default pose of the bone/joint.
    vircadia_transform default_transform;

    /// @brief The index of the bone/joint.
    int index;

    /// @brief The index of the parent bone/joint.
    int parent_index;

    /// @brief The name of the bone/joint.
    const char* name;

};

/// @brief Represents avatar skeleton bone/joint data or an error.
///
/// Used with vircadia_get_avatar_bone().
struct vircadia_avatar_bone_result {

    /// @brief The skeleton bone/joint data.
    vircadia_avatar_bone result;

    /// @brief The status of the result.
    ///
    /// 0 - success. \n
    /// negative value - error occurred, and the
    /// vircadia_avatar_bone_result::result has no meaning. \n
    /// Possible error codes: \n
    /// vircadia_error_context_invalid() \n
    /// vircadia_error_context_loss() \n
    /// vircadia_error_avatar_invalid() \n
    /// vircadia_error_avatar_bone_invalid()
    int error;

};

/// @brief Defines the layout of avatar grab data.
///
/// Used with vircadia_my_avatar_grab() and vircadia_get_avatar_grab().
struct vircadia_avatar_grab {

    /// @brief RFC4122 identifier of the grabbed object.
    uint8_t target_id[16];

    /// @brief The index of the grabbing joint.
    int joint_index;

    /// @brief The translation and rotation of the grabbed object relative to
    /// the grabbing joint.
    vircadia_vantage offset;

};

/// @brief Represents avatar grab data or an error.
///
/// Used with vircadia_get_avatar_grab().
struct vircadia_avatar_grab_result {

    /// @brief RFC4122 identifier of the grab action.
    ///
    /// Pass it vircadia_my_avatar_release_grab() to release the grabbed
    /// object.
    const uint8_t* id;

    /// @brief The grab data.
    vircadia_avatar_grab result;

    /// @brief The status of the result.
    ///
    /// 0 - success. \n
    /// negative value - error occurred, vircadia_avatar_grab_result::id is
    /// null, and the vircadia_avatar_bone_result::result has no meaning. \n
    /// Possible error codes: \n
    /// vircadia_error_context_invalid() \n
    /// vircadia_error_context_loss() \n
    /// vircadia_error_avatar_invalid() \n
    /// vircadia_error_avatar_bone_invalid()
    int error;

};

/// @brief Defines the layout of conical view frustum.
///
/// Used with vircadia_set_avatar_view().
struct vircadia_conical_view_frustum {

    /// @brief The position of the viewer.
    vircadia_vector position;

    /// @brief The direction of the view.
    vircadia_vector direction;

    /// @brief The angle of the cone.
    float angle;

    /// @brief The radius of the sphere around the viewer, separate from the cone.
    float radius;

    /// @brief The far clip plane of the view.
    float far_clip;

};

/// @brief Defines the layout of view frustum.
///
/// This is a representation of a view frustum based on near clip plane
/// corners.
///
/// Used with vircadia_set_avatar_view_corners().
struct vircadia_view_frustum_corners {

    /// @brief The position of the viewer.
    vircadia_vector position;

    /// @brief The radius of the sphere around the viewer.
    float radius;

    /// @brief The far clip plane of the view.
    float far_clip;

    /// @brief The top left corner of the near clip plane.
    vircadia_vector near_top_left;

    /// @brief The top right corner of the near clip plane.
    vircadia_vector near_top_right;

    /// @brief The bottom left corner of the near clip plane.
    vircadia_vector near_bottom_left;

    /// @brief The bottom right corner of the near clip plane.
    vircadia_vector near_bottom_right;

};

/// @brief Enable handling of avatars.
///
/// Avatar data will be received/sent from/to internal buffers accessible
/// through API functions. This introduces fixed size memory overhead per
/// avatar. Afterwards vircadia_update_avatars() must be called periodically to
/// update internal buffers for sending and receiving.
///
/// @param context_id - The id of the context (context.h).
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss()
VIRCADIA_CLIENT_DYN_API
int vircadia_enable_avatars(int context_id);

/// @brief Updates internal avatar data buffers.
///
/// This makes the avatar data received from the mixer up to the point of the
/// call available for reading immediately, and buffers the set avatar data up
/// to the point of the call to be sent to the mixer asynchronously.
///
/// @param context_id - The id of the context (context.h).
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_update_avatars(int context_id);

/// @brief Sets the avatar display to be sent to the mixer.
///
/// This might not be exactly what other's see, the mixer may sanitize and
/// de-duplicate the name.
///
/// @param context_id - The id of the context (context.h).
/// @param display_name - A null terminated string to set as display name. Must
/// not be null.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_display_name(int context_id, const char* display_name);

// not sure if this is read-only
/// @private
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_is_replicated(int context_id, uint8_t is_replicated);

/// @brief Sets the avatar "look at snapping" flag to be sent to the mixer.
///
/// If enabled make the avatar's eyes snap to look at another avatar's eyes
/// when the other avatar is in the line of sight and also has this flag set.
///
/// @param context_id - The id of the context (context.h).
/// @param look_at_snapping_enabled - 0 - disabled, 1 - enabled.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_look_at_snapping(int context_id, uint8_t look_at_snapping_enabled);

// not sure if this is read-only
/// @private
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_verification(int context_id, uint8_t verification_failed);

/// @brief Sets avatar attachment data at a specified index to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param attachment_index - The index of the attachment. Must be in range set
/// by vircadia_set_my_avatar_attachment_count().
/// @param attachment - The data of the attachment. The pointer members must
/// not be null.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_attachment_invalid() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachment(int context_id, int attachment_index, vircadia_avatar_attachment attachment);

/// @brief Sets the total count of avatar attachments be sent to the mixer.
///
/// All the attachment data must be initialized with
/// vircadia_set_my_avatar_attachment(). Increasing the count from the
/// previously set value preserves the data. Decreasing the count erases the
/// data past the newly set value.
///
/// @param context_id - The id of the context (context.h).
/// @param attachment_count - The count of attachments. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachment_count(int context_id, int attachment_count);

/// @brief Sets avatar attachments data be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param attachments - The array of attachments. Must not be null unless
/// attachment_count is 0.
/// @param attachment_count - The count of attachments. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_attachments(int context_id, const vircadia_avatar_attachment* attachments, int size);

// this is read-only
// @private
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_session_display_name(int context_id, const char* session_display_name);

/// @brief Sets the URL of avatar model and skeleton data to be sent to the
/// mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param skeleton_model_url - Null terminated URL string. Must not be null.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_model_url(int context_id, const char* skeleton_model_url);

/// @brief Sets the total count of avatar skeleton bones be sent to the mixer.
///
/// All the bone data must be initialized with vircadia_set_my_avatar_bone().
/// Increasing the count from the previously set value preserves the data.
/// Decreasing the count erases the data past the newly set value.
///
/// @param context_id - The id of the context (context.h).
/// @param bone_count - The count of bones. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bone_count(int context_id, int bone_count);

/// @brief Sets avatar skeleton bone data at a specified index to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param bone_index - The index of the bone. Must be in range set by
/// vircadia_set_my_avatar_bone_count().
/// @param bone - The data of the bone. The pointer members must not be null.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_bone_invalid() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bone(int context_id, int bone_index, vircadia_avatar_bone bone);

/// @brief Sets avatar skeleton data be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param data - The array of skeleton bones. Must not be null unless size is
/// 0.
/// @param size - The count of skeleton bones. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_skeleton_data(int context_id, vircadia_avatar_bone* data, int size);

/// @brief Sets avatar global position to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param position - The position vector.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_global_position(int context_id, vircadia_vector position);

/// @brief Sets avatar bounding box to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param bounding_box - The bounding box data.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_bounding_box(int context_id, vircadia_bounds bounding_box);

/// @brief Sets avatar orientation to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param orientation - The orientation quaternion.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_orientation(int context_id, vircadia_quaternion orientation);

/// @brief Sets avatar scale to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param scale - The scale value.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_scale(int context_id, float scale);

/// @brief Sets avatar "look at" position to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param position - The position vector.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_look_at(int context_id, vircadia_vector position);

/// @brief Sets avatar audio loudness to be sent to the mixer.
///
/// This represents the instantaneous loudness of the audio input.
///
/// @param context_id - The id of the context (context.h).
/// @param loudness - The loudness value.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_audio_loudness(int context_id, float loudness);

/// @brief Sets avatar sensor to world transform to be sent to the mixer.
///
/// This is the scale, rotation and translation transform from the user's real
/// world to the avatar's size, orientation and position in the virtual world.
///
/// @param context_id - The id of the context (context.h).
/// @param transform - The linear transformation data.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_sensor_to_world(int context_id, vircadia_transform transform);

/// @brief Sets avatar's additional flags to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param flags - The flags data.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_additional_flags(int context_id, vircadia_avatar_additional_flags flags);

/// @brief Sets avatar's parent information to be sent to the mixer.
///
/// Avatar's local position (vircadia_set_my_avatar_local_position()) is
/// relative to this parent.
///
/// @param context_id - The id of the context (context.h).
/// @param info - The parent information.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_parent_info(int context_id, vircadia_avatar_parent_info info);

/// @brief Sets avatar's local position to be sent to the mixer.
///
/// This is relative to avatar's parent, which must be set with
/// vircadia_set_my_avatar_parent_info().
///
/// @param context_id - The id of the context (context.h).
/// @param position - The position vector.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_local_position(int context_id, vircadia_vector position);

/// @brief Sets avatar hand controller data to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param controllers - The position and rotation of controllers.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_hand_controllers(int context_id, vircadia_avatar_hand_controllers controllers);

/// @brief Sets avatar's facial animation data to be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param info - The facial animation data.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_face_tracker_info(int context_id, vircadia_avatar_face_tracker_info info);

/// @brief Sets the total count of avatar current pose joints be sent to the
/// mixer.
///
/// These values define the current pose of the avatar. The individual joint
/// data can be set with vircadia_set_my_avatar_joint(). Increasing the count
/// from the previously set value preserves the data. Decreasing the count
/// erases the data past the newly set value.
///
/// @param context_id - The id of the context (context.h).
/// @param joint_count - The count of current pose joints. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_count(int context_id, int joint_count);

/// @brief Sets avatar current pose joint data at a specified index to be sent
/// to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param joint_index - The index of the joint. Must be in range set by
/// vircadia_set_my_avatar_joint_count().
/// @param joint - The position and rotation of the joint.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_joint_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint(int context_id, int joint_index, vircadia_vantage joint);

/// @brief Sets avatar current pose data be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param data - The array of joints that define the current pose. Must not be
/// null unless size is 0.
/// @param size - The count of joints. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_data(int context_id, const vircadia_vantage* joints, int size);

/// @brief Sets the total count of avatar current pose joint flags be sent to the
/// mixer.
///
/// These values define the current pose of the avatar. The individual flag
/// data can be set with vircadia_set_my_avatar_joint_flags(). Increasing the
/// count from the previously set value preserves the data. Decreasing the
/// count erases the data past the newly set value.
///
/// @param context_id - The id of the context (context.h).
/// @param joint_count - The count of current pose joint flags. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags_count(int context_id, int joint_count);

/// @brief Sets avatar current pose joint flags at a specified index to be sent
/// to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param joint_index - The index of the joint. Must be in range set by
/// vircadia_set_my_avatar_joint_flags_count().
/// @param flags - The joint flags.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_joint_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_joint_flags(int context_id, int joint_index, vircadia_joint_flags flags);

/// @brief Sets avatar current pose flags be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param data - The array of joint flags that define the current pose. Must
/// not be null unless size is 0.
/// @param size - The count of joints. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_all_joint_flags(int context_id, const vircadia_joint_flags* flags, int size);

/// @brief Sets avatar grab joint data be sent to the mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param joints - The far grab joint data.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_grab_joints(int context_id, vircadia_far_grab_joints joints);

/// @brief Creates a new grab action for the avatar.
///
/// @param context_id - The id of the context (context.h).
/// @param grab - The grab action data.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_my_avatar_grab(int context_id, vircadia_avatar_grab grab);

/// @brief Released/Destroys an existing grab action of the avatar.
///
/// @param context_id - The id of the context (context.h).
/// @param uuid - An RFC4122 identifier of the grab action. Must not be null.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_my_avatar_release_grab(int context_id, const uint8_t* uuid);

/// @brief Sets the total count of camera views to be sent to the mixer.
///
/// The mixer will use these to cull the avatar data sent back to the client.
/// The individual view data can be set with vircadia_set_avatar_view() or
/// vircadia_set_avatar_view_corners(). Increasing the count from the
/// previously set value preserves the data. Decreasing the count erases the
/// data past the newly set value.
///
/// @param context_id - The id of the context (context.h).
/// @param view_count - The count of camera views. Must not be negative.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_argument_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_avatar_view_count(int context_id, int view_count);

/// @brief Sets the camera view data at a specified index to be sent to the
/// mixer.
///
/// @param context_id - The id of the context (context.h).
/// @param view_index - The index of the view. Must be in range set by
/// vircadia_set_avatar_view_count().
/// @param view_frustum - The conical frustum of the camera view.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_view_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_avatar_view(int context_id, int view_index, vircadia_conical_view_frustum view_frustum);

/// @brief Sets the camera view data at a specified index to be sent to the
/// mixer.
///
/// Unlike vircadia_set_avatar_view(), this function accepts usual rectangular
/// near plane corners instead of direction and angle of the cone and performs
/// a conversion under the hood.
///
/// @param context_id - The id of the context (context.h).
/// @param view_index - The index of the view. Must be in range set by
/// vircadia_set_avatar_view_count().
/// @param view_frustum_corners - The corner data of the camera view frustum.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_view_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_avatar_view_corners(int context_id, int view_index, vircadia_view_frustum_corners view_frustum_corners);

/// @brief Gets the total count of avatars in the current domain.
///
/// @param context_id - The id of the context (context.h).
///
/// @return The avatar count, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_count(int context_id);

/// @brief Gets the RFC4122 identifier of the specified avatar.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return The identifier byte array, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_get_avatar_uuid(int context_id, int avatar_index);

/// @brief Gets the display name of the specified avatar.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A null terminated string, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_display_name(int context_id, int avatar_index);

/// @brief Indicates that the avatar is a replica of another (used for
/// testing purposes).
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return 1 - is a replica, 0 - is not a replica, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_is_replicated(int context_id, int avatar_index);

/// @brief Indicates that the avatar has "look at snapping" enabled.
///
/// If enabled make the avatar's eyes snap to look at another avatar's eyes
/// when the other avatar is in the line of sight and also has this flag set.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return 1 - enabled, 0 - disabled, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_look_at_snapping(int context_id, int avatar_index);

/// @brief Indicates that the specified avatar's model failed identity
/// verification.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return 1 - verification failed, 0 - verification succeeded, or a negative
/// error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_verification_failed(int context_id, int avatar_index);

/// @brief Gets the total count of the specified avatar's attachments.
///
/// Individual attachment data can be accessed with
/// vircadia_get_avatar_attachment().
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return The count of attachments, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_attachment_count(int context_id, int avatar_index);

/// @brief Gets the specified avatar's attachments data.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
/// @param attachment_index - The attachment index in the internal buffer. Must
/// be in range (vircadia_get_avatar_attachment_count()).
///
/// @return Attachment data or a negative error code.
VIRCADIA_CLIENT_DYN_API
vircadia_avatar_attachment_result vircadia_get_avatar_attachment(int context_id, int avatar_index, int attachment_index);

/// @brief Gets the specified avatar's session display name.
///
/// This is a sanitized and default version of the avatar's name defined by the
/// mixer rather than clients. It's unique among all the avatars present in the
/// domain.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return Null terminated string or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_session_display_name(int context_id, int avatar_index);

/// @brief Gets the URL of the specified avatar's skeleton and model.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return Null terminated string or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_avatar_skeleton_model_url(int context_id, int avatar_index);

/// @brief Gets the total count of the specified avatar's skeleton bones.
///
/// Individual bone data can be accessed with vircadia_get_avatar_bone().
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return The count of bones, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_bone_count(int context_id, int avatar_index);

/// @brief Gets the specified avatar's bone data.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
/// @param bone_index - The bone index in the internal buffer. Must
/// be in range (vircadia_get_avatar_bone_count()).
///
/// @return Bone data or a negative error code.
VIRCADIA_CLIENT_DYN_API
vircadia_avatar_bone_result vircadia_get_avatar_bone(int context_id, int avatar_index, int bone_index);

/// @brief Gets the specified avatar's global position.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the position vector, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_global_position(int context_id, int avatar_index);

/// @brief Gets the specified avatar's bounding box.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the bounding box data, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_bounds* vircadia_get_avatar_bounding_box(int context_id, int avatar_index);

/// @brief Gets the specified avatar's orientation.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the orientation quaternion, or null in case of an
/// error.
VIRCADIA_CLIENT_DYN_API
const vircadia_quaternion* vircadia_get_avatar_orientation(int context_id, int avatar_index);

/// @brief Gets the specified avatar's scale.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the scale value, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const float* vircadia_get_avatar_scale(int context_id, int avatar_index);

/// @brief Gets the specified avatar's "look at" position.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the position vector, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_look_at_position(int context_id, int avatar_index);

/// @brief Gets the specified avatar's audio loudness value.
///
/// This represents the instantaneous loudness of the audio input.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the loudness value, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const float* vircadia_get_avatar_audio_loudness(int context_id, int avatar_index);

/// @brief Gets the specified avatar's sensor to world transform.
///
/// This is the scale, rotation and translation transform from the user's real
/// world to the avatar's size, orientation and position in the virtual world.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the transform, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_transform* vircadia_get_avatar_sensor_to_world(int context_id, int avatar_index);

/// @brief Gets the specified avatar's additional flag values.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the flag values, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_additional_flags*  vircadia_get_avatar_additional_flags(int context_id, int avatar_index);

/// @brief Gets the specified avatar's parent information.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the flag values, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_parent_info* vircadia_get_avatar_parent_info(int context_id, int avatar_index);

/// @brief Gets the specified avatar's local position vector.
///
/// This is the position relative to the parent
/// (vircadia_get_avatar_parent_info()).
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the position vector, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_vector* vircadia_get_avatar_local_position(int context_id, int avatar_index);

/// @brief Gets the specified avatar's hand controller data.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the controller data, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_hand_controllers* vircadia_get_avatar_hand_controllers(int context_id, int avatar_index);

/// @brief Gets the specified avatar's facial animation data.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to the facial animation data, or null in case of an
/// error.
VIRCADIA_CLIENT_DYN_API
const vircadia_avatar_face_tracker_info* vircadia_get_avatar_face_tracker_info(int context_id, int avatar_index);

/// @brief Gets the total count of the specified avatar's current pose joints.
///
/// Individual joint data can be accessed with vircadia_get_avatar_joint().
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return The count of joints, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_joint_count(int context_id, int avatar_index);

/// @brief Gets the specified avatar's current pose joint data.
///
/// This joint data defines the current pose of the avatar, together with joint
/// flags (vircadia_get_avatar_joint_flags()).
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
/// @param joint_index - The joint index in the internal buffer. Must be in
/// range (vircadia_get_avatar_joint_count()).
///
/// @return A pointer to joint position and rotation, or null in case of an
/// error.
VIRCADIA_CLIENT_DYN_API
const vircadia_vantage* vircadia_get_avatar_joint(int context_id, int avatar_index, int joint_index);

/// @brief Gets the total count of the specified avatar's current pose joints.
///
/// Individual joint flags can be accessed with vircadia_get_avatar_joint_flags().
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return The count of joint flags, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_joint_flags_count(int context_id, int avatar_index);

/// @brief Gets the specified avatar's current pose joint flags.
///
/// This joint flags defines the current pose of the avatar, together with
/// joint data (vircadia_get_avatar_joint()).
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
/// @param joint_index - The joint flags index in the internal buffer. Must be
/// in range (vircadia_get_avatar_joint_flags_count()).
///
/// @return A pointer to joint flags, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_joint_flags* vircadia_get_avatar_joint_flags(int context_id, int avatar_index, int joint_index);

/// @brief Gets the specified avatar's grab joints data.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return A pointer to grab joint data, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const vircadia_far_grab_joints* vircadia_get_avatar_grab_joints(int context_id, int avatar_index);

/// @brief Gets the total count of the specified avatar's grab actions.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
///
/// @return The count of grab actions, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_grabs_count(int context_id, int avatar_index);

/// @brief Gets the specified avatar's grab action data.
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The avatar index in the internal buffer. Must be in
/// range (vircadia_get_avatar_count()).
/// @param grab_index - The grab action index in the internal buffer. Must be
/// in range (vircadia_get_avatar_grabs_count()).
///
/// @return Grab action data or a negative error code.
VIRCADIA_CLIENT_DYN_API
vircadia_avatar_grab_result vircadia_get_avatar_grab(int context_id, int avatar_index, int grab_index);

/// @brief Gets the total count of avatar disconnections.
///
/// The list of disconnected avatars is updated with vircadia_update_avatars().
/// Information of specific disconnection can be retrieved with
/// vircadia_get_avatar_disconnection_uuid(),
/// vircadia_get_avatar_disconnection_reason(),
///
/// @param context_id - The id of the context (context.h).
///
/// @return The count of disconnected avatars, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_disconnection_count(int context_id);

/// @brief Gets the UUID of specified disconnected avatar.
///
/// @param context_id - The id of the context (context.h).
/// @param disconnection_index - The avatar index in the internal buffer of
/// avatars disconnection. Must be in range
/// (vircadia_get_avatar_disconnection_count()).
///
/// @return RFC4122 avatar identifier or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_get_avatar_disconnection_uuid(int context_id, int disconnection_index);

/// @brief Gets the reason of specified avatar's disconnection.
///
/// @param context_id - The id of the context (context.h).
/// @param disconnection_index - The index in the internal buffer of
/// avatar disconnections. Must be in range
/// (vircadia_get_avatar_disconnection_count()).
///
/// @return The reason code of avatar disconnection, or a negative error code. \n
///
/// Reason codes: \n
/// vircadia_avatar_disconnection_unknown() \n
/// vircadia_avatar_disconnection_normal() \n
/// vircadia_avatar_disconnection_ignored() \n
/// vircadia_avatar_disconnection_they_entered_bubble() \n
/// vircadia_avatar_disconnection_you_entered_bubble() \n
///
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_disconnection_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_avatar_disconnection_reason(int context_id, int disconnection_index);

/// @brief Checks if given avatar data has been updated.
///
/// This function checks whether an individual avatar's data has been updated
/// by the last call to vircadia_update_avatars().
///
/// @param context_id - The id of the context (context.h).
/// @param avatar_index - The index in the internal buffer of avatars. Must be
/// in range (vircadia_get_avatar_count()).
///
/// @return 0 - avatar not updated, 1 - avatar updated, or a negative error code. \n
///
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_avatars_disabled() \n
/// vircadia_error_avatar_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_avatar_changed(int context_id, int avatar_index);

#endif /* end of include guard */

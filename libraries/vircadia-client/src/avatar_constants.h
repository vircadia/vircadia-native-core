//
//  avatar_constants.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 9 June 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_AVATAR_CONSTANTS_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_AVATAR_CONSTANTS_H

#include <stdint.h>

#include "common.h"

/// @brief Flag value indicating that the avatar's left hand is pointing.
///
/// See vircadia_avatar_additional_flags::hand_state
///
/// @return 0b1
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_left_hand_pointing();

/// @brief Flag value indicating that the avatar's right hand is pointing.
///
/// See vircadia_avatar_additional_flags::hand_state
///
/// @return 0b10
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_right_hand_pointing();

/// @brief Flag value indicating that the avatar's index finger on either hand
/// is pointing.
///
/// See vircadia_avatar_additional_flags::hand_state
///
/// @return 0b100
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_index_finger_pointing();

/// @brief The value corresponding to skeleton root bone type.
///
/// See vircadia_avatar_bone::type
///
/// @return 0
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_skeleton_root_bone();

/// @brief The value corresponding to skeleton child bone type.
///
/// See vircadia_avatar_bone::type
///
/// @return 1
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_skeleton_child_bone();

/// @brief The value corresponding to non-skeleton root bone type.
///
/// See vircadia_avatar_bone::type
///
/// @return 2
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_root_bone();

/// @brief The value corresponding to non-skeleton child bone type.
///
/// See vircadia_avatar_bone::type
///
/// @return 3
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_child_bone();

/// @brief TODO
///
/// vircadia_get_avatar_disconnection_reason()
///
/// @return 0
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_disconnection_unknown();

/// @brief TODO
///
/// vircadia_get_avatar_disconnection_reason()
///
/// @return 1
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_disconnection_normal();

/// @brief TODO
///
/// vircadia_get_avatar_disconnection_reason()
///
/// @return 2
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_disconnection_ignored();

/// @brief TODO
///
/// vircadia_get_avatar_disconnection_reason()
///
/// @return 3
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_disconnection_they_entered_bubble();

/// @brief TODO
///
/// vircadia_get_avatar_disconnection_reason()
///
/// @return 4
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_disconnection_you_entered_bubble();

#endif /* end of include guard */

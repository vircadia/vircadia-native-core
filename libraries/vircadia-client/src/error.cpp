//
//  error.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "error.h"

#include "internal/Error.h"

using namespace vircadia::client;

VIRCADIA_CLIENT_DYN_API int vircadia_error_context_exists() {
    return toInt(ErrorCode::CONTEXT_EXISTS);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_context_invalid() {
    return toInt(ErrorCode::CONTEXT_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_context_loss() {
    return toInt(ErrorCode::CONTEXT_LOSS);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_node_invalid() {
    return toInt(ErrorCode::NODE_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_message_invalid() {
    return toInt(ErrorCode::MESSAGE_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_message_type_invalid() {
    return toInt(ErrorCode::MESSAGE_TYPE_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_message_type_disabled() {
    return toInt(ErrorCode::MESSAGE_TYPE_DISABLED);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_packet_write() {
    return toInt(ErrorCode::PACKET_WRITE);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_argument_invalid() {
    return toInt(ErrorCode::ARGUMENT_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_audio_disabled() {
    return toInt(ErrorCode::AUDIO_DISABLED);
}

VIRCADIA_CLIENT_DYN_API int vircadia_audio_format_invalid() {
    return toInt(ErrorCode::AUDIO_FORMAT_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_audio_context_invalid() {
    return toInt(ErrorCode::AUDIO_CONTEXT_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_audio_codec_invalid() {
    return toInt(ErrorCode::AUDIO_CODEC_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatars_disabled() {
    return toInt(ErrorCode::AVATARS_DISABLED);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_invalid() {
    return toInt(ErrorCode::AVATAR_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_attachment_invalid() {
    return toInt(ErrorCode::AVATAR_ATTACHMENT_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_joint_invalid() {
    return toInt(ErrorCode::AVATAR_JOINT_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_bone_invalid() {
    return toInt(ErrorCode::AVATAR_BONE_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_grab_invalid() {
    return toInt(ErrorCode::AVATAR_GRAB_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_entity_invalid() {
    return toInt(ErrorCode::AVATAR_ENTITY_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_view_invalid() {
    return toInt(ErrorCode::AVATAR_VIEW_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_avatar_disconnection_invalid() {
    return toInt(ErrorCode::AVATAR_EPITAPH_INVALID);
}


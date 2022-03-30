//
//  messages.h
//  libraries/client/src
//
//  Created by Nshan G. on 25 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef VIRCADIA_LIBRARIES_VIRCADIA_CLIENT_SRC_MESSAGES_H
#define VIRCADIA_LIBRARIES_VIRCADIA_CLIENT_SRC_MESSAGES_H

#include <stdint.h>

#include "common.h"

/// @brief Enable handling of messages of specified type.
///
/// Messages of specified type will be buffered as they come in, so you
/// need to make sure to call vircadia_update_messages() followed by
/// vircadia_clear_messages() for any type that is enabled, to avoid
/// unbounded memory use.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any combination of the flag values defined in
/// message_types.h.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_type_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_enable_messages(int context_id, uint8_t type);

/// @brief Subscribes to receive messages on a specific channel.
///
/// This means, for example, that if there are two Interface scripts
/// that subscribe to different channels, both scripts will receive
/// messages on both channels.
///
/// @param context_id - The id of the context (context.h).
/// @param channel - The channel to subscribe to.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss()
VIRCADIA_CLIENT_DYN_API
int vircadia_messages_subscribe(int context_id, const char* channel);

/// @brief Unsubscribes from receiving messages on a specific channel.
///
/// @param context_id - The id of the context (context.h).
/// @param channel - The channel to unsubscribe from.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss()
VIRCADIA_CLIENT_DYN_API
int vircadia_messages_unsubscribe(int context_id, const char* channel);

/// @brief Updates the list of messages.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any combination of the flag values defined in
/// message_types.h.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_type_invalid() \n
/// vircadia_error_message_type_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_update_messages(int context_id, uint8_t type);

VIRCADIA_CLIENT_DYN_API
int vircadia_messages_count(int context_id, uint8_t type);

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_message(int context_id, uint8_t type, int index);

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_message_channel(int context_id, uint8_t type, int index);

VIRCADIA_CLIENT_DYN_API
int vircadia_is_message_local_only(int context_id, uint8_t type, int index);

VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_get_message_sender(int context_id, uint8_t type, int index);

VIRCADIA_CLIENT_DYN_API
int vircadia_clear_messages(int context_id, uint8_t type);

VIRCADIA_CLIENT_DYN_API
int vircadia_send_message(int context_id, uint8_t type, const char* channel, const char* payload, uint8_t localOnly);

#endif /* end of include guard */

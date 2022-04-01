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

/// @brief Enable handling of messages of specified types.
///
/// Messages of specified type will be buffered as they come in, so you
/// need to make sure to call vircadia_update_messages() followed by
/// vircadia_clear_messages() for any type that is enabled, to avoid
/// unbounded memory use.
///
/// @param context_id - The id of the context (context.h).
/// @param types - Any combination of the flags defined in
/// message_types.h.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_type_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_enable_messages(int context_id, uint8_t types);

/// @brief Subscribes to receive messages on a specific channel.
///
/// This means, for example, that if there are two clients that
/// subscribe to different channels, both will receive messages on both
/// channels.
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

/// @brief Updates the list of messages of specified types.
///
/// @param context_id - The id of the context (context.h).
/// @param types - Any combination of the flags defined in
/// message_types.h.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_type_invalid() \n
/// vircadia_error_message_type_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_update_messages(int context_id, uint8_t types);

/// @brief Returns the size of the message list of specific type.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any single flag defined in message_types.h.
///
/// @return the count of messages on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_type_invalid() \n
/// vircadia_error_message_type_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_messages_count(int context_id, uint8_t type);

/// @brief Get a specific message from the message list of specific
/// type.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any single flag defined in message_types.h.
/// @param index - The index of the message in the list.
///
/// @return pointer to null terminated message data, or null in case of an
/// error.
VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_message(int context_id, uint8_t type, int index);

/// @brief Get the size of a specific message from the message list of
/// specific type.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any single flag defined in message_types.h.
/// @param index - The index of the message in the list.
///
/// @return the size of message returned by vircadia_get_message()
/// excluding the null terminator, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_invalid() \n
/// vircadia_error_message_type_invalid() \n
/// vircadia_error_message_type_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_message_size(int context_id, uint8_t type, int index);

/// @brief Get the channel of a specific message from the message list
/// of specific type.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any single flag defined in message_types.h.
/// @param index - The index of the message in the list.
///
/// @return pointer to null terminated channel string, or null in case
/// of an error.
VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_message_channel(int context_id, uint8_t type, int index);

/// @brief Check if the specific message from the message list of
/// specific type is local only.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any single flag defined in message_types.h.
/// @param index - The index of the message in the list.
///
/// @return 1 - message is local only, 0 - message is not local only, or
/// a negative error code \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_invalid() \n
/// vircadia_error_message_type_invalid() \n
/// vircadia_error_message_type_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_is_message_local_only(int context_id, uint8_t type, int index);

/// @brief Get the UUID of the sender of a specific message from the
/// message list of specific type.
///
/// @param context_id - The id of the context (context.h).
/// @param type - Any single flag defined in message_types.h.
/// @param index - The index of the message in the list.
///
/// @return pointer to 16 bytes of UUID, or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_get_message_sender(int context_id, uint8_t type, int index);

/// @brief Clears the list of messages of specified types.
///
/// @param context_id - The id of the context (context.h).
/// @param types - Any combination of the flags defined in
/// message_types.h.
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_type_invalid() \n
/// vircadia_error_message_type_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_clear_messages(int context_id, uint8_t types);

/// @brief Sends a message.
///
/// @param context_id - The id of the context (context.h).
/// @param types - Any combination of the flags defined in
/// message_types.h.
/// @param channel - the channel to send the message on.
/// @param payload - the main body of the message.
/// @param local - whether to send the message as local only
/// (1 - true, 0 - false).
///
/// @return 0 on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_message_type_invalid() \n
/// vircadia_error_message_type_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_send_message(int context_id, uint8_t types, const char* channel, const char* payload, uint8_t local);

#endif /* end of include guard */

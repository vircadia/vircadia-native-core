//
//  node_list.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 31 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_NODE_LIST_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_NODE_LIST_H

#include <stdint.h>

#include "common.h"

/// @brief Connect/jump to server on specified address.
///
/// This function only initiates the connection, use
/// vircadia_connection_status() to check if it was established.
///
/// @param context_id The ID of the context to use (context.h).
/// @param address The address to go to: a "hifi://" address, an IP
/// address (e.g., "127.0.0.1" or "localhost"), a file:/// address, a
/// domain name, a named path on a domain (starts with "/"), a position
/// or position and orientation, or a user (starts with "@").
///
/// @return Negative code in case of an error, otherwise 0. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
VIRCADIA_CLIENT_DYN_API
int vircadia_connect(int context_id, const char* address);

/// @brief Retrieve the connection status.
///
/// @param context_id The ID of the context to use (context.h).
/// @return 0 if not connected, 1 if connected, negative error code
/// otherwise. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
VIRCADIA_CLIENT_DYN_API
int vircadia_connection_status(int context_id);

/// @brief Update the internal list of nodes.
///
/// @param context_id The ID of the context to use (context.h).
/// @return Negative code in case of an errors, otherwise 0. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
VIRCADIA_CLIENT_DYN_API
int vircadia_update_nodes(int context_id);

/// @brief Retrieve the count of connected nodes.
//
/// The count only changes with explicit call to
/// vircadia_update_nodes().
///
/// @param context_id The ID of the context to use (context.h).
/// @return The count of connected nodes, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
VIRCADIA_CLIENT_DYN_API
int vircadia_node_count(int context_id);

/// @brief Retrieve the node UUID by index.
///
/// The list of nodes only changes with an explicit call to
/// vircadia_update_nodes(), which may invalidate the pointer returned
/// by this function.
///
/// @param context_id The ID of the context to use (context.h).
/// @param index The index of the node in the node list.
/// @return Pointer to 16 byte UUID or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_node_uuid(int context_id, int index);

/// @brief Check if the specified node is activated.
///
/// The internal list of nodes only changes with an explicit call to
/// vircadia_update_nodes().
///
/// @param context_id The ID of the context to use (context.h).
/// @param index The index of the node in the node list.
/// @return 0 - inactive, 1 - active, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_node_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_node_active(int context_id, int index);

/// @brief Get the type of the specified node.
///
/// The internal list of nodes only changes with an explicit call to
/// vircadia_update_nodes().
///
/// @param context_id The ID of the context to use (context.h).
/// @param index The index of the node in the node list.
/// @return one of the values defined in node_types.h, or a negative
/// error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_node_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_node_type(int context_id, int index);

#endif /* end of include guard */

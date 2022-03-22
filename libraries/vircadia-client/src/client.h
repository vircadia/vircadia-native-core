//
//  client.h
//  libraries/client/src
//
//  Created by Nshan G. on 15 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef VIRCADIA_LIBRARIES_VIRCADIA_CLIENT_SRC_CLIENT_H
#define VIRCADIA_LIBRARIES_VIRCADIA_CLIENT_SRC_CLIENT_H

#include <stdint.h>

#include "common.h"

/// @brief Defines the layout of client application information.
///
/// Part of vircadia_context_params.
struct vircadia_client_info {
    /// @brief The name of the application (default: VircadiaClient).
    const char* name;

    /// @brief The name of the organization (default: Vircadia).
    const char* organization;

    /// @brief The domain name of the organization (default: vircadia.com).
    const char* domain;

    /// @brief The version of the application (default: client full version string, see vircadia_client_version()).
    const char* version;
};

/// @brief Defines the layout of context parameters.
///
/// Always use vircadia_context_defaults() to create an instance of this struct
/// with properly filled default values. Filling it with zeros is not sufficient.
struct vircadia_context_params {

    /// @brief The UDP port to bind a socket on for listening (default: random port)
    int listenPort;

    /// @brief The DTLS port to bind a socket on for listening (default: not bound)
    int dtlsListenPort;

    /// @brief Platform information in JSON format. (default: null/empty)
    ///
    /// // TODO: figure out the details and maybe accept a struct instead of a JSON string.
    const char* platform_info;

    /// @brief User agent string used for connecting to Metaverse server (default: platform dependent)
    const char* user_agent;

    /// @brief Client application information.
    vircadia_client_info client_info;
};

/// @brief Default values of context parameters.
///
/// @return default initialized context parameter object.
VIRCADIA_CLIENT_DYN_API
vircadia_context_params vircadia_context_defaults();

/// @brief Create the client context.
///
/// Context represents all the state necessary for an instance of a
/// client. This function may allocate various internal data structures,
/// start threads, etc.
/// Note: Currently only one context can be created.
///
/// @param context_params The parameters of the context.
/// @return The ID of the created context, or a negative error code.
VIRCADIA_CLIENT_DYN_API
int vircadia_create_context(vircadia_context_params);

/// @brief Destroy the client context.
///
/// Note: currently only one context can be created.
///
/// @param id The ID of the context to destroy.
/// @return 0 on success, negative error code otherwise.
VIRCADIA_CLIENT_DYN_API
int vircadia_destroy_context(int id);

/// @brief Connect/jump to server on specified address.
///
/// This function only initiates the connection, use
/// vircadia_connection_status() to check if it was established.
///
/// @param id The ID of the context to use.
/// @param address The address to connect/jump to. TODO: clarify the format
/// @return Negative code in case of an error, otherwise 0.
VIRCADIA_CLIENT_DYN_API
int vircadia_connect(int id, const char* address);

/// @brief Retrieve the connection status.
///
/// @param id The ID of the context to use.
/// @return 0 if not connected, 1 if connected, negative error code otherwise.
VIRCADIA_CLIENT_DYN_API
int vircadia_connection_status(int id);

/// @brief Update the internal list of nodes.
///
/// @param id The ID of the context to use.
/// @return Negative code in case of an errors, otherwise 0.
VIRCADIA_CLIENT_DYN_API
int vircadia_update_nodes(int id);

/// @brief Retrieve the count of connected nodes.
//
/// The count only changes with explicit call to vircadia_update_nodes().
///
/// @param id The ID of the context to use.
/// @return The count of connected nodes.
VIRCADIA_CLIENT_DYN_API
int vircadia_node_count(int id);

/// @brief Retrieve the node UUID by index.
///
/// The list of nodes only changes with an explicit call to
/// vircadia_update_nodes(), which may invalidate the pointer returned by this
/// function.
///
/// @param id The ID of the context to use.
/// @return Pointer to 16 byte UUID or null in case of an error.
VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_node_uuid(int id, int index);

#endif /* end of include guard */

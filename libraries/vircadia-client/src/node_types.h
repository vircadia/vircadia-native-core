//
//  node_types.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 13 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_NODE_TYPES_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_NODE_TYPES_H

#include <stdint.h>

#include "common.h"

/// @brief Value indicating domain server node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_domain_server_node();

/// @brief Value indicating entity server node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_entity_server_node();

/// @brief Value indicating agent node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_agent_node();

/// @brief Value indicating audio mixer node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_audio_mixer_node();

/// @brief Value indicating avatar mixer node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_mixer_node();

/// @brief Value indicating asset server node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_asset_server_node();

/// @brief Value indicating messages mixer node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_messages_mixer_node();

/// @brief Value indicating entity script server node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_entity_script_server_node();

/// @brief Value indicating upstream audio mixer node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_upstream_audio_mixer_node();

/// @brief Value indicating upstream audio mixer node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_upstream_avatar_mixer_node();

/// @brief Value indicating downstream audio mixer node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_downstream_audio_mixer_node();

/// @brief Value indicating downstream avatar mixer node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_downstream_avatar_mixer_node();

/// @brief Value indicating unassigned node type.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_unassigned_node();

#endif /* end of include guard */

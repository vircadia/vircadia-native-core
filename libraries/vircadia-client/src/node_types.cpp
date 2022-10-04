//
//  node_types.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 13 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "node_types.h"

#include <NodeType.h>

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_domain_server_node() {
    return NodeType::DomainServer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_entity_server_node() {
    return NodeType::EntityServer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_agent_node() {
    return NodeType::Agent;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_audio_mixer_node() {
    return NodeType::AudioMixer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_avatar_mixer_node() {
    return NodeType::AvatarMixer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_asset_server_node() {
    return NodeType::AssetServer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_messages_mixer_node() {
    return NodeType::MessagesMixer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_entity_script_server_node() {
    return NodeType::EntityScriptServer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_upstream_audio_mixer_node() {
    return NodeType::UpstreamAudioMixer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_upstream_avatar_mixer_node() {
    return NodeType::UpstreamAvatarMixer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_downstream_audio_mixer_node() {
    return NodeType::DownstreamAudioMixer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_downstream_avatar_mixer_node() {
    return NodeType::DownstreamAvatarMixer;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_unassigned_node() {
    return NodeType::Unassigned;
}

